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
// 5 minutes
unsigned long print_wait_period = 300000;

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
  String msg = "Lux:";
  msg += String(lux) + " Temp:" + String(temp) + " Moisture:" + String(moisture);
  display.write(header, msg);
}

/**
 * Read and convert moisture value to a percentage value.
 * @param[in] cal The calibration data.
 * @return Percent value of moisture, if both calibration values are zero the raw value is returned.
 */
int read_soil_moisture(int pin, const struct calibration cal) {
  int raw_value = analogRead(pin);
  if (cal.airValue == 0 && cal.waterValue == 0) {
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
  config.good_pin = GOOD_STATE_PIN;
  config.cal_pin = CALIBRATION_PIN;
  config.err_pin = ERROR_PIN;
  config.lux_pin = LUX_PIN;
  config.moisture_pin = MOISTURE_PIN;
  config.temp_pin = TEMP_PIN;
  state.config = config;
  state.temperature = sensors;

  // set pins
  pinMode(config.good_pin, OUTPUT);
  pinMode(config.cal_pin, OUTPUT);
  pinMode(config.err_pin, OUTPUT);
  pinMode(config.moisture_pin, INPUT);
  pinMode(config.lux_pin, INPUT);
  sensors.begin();
  digitalWrite(config.good_pin, HIGH);
  digitalWrite(config.cal_pin, LOW);
  digitalWrite(config.err_pin, LOW);
}

void flip_leds_on_state(const state_machine_t* sm, struct state *state) {
  if (sm->state == ERROR) {
    digitalWrite(state->config.good_pin, LOW);
    digitalWrite(state->config.err_pin, HIGH);
  } else if (sm->state == CLEAR_ERROR) {
    digitalWrite(state->config.good_pin, HIGH);
    digitalWrite(state->config.err_pin, LOW);
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
  if (state->last_print != 0 && diff <= print_wait_period) {
    return;
  }
  int moisture = read_soil_moisture(state->config.moisture_pin, state->calibration);
  float lux = read_lux(state->config.lux_pin);
  float temp = read_temperature(&state->temperature);
  String value = String(status_prefix) + "t=" + String(temp) + ";l=" + String(lux) + ";m=" + String(moisture);
  println(value);
  display_status(moisture, lux, temp);
  state->last_print = millis();
}

void garden_calibration(state_machine_t *machine, void* context) {
  struct state *state = (struct state *)context;
  display.write(calibration_header, "");
  digitalWrite(state->config.cal_pin, HIGH);
  delay(3000);
  digitalWrite(state->config.cal_pin, LOW);
  display.write(calibration_header, cal_air_msg);
  state->calibration.airValue = avg_moisture();
  display.write(calibration_header, cal_switch_msg);
  digitalWrite(state->config.cal_pin, HIGH);
  delay(3000);
  digitalWrite(state->config.cal_pin, LOW);
  display.write(calibration_header, cal_water_msg);
  state->calibration.waterValue = avg_moisture();
  display.write(calibration_header, done_msg);
  machine->state = STATUS_CALL;
}

void garden_error(state_machine_t *machine, void* context) {
  struct state* state = (struct state*)context;
  if (machine->state == ERROR) {
    digitalWrite(state->config.err_pin, HIGH);
    machine->state = NONE;
  } else {
    digitalWrite(state->config.err_pin, LOW);
    machine->state = STATUS_CALL;
  }
}

void publish_error(String err) {
  display.write(err_header, err);
  println("status:e="+err);
}

int get_config_value(struct state* state, enum config_index type, int orig, int incoming) {
  if (incoming != 0 && orig != incoming) {
    switch (type) {
      case MOISTURE_INDEX:
      case LUX_INDEX:
        pinMode(incoming, INPUT);
        break;
      case TEMP_INDEX:{
        // temp already has a pointer to oneWire
        oneWire.begin(incoming);
        state->temperature.begin();
        break;
      }
      case CAL_INDEX:
      case ERR_INDEX:
      case GOOD_INDEX:
        pinMode(incoming, OUTPUT);
        break;
      default:
        break;
    }
    return incoming;
  }
  return orig;
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
  state->config.wait_time = get_config_value(state, WAIT_INDEX, state->config.wait_time, local.wait_time);
  state->config.moisture_pin = get_config_value(state, MOISTURE_INDEX, state->config.moisture_pin, local.moisture_pin);
  state->config.lux_pin = get_config_value(state, LUX_INDEX, state->config.lux_pin, local.lux_pin);
  state->config.temp_pin = get_config_value(state, TEMP_INDEX, state->config.temp_pin, local.temp_pin);
  state->config.cal_pin = get_config_value(state, CAL_INDEX, state->config.cal_pin, local.cal_pin);
  state->config.err_pin = get_config_value(state, ERR_INDEX, state->config.err_pin, local.err_pin);
  state->config.good_pin = get_config_value(state, GOOD_INDEX, state->config.good_pin, local.good_pin);
  digitalWrite(state->config.err_pin, LOW);
  digitalWrite(state->config.good_pin, HIGH);
  display.write(
    done_msg,
    display.getCenteredX(done_msg),
    display.getCenteredY(done_msg)
  );
  machine->state = STATUS_CALL;
}
