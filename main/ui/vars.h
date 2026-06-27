#ifndef EEZ_LVGL_UI_VARS_H
#define EEZ_LVGL_UI_VARS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// enum declarations

// Flow global variables

enum FlowGlobalVariables {
    FLOW_GLOBAL_VARIABLE_HA_TIME = 0,
    FLOW_GLOBAL_VARIABLE_HA_DATE = 1,
    FLOW_GLOBAL_VARIABLE_HA_HOME_REALFEEL_TEMPERATURE = 2,
    FLOW_GLOBAL_VARIABLE_HA_HOME_CONDITION_TODAY = 3,
    FLOW_GLOBAL_VARIABLE_HA_FORECAST_DAY1_TEMP = 4,
    FLOW_GLOBAL_VARIABLE_HA_FORECAST_DAY1_CONDITION = 5,
    FLOW_GLOBAL_VARIABLE_HA_FORECAST_DAY1_NAME = 6,
    FLOW_GLOBAL_VARIABLE_HA_FORECAST_DAY2_TEMP = 7,
    FLOW_GLOBAL_VARIABLE_HA_FORECAST_DAY2_CONDITION = 8,
    FLOW_GLOBAL_VARIABLE_HA_FORECAST_DAY2_NAME = 9,
    FLOW_GLOBAL_VARIABLE_HA_FORECAST_DAY3_TEMP = 10,
    FLOW_GLOBAL_VARIABLE_HA_FORECAST_DAY3_CONDITION = 11,
    FLOW_GLOBAL_VARIABLE_HA_FORECAST_DAY3_NAME = 12,
    FLOW_GLOBAL_VARIABLE_HA_FORECAST_DAY4_TEMP = 13,
    FLOW_GLOBAL_VARIABLE_HA_FORECAST_DAY4_CONDITION = 14,
    FLOW_GLOBAL_VARIABLE_HA_FORECAST_DAY4_NAME = 15,
    FLOW_GLOBAL_VARIABLE_HA_FORECAST_DAY5_TEMP = 16,
    FLOW_GLOBAL_VARIABLE_HA_FORECAST_DAY5_CONDITION = 17,
    FLOW_GLOBAL_VARIABLE_HA_FORECAST_DAY5_NAME = 18
};

// Native global variables

extern const char *get_var_ha_time();
extern void set_var_ha_time(const char *value);
extern const char *get_var_ha_date();
extern void set_var_ha_date(const char *value);
extern const char *get_var_ha_home_realfeel_temperature();
extern void set_var_ha_home_realfeel_temperature(const char *value);
extern const char *get_var_ha_home_condition_today();
extern void set_var_ha_home_condition_today(const char *value);
extern const char *get_var_ha_forecast_day1_temp();
extern void set_var_ha_forecast_day1_temp(const char *value);
extern const char *get_var_ha_forecast_day1_condition();
extern void set_var_ha_forecast_day1_condition(const char *value);
extern const char *get_var_ha_forecast_day1_name();
extern void set_var_ha_forecast_day1_name(const char *value);
extern const char *get_var_ha_forecast_day2_temp();
extern void set_var_ha_forecast_day2_temp(const char *value);
extern const char *get_var_ha_forecast_day2_condition();
extern void set_var_ha_forecast_day2_condition(const char *value);
extern const char *get_var_ha_forecast_day2_name();
extern void set_var_ha_forecast_day2_name(const char *value);
extern const char *get_var_ha_forecast_day3_temp();
extern void set_var_ha_forecast_day3_temp(const char *value);
extern const char *get_var_ha_forecast_day3_condition();
extern void set_var_ha_forecast_day3_condition(const char *value);
extern const char *get_var_ha_forecast_day3_name();
extern void set_var_ha_forecast_day3_name(const char *value);
extern const char *get_var_ha_forecast_day4_temp();
extern void set_var_ha_forecast_day4_temp(const char *value);
extern const char *get_var_ha_forecast_day4_condition();
extern void set_var_ha_forecast_day4_condition(const char *value);
extern const char *get_var_ha_forecast_day4_name();
extern void set_var_ha_forecast_day4_name(const char *value);
extern const char *get_var_ha_forecast_day5_temp();
extern void set_var_ha_forecast_day5_temp(const char *value);
extern const char *get_var_ha_forecast_day5_condition();
extern void set_var_ha_forecast_day5_condition(const char *value);
extern const char *get_var_ha_forecast_day5_name();
extern void set_var_ha_forecast_day5_name(const char *value);

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_VARS_H*/