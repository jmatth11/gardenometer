#include "serial_parse.h"

void parse_serial(state_machine_t* sm, struct state* state, String data) {
  if (data.startsWith(code_prefix)) {
    String code = data.substring(strlen(code_prefix));
    int code_value = code.toInt();
    sm->state = code_value;
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
  if (idx != data.length()) {
    result.starting_idx++;
  }
  return result;
}

struct config parse_config(String data) {
  String err_msg = "data format error";
  struct config local;
  struct parse_info result;
  while (data.length() > 0) {
    config_index idx = atoi(data.charAt(0));
    switch(idx) {
      case WAIT_INDEX: {
        if (data.charAt(1) == '=') {
          result = parse_value(data);
          local.wait_time = result.value;
          data = data.substring(result.starting_idx);
        } else {
          publish_error(err_msg + " code:" + idx);
        }
        break;
      }
      case MOISTURE_INDEX:{
        if (data.charAt(1) == '=') {
          result = parse_value(data);
          local.moisture_pin = parse_value(data);
          data = data.substring(result.starting_idx);
        } else {
          publish_error(err_msg + " code:" + idx);
        }
        break;
      }
      case LUX_INDEX:{
        if (data.charAt(1) == '=') {
          result = parse_value(data);
          local.lux_pin = result.value;
          data = data.substring(result.starting_idx);
        } else {
          publish_error(err_msg + " code:" + idx);
        }
        break;
      }
      case TEMP_INDEX:{
        if (data.charAt(1) == '=') {
          result = parse_value(data);
          local.temp_pin = result.value;
          data = data.substring(result.starting_idx);
        } else {
          publish_error(err_msg + " code:" + idx);
        }
        break;
      }
      case CAL_INDEX:{
        if (data.charAt(1) == '=') {
          result = parse_value(data);
          local.cal_pin = result.value;
          data = data.substring(result.starting_idx);
        } else {
          publish_error(err_msg + " code:" + idx);
        }
        break;
      }
      case ERR_INDEX:{
        if (data.charAt(1) == '=') {
          result = parse_value(data);
          local.err_pin = result.value;
          data = data.substring(result.starting_idx);
        } else {
          publish_error(err_msg + " code:" + idx);
        }
        break;
      }
      case GOOD_INDEX:{
        if (data.charAt(1) == '=') {
          result = parse_value(data);
          local.good_pin = result.value;
          data = data.substring(result.starting_idx);
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

