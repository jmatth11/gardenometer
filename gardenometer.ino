#include <string.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "state.h"

// define constants
#define CONTROLLER_ID "G1"
#define MOISTURE_PIN A0
#define LUX_PIN A2
#define TEMP_PIN 4
#define GOOD_STATE_PIN = 6;
#define CALIBRATION_PIN = 8;
#define ERROR_PIN = 10;

const char *code_prefix = "code:";
const char *config_prefix = "config:";
const char *cal_prefix = "cal:";

// define structures
struct calibration {
  int airValue;
  int waterValue;
};

enum config_index {
  WAIT_INDEX=0;
  MOISTURE_INDEX;
  LUX_INDEX;
  TEMP_INDEX;
  CAL_INDEX;
  ERR_INDEX;
  GOOD_PIN;
};

struct config {
  int wait_time;
  int moisture_pin;
  int lux_pin;
  int temp_pin;
  int cal_pin;
  int err_pin;
  int good_pin;
};

struct state {
  struct calibration calibration;
  DallasTemperature temperature;
  struct config config;
  String serial_data;
};

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

/**
 * Read and convert moisture value to a percentage value.
 * @param[in] cal The calibration data.
 * @return Percent value of moisture.
 */
int read_soil_moisture(int pin, const struct calibration cal) {
  int raw_value = analogRead(pin);
  return map(raw_value, cal.airValue, cal.waterValue, 0, 100);
}

/**
 * Read the value of the light sensor and convert it to Lux value.
 * @return The lux value.
 */
float read_lux(int pin) {
  float raw_value = analogRead(pin);
  // multiply value by constant to get lux value.
  // this constant assumes AREF is 5v.
  return raw_value * 0.9765625;
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

// initialize globals
struct state state;
state_machine_t state_machine;
// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(TEMP_PIN);

// setup function
void setup() {
  Serial.begin(9600);
  // initialize global structures
  // Pass our oneWire reference to Dallas Temperature sensor
  DallasTemperature sensors(&oneWire);
  state_machine.state = STATUS;
  state_machine.status = garden_status;
  state_machine.calibration = garden_calibration;
  state_machine.error = garden_error;
  state_machine.config = garden_config;
  struct calibration calibration;
  memset(&calibration, 0, sizeof(struct calibration));
  state.calibration = calibration;
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
  pinMode(config.good_pin, OUTPUT)
  pinMode(config.cal_pin, OUTPUT);
  pinMode(config.err_pin, OUTPUT);
  pinMode(config.moisture_pin, INPUT);
  pinMode(cofnig.lux_pin, INPUT);
  sensors.begin();
  digitalWrite(config.good_pin, HIGH);
  digitalWrite(config.err_pin, LOW);
}

void parse_serial(state_machine_t* sm, String data) {
  if (data.startsWith(code_prefix)) {
    String code = data.substring(strlen(code_prefix));
    int code_value = atoi(code);
    sm->state = code_value;
  } else if (data.startsWith(config_prefix)) {
    sm->state = CONFIG;
    sm->serial_data = data.substring(strlen(config_prefix));
  } else if (data.startsWith(cal_prefix)) {
    sm->state = CALIBRATION;
  }
}

void flip_leds_on_state(const state_machine_t* sm) {
  if (sm->state == ERROR) {
    digitalWrite(sm->config.good_pin, LOW);
    digitalWrite(sm->config.err_pin, HIGH);
  } else if (sm->state == CLEAR_ERROR) {
    digitalWrite(sm->config.good_pin, HIGH);
    digitalWrite(sm->config.err_pin, LOW);
  }
}

// loop function
void loop() {
  state.serial_data = "";
  flip_leds_on_state(&state_machine);
  if (Serial.available()) {
    String data = Serial.readStringUntil('\n');
    parse_serial(&state_machine, data);
  }
  handle_state_machine(&state_machine, &state);
  delay(state.config.wait_time);
}

int avg_moisture() {
  int tmp = 0;
  for(int i = 0; i < 5; ++i) {
    tmp += analogRead(MOISTURE_PIN);
    delay(100);
  }
  return tmp / 5;
}

// implement state functions
void garden_status(state_machine_t *machine, void* context) {
  struct state *state = (struct state *)context;
  int moisture = read_soil_moisture(state->config.moisture_pin, state->calibration);
  float lux = read_lux(state->config.lux_pin);
  float temp = read_temperature(&state->temperature);
  String value = "t=" + String(temp) + ";l=" + String(lux) + ";m=" + String(moisture);
  println(value);
}

void garden_calibration(state_machine_t *machine, void* context) {
  struct state *state = (struct state *)context;
  digitalWrite(state->config.cal_pin, HIGH);
  delay(1000);
  digitalWrite(state->config.cal_pin, LOW);
  state->calibration.airValue = avg_moisture();
  digitalWrite(state->config.cal_pin, HIGH);
  delay(1000);
  digitalWrite(state->config.cal_pin, LOW);
  state->calibration.waterValue = avg_moisture();
  machine->state = STATUS;
}

void garden_error(state_machine_t *machine, void* context) {
  if (machine->state == ERROR) {
    digitalWrite(state->config.err_pin, HIGH);
    machine->state = NONE;
  } else {
    digitalWrite(state->config.err_pin, LOW);
    machine->state = STATUS;
  }
}

int parse_value(String data) {
  String value = data.substring(2, data.indexOf(';'));
  return atoi(value.c_str());
}

void publish_error(String err) {
  println("status:error=\""+err+"\"");
}

struct config parse_config(String data) {
  String err_msg = "data format error";
  struct config local;
  while (data != "\n") {
    config_index idx = atoi(data.charAt(0));
    switch(idx) {
      case WAIT_INDEX: {
        if (data.charAt(1) == '=') {
          local.wait_time = parse_value(data);
          data = data.substring(data.indexOf(';')+1);
        } else {
          publish_error(err_msg + " code:" + idx);
        }
        break;
      }
      case MOISTURE_INDEX:{
        if (data.charAt(1) == '=') {
          local.moisture_pin = parse_value(data);
          data = data.substring(data.indexOf(';')+1);
        } else {
          publish_error(err_msg + " code:" + idx);
        }
        break;
      }
      case LUX_INDEX:{
        if (data.charAt(1) == '=') {
          local.lux_pin = parse_value(data);
          data = data.substring(data.indexOf(';')+1);
        } else {
          publish_error(err_msg + " code:" + idx);
        }
        break;
      }
      case TEMP_INDEX:{
        if (data.charAt(1) == '=') {
          local.temp_pin = parse_value(data);
          data = data.substring(data.indexOf(';')+1);
        } else {
          publish_error(err_msg + " code:" + idx);
        }
        break;
      }
      case CAL_INDEX:{
        if (data.charAt(1) == '=') {
          local.cal_pin = parse_value(data);
          data = data.substring(data.indexOf(';')+1);
        } else {
          publish_error(err_msg + " code:" + idx);
        }
        break;
      }
      case ERR_INDEX:{
        if (data.charAt(1) == '=') {
          local.err_pin = parse_value(data);
          data = data.substring(data.indexOf(';')+1);
        } else {
          publish_error(err_msg + " code:" + idx);
        }
        break;
      }
      case GOOD_INDEX:{
        if (data.charAt(1) == '=') {
          local.good_pin = parse_value(data);
          data = data.substring(data.indexOf(';')+1);
        } else {
          publish_error(err_msg + " code:" + idx);
        }
        break;
      }
      default:{
        publish_error(err_msg + " code:" + idx);
        break;
      }
    }
  }
}

int get_config_value(enum config_index type, int orig, int incoming) {
  if (incoming != 0 && orig != incoming) {
    switch (type) {
      case MOISTURE_INDEX:
      case LUX_INDEX:
      case TEMP_INDEX:
        pinMode(incoming, INPUT);
        break;
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
  struct config local = parse_config(state->serial_data);
  state->config.wait_time = get_config_value(state->config.wait_time, local.wait_time);
  state->config.moisture_pin = get_config_value(state->config.moisture_pin, local.moisture_pin);
  state->config.lux_pin = get_config_value(state->config.lux_pin, local.lux_pin);
  state->config.temp_pin = get_config_value(state->config.temp_pin, local.temp_pin);
  state->config.cal_pin = get_config_value(state->config.cal_pin, local.cal_pin);
  state->config.err_pin = get_config_value(state->config.err_pin, local.err_pin);
  state->config.good_pin = get_config_value(state->config.good_pin, local.good_pin);
  digitalWrite(state->config.err_pin, LOW);
  digitalWrite(state->config.good_pin, HIGH);
  machine->state = STATUS;
}
