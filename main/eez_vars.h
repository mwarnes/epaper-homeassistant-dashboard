#ifndef EEZ_VARS_H
#define EEZ_VARS_H

#include <stdbool.h>

// Status icon update functions
void eez_update_wifi_icon(bool connected);  // Update WiFi icon image
void eez_update_ha_icon(bool connected);    // Update HA icon image
void eez_update_weather_icon(void);         // Update current weather icon (large 128x128)
void eez_update_forecast_icons(void);       // Update 5-day forecast icons (small 48x48)

// Helper functions (call with LVGL lock after ui_tick())
void eez_center_date_label(void);           // Center date label after text update

// 5-Day Forecast Getters (read access)
const char *get_var_ha_forecast_day1_temp(void);
const char *get_var_ha_forecast_day1_condition(void);
const char *get_var_ha_forecast_day1_name(void);
const char *get_var_ha_forecast_day2_temp(void);
const char *get_var_ha_forecast_day2_condition(void);
const char *get_var_ha_forecast_day2_name(void);
const char *get_var_ha_forecast_day3_temp(void);
const char *get_var_ha_forecast_day3_condition(void);
const char *get_var_ha_forecast_day3_name(void);
const char *get_var_ha_forecast_day4_temp(void);
const char *get_var_ha_forecast_day4_condition(void);
const char *get_var_ha_forecast_day4_name(void);
const char *get_var_ha_forecast_day5_temp(void);
const char *get_var_ha_forecast_day5_condition(void);
const char *get_var_ha_forecast_day5_name(void);

// 5-Day Forecast Setters (write access from HA client task)
void set_var_ha_forecast_day1_temp(const char *value);
void set_var_ha_forecast_day1_condition(const char *value);
void set_var_ha_forecast_day1_name(const char *value);
void set_var_ha_forecast_day2_temp(const char *value);
void set_var_ha_forecast_day2_condition(const char *value);
void set_var_ha_forecast_day2_name(const char *value);
void set_var_ha_forecast_day3_temp(const char *value);
void set_var_ha_forecast_day3_condition(const char *value);
void set_var_ha_forecast_day3_name(const char *value);
void set_var_ha_forecast_day4_temp(const char *value);
void set_var_ha_forecast_day4_condition(const char *value);
void set_var_ha_forecast_day4_name(const char *value);
void set_var_ha_forecast_day5_temp(const char *value);
void set_var_ha_forecast_day5_condition(const char *value);
void set_var_ha_forecast_day5_name(const char *value);

// Screen management (placeholder - extend when you add error screen in EEZ Studio)
void eez_show_error_screen(const char *wifi_status, const char *ha_status, const char *last_update);
void eez_show_dashboard_screen(void);

#endif // EEZ_VARS_H
