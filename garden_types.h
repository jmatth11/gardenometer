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
  MOISTURE_AIR,
  MOISTURE_WATER,
};

struct config {
  int wait_time;
  int moisture_air;
  int moisture_water;
};

struct state {
  struct calibration calibration;
  DallasTemperature temperature;
  struct config config;
  String serial_data;
  unsigned long last_print;
};

#endif
