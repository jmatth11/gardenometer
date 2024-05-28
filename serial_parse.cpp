#include "serial_parse.h"


const char *code_prefix = "code:";
const char *config_prefix = "config:";
const char *cal_prefix = "cal:";
const char *status_prefix = "status:";

void parse_serial(state_machine_t* sm, struct state* state, String data) {
  if (data.startsWith(code_prefix)) {
    String code = data.substring(strlen(code_prefix));
    int code_value = code.toInt();
    sm->state = int_to_garden_state_t(code_value);
  } else if (data.startsWith(config_prefix)) {
    sm->state = CONFIG;
    state->serial_data = data.substring(strlen(config_prefix));
  } else if (data.startsWith(cal_prefix)) {
    sm->state = CALIBRATION;
  } else {
    sm->state = STATUS_CALL;
  }
}

struct parse_info parse_value(String data) {
  int idx = data.indexOf(';');
  if (idx == -1) {
    idx = data.length();
  }
  String value = data.substring(2, idx);
  int parsedValue = atoi(value.c_str());
  struct parse_info result;
  result.starting_idx = idx;
  result.value = parsedValue;
  if ((unsigned int)idx != data.length()) {
    result.starting_idx++;
  }
  return result;
}

String parse_config(String data, struct config* out) {
  String err_msg;
  struct parse_info result;
  while (data.length() > 0) {
    int idx = atoi(data.substring(0, 1).c_str());
    switch(idx) {
      case WAIT_INDEX: {
        if (data.charAt(1) == '=') {
          result = parse_value(data);
          out->wait_time = result.value;
          data = data.substring(result.starting_idx);
        } else {
          err_msg = err_msg + "error for code:" + idx + "; ";
        }
        break;
      }
      case MOISTURE_AIR:{
        if (data.charAt(1) == '=') {
          result = parse_value(data);
          out->moisture_air = result.value;
          data = data.substring(result.starting_idx);
        } else {
          err_msg = err_msg + "error for code:" + idx + "; ";
        }
        break;
      }
      case MOISTURE_WATER:{
        if (data.charAt(1) == '=') {
          result = parse_value(data);
          out->moisture_water = result.value;
          data = data.substring(result.starting_idx);
        } else {
          err_msg = err_msg + "error for code:" + idx + "; ";
        }
        break;
      }
      default:{
        err_msg = err_msg + "error for code:" + idx + "; ";
        break;
      }
    }
  }
  return err_msg;
}

