#ifndef GARDENOMETER_SERIAL_PARSE
#define GARDENOMETER_SERIAL_PARSE

#include <WString.h>
#include "state.h"
#include "garden_types.h"

const char *code_prefix = "code:";
const char *config_prefix = "config:";
const char *cal_prefix = "cal:";
const char *status_prefix = "status:";

struct parse_info {
  int value;
  int starting_idx;
};

void parse_serial(state_machine_t* sm, struct state* state, String data);
struct config parse_config(String data);
struct parse_info parse_value(String data);

#endif
