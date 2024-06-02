#include <string.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <OLEDHelper.h>
#include "state.h"
#include "garden_types.h"
#include "serial_parse.h"

// define constants
#define CONTROLLER_ID "G1"
#define MOISTURE_PIN A1
#define LUX_PIN A2
#define TEMP_PIN 4
#define GOOD_STATE_PIN 10
#define CALIBRATION_PIN 8
#define ERROR_PIN 6
const String init_str = "Initializing...";
const String err_header = "ERROR";
const String calibration_header = "Calibrating";
const String cal_air_msg = "Reading air values...";
const String cal_water_msg = "Reading water values...";
const String cal_switch_msg = "Put moisture sensor in water.";
const String done_msg = "Done.";
const String config_header = "Configuring.";
// 1 minute
unsigned long print_wait_period = 60000;

// initialize globals
struct state state;
state_machine_t state_machine;
OLEDHelper display;
// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(TEMP_PIN);

// define functions
void garden_status(state_machine_t *machine, void* context);
void garden_calibration(state_machine_t *machine, void* context);
void garden_error(state_machine_t *machine, void* context);
void garden_config(state_machine_t *machine, void* context);

void println(String msg) {
  if (!msg.endsWith(";")) {
    msg = msg + ";";
  }
  Serial.println(msg + "id="+CONTROLLER_ID);
}

void display_status(int moisture, float lux, float temp) {
  String header = String(CONTROLLER_ID) + " DATA";
  String msg = "L:";
  msg = msg + String(lux) + " T:" + String(temp) + "\n M:" + String(moisture);
  display.write(header, msg);
}

/**
 * Read and convert moisture value to a percentage value.
 * @param[in] cal The calibration data.
 * @return Percent value of moisture, if both calibration values are zero the raw value is returned.
 */
int read_soil_moisture(int pin, const struct calibration cal) {
  int raw_value = analogRead(pin);
  if (cal.airValue == cal.waterValue) {
    return raw_value;
  }
  return map(raw_value, cal.airValue, cal.waterValue, 0, 100);
}

/**
 * Read the value of the light sensor and convert it to Lux value.
 * @return The lux value.
 */
float read_lux(int pin) {
  int const AREF = 5.0;
  float raw_value = analogRead(pin);
  float volts = raw_value * AREF / 1024.0;    // Convert reading to voltage
  float amps = volts / 10000.0;             // Convert to amps across 10K resistor
  float microamps = amps * 1000000.0;             // Convert amps to microamps
  return microamps * 2.0;                  // 2 microamps = 1 lux
}

/**
 * Read the temperature sensor and convert it to Fahrenheit value.
 * @param[in] sensor The Dallas Temperature sensor.
 * @return The temperature in Fahrenheit.
 */
float read_temperature(DallasTemperature *sensor) {
  sensor->requestTemperatures();
  return sensor->getTempFByIndex(0);
}

// setup function
void setup() {
  Serial.begin(9600);
  Serial.println("booting up...");
  // initialize global structures
  // Pass our oneWire reference to Dallas Temperature sensor
  DallasTemperature sensors(&oneWire);
  if (!display.setup()) {
    publish_error("display failed to initialize.");
  }
  display.write(
    init_str,
    display.getCenteredX(init_str),
    display.getCenteredY(init_str)
  );
  state_machine.state = STATUS_CALL;
  state_machine.status = garden_status;
  state_machine.calibration = garden_calibration;
  state_machine.error = garden_error;
  state_machine.config = garden_config;
  struct calibration calibration;
  // default calibration values
  calibration.airValue = 0;
  calibration.waterValue = 0;
  state.calibration = calibration;
  state.last_print = 0;
  struct config config;
  config.wait_time = 1000;
  state.config = config;
  state.temperature = sensors;

  // set pins
  pinMode(GOOD_STATE_PIN, OUTPUT);
  pinMode(CALIBRATION_PIN, OUTPUT);
  pinMode(ERROR_PIN, OUTPUT);
  pinMode(MOISTURE_PIN, INPUT);
  pinMode(LUX_PIN, INPUT);
  sensors.begin();
  digitalWrite(GOOD_STATE_PIN, HIGH);
  digitalWrite(CALIBRATION_PIN, LOW);
  digitalWrite(ERROR_PIN, LOW);
}

void flip_leds_on_state(const state_machine_t* sm, struct state *state) {
  if (sm->state == ERROR) {
    digitalWrite(GOOD_STATE_PIN, LOW);
    digitalWrite(ERROR_PIN, HIGH);
  } else if (sm->state == CLEAR_ERROR) {
    digitalWrite(GOOD_STATE_PIN, HIGH);
    digitalWrite(ERROR_PIN, LOW);
  }
}

// loop function
void loop() {
  state.serial_data = "";
  flip_leds_on_state(&state_machine, &state);
  if (Serial.available()) {
    String data = Serial.readStringUntil('\n');
    parse_serial(&state_machine, &state, data);
  }
  handle_state_machine(&state_machine, &state);
  delay(state.config.wait_time);
}

int avg_moisture() {
  int tmp = 0;
  for(int i = 0; i < 5; ++i) {
    tmp += analogRead(MOISTURE_PIN);
    delay(1000);
  }
  return tmp / 5;
}

// implement state functions
void garden_status(state_machine_t *machine, void* context) {
  struct state *state = (struct state *)context;
  unsigned long now = millis();
  unsigned long diff = now - state->last_print;
  if (state->last_print != 0 && diff <= print_wait_period && now > state->last_print) {
    return;
  }
  int moisture = read_soil_moisture(MOISTURE_PIN, state->calibration);
  float lux = read_lux(LUX_PIN);
  float temp = read_temperature(&state->temperature);
  String value = String(status_prefix) + "t=" + String(temp) + ";l=" + String(lux) + ";m=" + String(moisture);
  println(value);
  display_status(moisture, lux, temp);
  state->last_print = millis();
}

void garden_calibration(state_machine_t *machine, void* context) {
  struct state *state = (struct state *)context;
  display.write(calibration_header, "");
  digitalWrite(CALIBRATION_PIN, HIGH);
  delay(3000);
  digitalWrite(CALIBRATION_PIN, LOW);
  display.write(calibration_header, cal_air_msg);
  state->calibration.airValue = avg_moisture();
  display.write(calibration_header, cal_switch_msg);
  digitalWrite(CALIBRATION_PIN, HIGH);
  delay(3000);
  digitalWrite(CALIBRATION_PIN, LOW);
  display.write(calibration_header, cal_water_msg);
  state->calibration.waterValue = avg_moisture();
  display.write(calibration_header, done_msg);
  machine->state = STATUS_CALL;
}

void garden_error(state_machine_t *machine, void* context) {
  struct state* state = (struct state*)context;
  if (machine->state == ERROR) {
    digitalWrite(ERROR_PIN, HIGH);
    machine->state = NONE;
  } else {
    digitalWrite(ERROR_PIN, LOW);
    machine->state = STATUS_CALL;
  }
}

void publish_error(String err) {
  display.write(err_header, err);
  println("status:e="+err);
}

void garden_config(state_machine_t *machine, void* context) {
  struct state *state = (struct state *)context;
  display.write(
    config_header,
    display.getCenteredX(config_header),
    display.getCenteredY(config_header)
  );
  struct config local;
  String err = parse_config(state->serial_data, &local);
  if (err.length() > 0) {
    publish_error(err);
  }
  state->config.wait_time = local.wait_time;
  state->calibration.airValue = local.moisture_air;
  state->calibration.waterValue = local.moisture_water;
  digitalWrite(ERROR_PIN, LOW);
  digitalWrite(GOOD_STATE_PIN, HIGH);
  display.write(
    done_msg,
    display.getCenteredX(done_msg),
    display.getCenteredY(done_msg)
  );
  machine->state = STATUS_CALL;
}
