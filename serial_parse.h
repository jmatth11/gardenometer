#ifndef GARDENOMETER_SERIAL_PARSE
#define GARDENOMETER_SERIAL_PARSE

#include <WString.h>
#include "state.h"
#include "garden_types.h"

extern const char *code_prefix;
extern const char *config_prefix;
extern const char *cal_prefix;
extern const char *status_prefix;

struct parse_info {
  int value;
  int starting_idx;
};

void parse_serial(state_machine_t* sm, struct state* state, String data);
String parse_config(String data, struct config* out);
struct parse_info parse_value(String data);

#endif
