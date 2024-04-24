#ifndef GARDENOMETER_TYPES_H
#define GARDENOMETER_TYPES_H

#include <WString.h>
#include <DallasTemperature.h>

struct calibration {
  int airValue;
  int waterValue;
};

enum config_index {
  WAIT_INDEX=0,
  MOISTURE_INDEX,
  LUX_INDEX,
  TEMP_INDEX,
  CAL_INDEX,
  ERR_INDEX,
  GOOD_INDEX,
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

#endif
