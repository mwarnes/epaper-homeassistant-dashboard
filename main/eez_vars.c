#include "eez_vars.h"
#include "ui/screens.h"
#include "ui/ui.h"
#include "ui/images.h"  // For WiFi icon images
#include "lvgl.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>

static const char *TAG = "eez_vars";

//
// HA Variable Storage
// All Home Assistant data flows through these variables
// Pattern follows EEZ Studio conventions
//

char ha_time[100] = { 0 };
char ha_date[100] = { 0 };
char ha_wifi_status[100] = { 0 };
char ha_ha_status[100] = { 0 };
char ha_home_realfeel_temperature[100] = { 0 };
char ha_home_condition_today[100] = { 0 };

// Additional current weather variables (names match EEZ Studio variables)
char ha_wind[50] = { 0 };                  // e.g., "15 km/h NE" or "15 NE"
char ha_humidity[50] = { 0 };              // e.g., "65%" or "65"
char ha_air_quality_pm25[50] = { 0 };      // e.g., "12 µg/m³" or "12"

// 5-Day Forecast Variables (15 total: 5 days × 3 variables)
char ha_forecast_day1_temp[50] = { 0 };
char ha_forecast_day1_condition[50] = { 0 };
char ha_forecast_day1_name[10] = { 0 };

char ha_forecast_day2_temp[50] = { 0 };
char ha_forecast_day2_condition[50] = { 0 };
char ha_forecast_day2_name[10] = { 0 };

char ha_forecast_day3_temp[50] = { 0 };
char ha_forecast_day3_condition[50] = { 0 };
char ha_forecast_day3_name[10] = { 0 };

char ha_forecast_day4_temp[50] = { 0 };
char ha_forecast_day4_condition[50] = { 0 };
char ha_forecast_day4_name[10] = { 0 };

char ha_forecast_day5_temp[50] = { 0 };
char ha_forecast_day5_condition[50] = { 0 };
char ha_forecast_day5_name[10] = { 0 };

//
// HA Variable Getters (read-only access)
//

const char *get_var_ha_time()
{
    return ha_time;
}

const char *get_var_ha_date()
{
    return ha_date;
}

const char *get_var_ha_wifi_status()
{
    return ha_wifi_status;
}

const char *get_var_ha_ha_status()
{
    return ha_ha_status;
}

const char *get_var_ha_home_realfeel_temperature()
{
    return ha_home_realfeel_temperature;
}

const char *get_var_ha_home_condition_today()
{
    return ha_home_condition_today;
}

const char *get_var_ha_wind()
{
    return ha_wind;
}

const char *get_var_ha_humidity()
{
    return ha_humidity;
}

const char *get_var_ha_air_quality_pm25()
{
    return ha_air_quality_pm25;
}

//
// HA Variable Setters (write access from HA client task)
//

void set_var_ha_time(const char *value)
{
    if (value) {
        strncpy(ha_time, value, sizeof(ha_time) / sizeof(char));
        ha_time[sizeof(ha_time) / sizeof(char) - 1] = 0;
        ESP_LOGD(TAG, "HA variable updated: time = %s", ha_time);
        // Note: EEZ Studio's tick_screen_*() automatically updates label from variable
    }
}

void set_var_ha_date(const char *value)
{
    if (value) {
        strncpy(ha_date, value, sizeof(ha_date) / sizeof(char));
        ha_date[sizeof(ha_date) / sizeof(char) - 1] = 0;
        ESP_LOGD(TAG, "HA variable updated: date = %s", ha_date);
        // Note: EEZ Studio's tick_screen_*() automatically updates label from variable
        // Centering happens after ui_tick() via eez_center_date_label()
    }
}

void set_var_ha_wifi_status(const char *value)
{
    if (value) {
        strncpy(ha_wifi_status, value, sizeof(ha_wifi_status) / sizeof(char));
        ha_wifi_status[sizeof(ha_wifi_status) / sizeof(char) - 1] = 0;
        ESP_LOGD(TAG, "HA variable updated: wifi_status = %s", ha_wifi_status);
        // Note: WiFi status now displayed via icon (img_wifi_state), not text label
    }
}

void set_var_ha_ha_status(const char *value)
{
    if (value) {
        strncpy(ha_ha_status, value, sizeof(ha_ha_status) / sizeof(char));
        ha_ha_status[sizeof(ha_ha_status) / sizeof(char) - 1] = 0;
        ESP_LOGD(TAG, "HA variable updated: ha_status = %s", ha_ha_status);
        // Note: HA status now displayed via icon (img_ha_state), not text label
    }
}

void set_var_ha_home_realfeel_temperature(const char *value)
{
    if (value) {
        strncpy(ha_home_realfeel_temperature, value, sizeof(ha_home_realfeel_temperature) / sizeof(char));
        ha_home_realfeel_temperature[sizeof(ha_home_realfeel_temperature) / sizeof(char) - 1] = 0;
        ESP_LOGI(TAG, "HA variable updated: home_realfeel_temperature = '%s' (len=%d)", 
                 ha_home_realfeel_temperature, strlen(ha_home_realfeel_temperature));
        // Note: EEZ Studio's tick_screen_*() automatically updates label from variable
    }
}

void set_var_ha_home_condition_today(const char *value)
{
    if (value) {
        strncpy(ha_home_condition_today, value, sizeof(ha_home_condition_today) / sizeof(char));
        ha_home_condition_today[sizeof(ha_home_condition_today) / sizeof(char) - 1] = 0;
        ESP_LOGI(TAG, "HA variable updated: home_condition_today = '%s' (len=%d)", 
                 ha_home_condition_today, strlen(ha_home_condition_today));
        // Note: EEZ Studio's tick_screen_*() automatically updates label from variable
    }
}

void set_var_ha_wind(const char *value)
{
    if (value) {
        strncpy(ha_wind, value, sizeof(ha_wind));
        ha_wind[sizeof(ha_wind) - 1] = 0;
        ESP_LOGD(TAG, "HA variable updated: wind = '%s'", ha_wind);
    }
}

void set_var_ha_humidity(const char *value)
{
    if (value) {
        strncpy(ha_humidity, value, sizeof(ha_humidity));
        ha_humidity[sizeof(ha_humidity) - 1] = 0;
        ESP_LOGD(TAG, "HA variable updated: humidity = '%s'", ha_humidity);
    }
}

void set_var_ha_air_quality_pm25(const char *value)
{
    if (value) {
        strncpy(ha_air_quality_pm25, value, sizeof(ha_air_quality_pm25));
        ha_air_quality_pm25[sizeof(ha_air_quality_pm25) - 1] = 0;
        ESP_LOGD(TAG, "HA variable updated: air_quality_pm25 = '%s'", ha_air_quality_pm25);
    }
}

//
// 5-Day Forecast Getters
//

// Day 1
const char *get_var_ha_forecast_day1_temp() { return ha_forecast_day1_temp; }
const char *get_var_ha_forecast_day1_condition() { return ha_forecast_day1_condition; }
const char *get_var_ha_forecast_day1_name() { return ha_forecast_day1_name; }

// Day 2
const char *get_var_ha_forecast_day2_temp() { return ha_forecast_day2_temp; }
const char *get_var_ha_forecast_day2_condition() { return ha_forecast_day2_condition; }
const char *get_var_ha_forecast_day2_name() { return ha_forecast_day2_name; }

// Day 3
const char *get_var_ha_forecast_day3_temp() { return ha_forecast_day3_temp; }
const char *get_var_ha_forecast_day3_condition() { return ha_forecast_day3_condition; }
const char *get_var_ha_forecast_day3_name() { return ha_forecast_day3_name; }

// Day 4
const char *get_var_ha_forecast_day4_temp() { return ha_forecast_day4_temp; }
const char *get_var_ha_forecast_day4_condition() { return ha_forecast_day4_condition; }
const char *get_var_ha_forecast_day4_name() { return ha_forecast_day4_name; }

// Day 5
const char *get_var_ha_forecast_day5_temp() { return ha_forecast_day5_temp; }
const char *get_var_ha_forecast_day5_condition() { return ha_forecast_day5_condition; }
const char *get_var_ha_forecast_day5_name() { return ha_forecast_day5_name; }

//
// 5-Day Forecast Setters
//

void set_var_ha_forecast_day1_temp(const char *value) {
    if (value) {
        strncpy(ha_forecast_day1_temp, value, sizeof(ha_forecast_day1_temp));
        ha_forecast_day1_temp[sizeof(ha_forecast_day1_temp) - 1] = 0;
        ESP_LOGD(TAG, "Forecast Day 1 temp = '%s'", ha_forecast_day1_temp);
    }
}

void set_var_ha_forecast_day1_condition(const char *value) {
    if (value) {
        strncpy(ha_forecast_day1_condition, value, sizeof(ha_forecast_day1_condition));
        ha_forecast_day1_condition[sizeof(ha_forecast_day1_condition) - 1] = 0;
        ESP_LOGD(TAG, "Forecast Day 1 condition = '%s'", ha_forecast_day1_condition);
    }
}

void set_var_ha_forecast_day1_name(const char *value) {
    if (value) {
        strncpy(ha_forecast_day1_name, value, sizeof(ha_forecast_day1_name));
        ha_forecast_day1_name[sizeof(ha_forecast_day1_name) - 1] = 0;
        ESP_LOGD(TAG, "Forecast Day 1 name = '%s'", ha_forecast_day1_name);
    }
}

void set_var_ha_forecast_day2_temp(const char *value) {
    if (value) {
        strncpy(ha_forecast_day2_temp, value, sizeof(ha_forecast_day2_temp));
        ha_forecast_day2_temp[sizeof(ha_forecast_day2_temp) - 1] = 0;
        ESP_LOGD(TAG, "Forecast Day 2 temp = '%s'", ha_forecast_day2_temp);
    }
}

void set_var_ha_forecast_day2_condition(const char *value) {
    if (value) {
        strncpy(ha_forecast_day2_condition, value, sizeof(ha_forecast_day2_condition));
        ha_forecast_day2_condition[sizeof(ha_forecast_day2_condition) - 1] = 0;
        ESP_LOGD(TAG, "Forecast Day 2 condition = '%s'", ha_forecast_day2_condition);
    }
}

void set_var_ha_forecast_day2_name(const char *value) {
    if (value) {
        strncpy(ha_forecast_day2_name, value, sizeof(ha_forecast_day2_name));
        ha_forecast_day2_name[sizeof(ha_forecast_day2_name) - 1] = 0;
        ESP_LOGD(TAG, "Forecast Day 2 name = '%s'", ha_forecast_day2_name);
    }
}

void set_var_ha_forecast_day3_temp(const char *value) {
    if (value) {
        strncpy(ha_forecast_day3_temp, value, sizeof(ha_forecast_day3_temp));
        ha_forecast_day3_temp[sizeof(ha_forecast_day3_temp) - 1] = 0;
        ESP_LOGD(TAG, "Forecast Day 3 temp = '%s'", ha_forecast_day3_temp);
    }
}

void set_var_ha_forecast_day3_condition(const char *value) {
    if (value) {
        strncpy(ha_forecast_day3_condition, value, sizeof(ha_forecast_day3_condition));
        ha_forecast_day3_condition[sizeof(ha_forecast_day3_condition) - 1] = 0;
        ESP_LOGD(TAG, "Forecast Day 3 condition = '%s'", ha_forecast_day3_condition);
    }
}

void set_var_ha_forecast_day3_name(const char *value) {
    if (value) {
        strncpy(ha_forecast_day3_name, value, sizeof(ha_forecast_day3_name));
        ha_forecast_day3_name[sizeof(ha_forecast_day3_name) - 1] = 0;
        ESP_LOGD(TAG, "Forecast Day 3 name = '%s'", ha_forecast_day3_name);
    }
}

void set_var_ha_forecast_day4_temp(const char *value) {
    if (value) {
        strncpy(ha_forecast_day4_temp, value, sizeof(ha_forecast_day4_temp));
        ha_forecast_day4_temp[sizeof(ha_forecast_day4_temp) - 1] = 0;
        ESP_LOGD(TAG, "Forecast Day 4 temp = '%s'", ha_forecast_day4_temp);
    }
}

void set_var_ha_forecast_day4_condition(const char *value) {
    if (value) {
        strncpy(ha_forecast_day4_condition, value, sizeof(ha_forecast_day4_condition));
        ha_forecast_day4_condition[sizeof(ha_forecast_day4_condition) - 1] = 0;
        ESP_LOGD(TAG, "Forecast Day 4 condition = '%s'", ha_forecast_day4_condition);
    }
}

void set_var_ha_forecast_day4_name(const char *value) {
    if (value) {
        strncpy(ha_forecast_day4_name, value, sizeof(ha_forecast_day4_name));
        ha_forecast_day4_name[sizeof(ha_forecast_day4_name) - 1] = 0;
        ESP_LOGD(TAG, "Forecast Day 4 name = '%s'", ha_forecast_day4_name);
    }
}

void set_var_ha_forecast_day5_temp(const char *value) {
    if (value) {
        strncpy(ha_forecast_day5_temp, value, sizeof(ha_forecast_day5_temp));
        ha_forecast_day5_temp[sizeof(ha_forecast_day5_temp) - 1] = 0;
        ESP_LOGD(TAG, "Forecast Day 5 temp = '%s'", ha_forecast_day5_temp);
    }
}

void set_var_ha_forecast_day5_condition(const char *value) {
    if (value) {
        strncpy(ha_forecast_day5_condition, value, sizeof(ha_forecast_day5_condition));
        ha_forecast_day5_condition[sizeof(ha_forecast_day5_condition) - 1] = 0;
        ESP_LOGD(TAG, "Forecast Day 5 condition = '%s'", ha_forecast_day5_condition);
    }
}

void set_var_ha_forecast_day5_name(const char *value) {
    if (value) {
        strncpy(ha_forecast_day5_name, value, sizeof(ha_forecast_day5_name));
        ha_forecast_day5_name[sizeof(ha_forecast_day5_name) - 1] = 0;
        ESP_LOGD(TAG, "Forecast Day 5 name = '%s'", ha_forecast_day5_name);
    }
}

//
// Screen management functions
//

void eez_show_error_screen(const char *wifi_status, const char *ha_status, const char *last_update)
{
    // TODO: Add an error screen in EEZ Studio with error details
    // For now, just log the error
    
    ESP_LOGI(TAG, "Error screen requested:");
    ESP_LOGI(TAG, "  WiFi: %s", wifi_status ? wifi_status : "unknown");
    ESP_LOGI(TAG, "  HA: %s", ha_status ? ha_status : "unknown");
    ESP_LOGI(TAG, "  Last update: %s", last_update ? last_update : "never");
    
    // When you add an error screen, load it:
    // loadScreen(SCREEN_ID_ERROR);
    
    // For now, show error in time label:
    if (objects.ha_lbl_time) {
        lv_label_set_text(objects.ha_lbl_time, "HA Error - Check logs");
    }
}

void eez_show_dashboard_screen(void)
{
    // Load the main screen
    if (objects.main) {
        loadScreen(SCREEN_ID_MAIN);
        ESP_LOGI(TAG, "Switched to dashboard screen");
    }
}

void eez_update_wifi_icon(bool connected)
{
    if (objects.img_wifi_state) {
        // Set white background for transparent pixels
        lv_obj_set_style_bg_color(objects.img_wifi_state, lv_color_white(), 0);
        lv_obj_set_style_bg_opa(objects.img_wifi_state, LV_OPA_COVER, 0);
        
        // Update icon image
        if (connected) {
            lv_image_set_src(objects.img_wifi_state, &img_wifi_on);
            ESP_LOGD(TAG, "WiFi icon: ON (black)");
        } else {
            lv_image_set_src(objects.img_wifi_state, &img_wifi_off);
            ESP_LOGD(TAG, "WiFi icon: OFF (red)");
        }
    } else {
        ESP_LOGW(TAG, "WiFi icon widget not available");
    }
}

void eez_update_ha_icon(bool connected)
{
    if (objects.img_ha_state) {
        // Set white background (even though your icons already have it)
        lv_obj_set_style_bg_color(objects.img_ha_state, lv_color_white(), 0);
        lv_obj_set_style_bg_opa(objects.img_ha_state, LV_OPA_COVER, 0);
        
        // Update icon image
        if (connected) {
            lv_image_set_src(objects.img_ha_state, &img_ha_on);
            ESP_LOGD(TAG, "HA icon: ON (connected)");
        } else {
            lv_image_set_src(objects.img_ha_state, &img_ha_off);
            ESP_LOGD(TAG, "HA icon: OFF (disconnected)");
        }
    } else {
        ESP_LOGW(TAG, "HA icon widget not available");
    }
}

// Helper: Get large weather icon (128x128) for current weather
static const lv_image_dsc_t* get_large_weather_icon(const char *condition)
{
    if (!condition) return NULL;
    
    if (strcmp(condition, "Sunny") == 0) return &img_weather_sunny_large;
    if (strcmp(condition, "Clear night") == 0) return &img_weather_night_large;
    if (strcmp(condition, "Partly cloudy") == 0) return &img_weather_partly_cloudy_large;
    if (strcmp(condition, "Cloudy") == 0) return &img_weather_cloudy_large;
    if (strcmp(condition, "Foggy") == 0) return &img_weather_fog_large;
    if (strcmp(condition, "Rainy") == 0) return &img_weather_rainy_large;
    if (strcmp(condition, "Pouring") == 0) return &img_weather_pouring_large;
    if (strcmp(condition, "Snowy") == 0) return &img_weather_snowy_large;
    if (strcmp(condition, "Snow & Rain") == 0) return &img_weather_snowy_rainy_large;
    if (strcmp(condition, "Hail") == 0) return &img_weather_hail_large;
    if (strcmp(condition, "Lightning") == 0) return &img_weather_lightning_large;
    if (strcmp(condition, "Lightning & Rain") == 0) return &img_weather_lightning_rainy_large;
    if (strcmp(condition, "Windy") == 0) return &img_weather_windy_large;
    if (strcmp(condition, "Exceptional") == 0) return &img_weather_alert_circle_large;
    
    return NULL;  // Unknown condition
}

// Helper: Get small weather icon (48x48) for forecast
static const lv_image_dsc_t* get_small_weather_icon(const char *condition)
{
    if (!condition) return NULL;
    
    if (strcmp(condition, "Sunny") == 0) return &img_weather_sunny_small;
    if (strcmp(condition, "Clear night") == 0) return &img_weather_night_small;
    if (strcmp(condition, "Partly cloudy") == 0) return &img_weather_partly_cloudy_small;
    if (strcmp(condition, "Cloudy") == 0) return &img_weather_cloudy_small;
    if (strcmp(condition, "Foggy") == 0) return &img_weather_fog_small;
    if (strcmp(condition, "Rainy") == 0) return &img_weather_rainy_small;
    if (strcmp(condition, "Pouring") == 0) return &img_weather_pouring_small;
    if (strcmp(condition, "Snowy") == 0) return &img_weather_snowy_small;
    if (strcmp(condition, "Snow & Rain") == 0) return &img_weather_snowy_rainy_small;
    if (strcmp(condition, "Hail") == 0) return &img_weather_hail_small;
    if (strcmp(condition, "Lightning") == 0) return &img_weather_lightning_small;
    if (strcmp(condition, "Lightning & Rain") == 0) return &img_weather_lightning_rainy_small;
    if (strcmp(condition, "Windy") == 0) return &img_weather_windy_small;
    if (strcmp(condition, "Exceptional") == 0) return &img_weather_alert_circle_small;
    
    return NULL;  // Unknown condition
}

// Update current weather icon (large 128x128)
void eez_update_weather_icon(void)
{
    // Get current weather condition from variable
    const char *condition = get_var_ha_home_condition_today();
    
    if (!condition || strlen(condition) == 0) {
        ESP_LOGD(TAG, "Weather condition not set yet");
        return;
    }
    
    // Get the large icon for current weather
    const lv_image_dsc_t *icon = get_large_weather_icon(condition);
    
    if (!icon) {
        ESP_LOGW(TAG, "Unknown weather condition: %s", condition);
        return;
    }
    
    // Update the current weather icon widget
    // Widget name: img_weather_current (from EEZ Studio)
    if (objects.img_weather_current) {
        lv_obj_set_style_bg_color(objects.img_weather_current, lv_color_white(), 0);
        lv_obj_set_style_bg_opa(objects.img_weather_current, LV_OPA_COVER, 0);
        lv_image_set_src(objects.img_weather_current, icon);
        ESP_LOGD(TAG, "Updated current weather icon: %s", condition);
    } else {
        ESP_LOGW(TAG, "img_weather_current widget not found");
    }
}

// Update 5-day forecast icons (small 48x48)
void eez_update_forecast_icons(void)
{
    // Day 1
    const char *day1_condition = get_var_ha_forecast_day1_condition();
    if (day1_condition && strlen(day1_condition) > 0) {
        const lv_image_dsc_t *icon = get_small_weather_icon(day1_condition);
        if (icon && objects.img_forecast_day1) {
            lv_obj_set_style_bg_color(objects.img_forecast_day1, lv_color_white(), 0);
            lv_obj_set_style_bg_opa(objects.img_forecast_day1, LV_OPA_COVER, 0);
            lv_image_set_src(objects.img_forecast_day1, icon);
        }
    }
    
    // Day 2
    const char *day2_condition = get_var_ha_forecast_day2_condition();
    if (day2_condition && strlen(day2_condition) > 0) {
        const lv_image_dsc_t *icon = get_small_weather_icon(day2_condition);
        if (icon && objects.img_forecast_day2) {
            lv_obj_set_style_bg_color(objects.img_forecast_day2, lv_color_white(), 0);
            lv_obj_set_style_bg_opa(objects.img_forecast_day2, LV_OPA_COVER, 0);
            lv_image_set_src(objects.img_forecast_day2, icon);
        }
    }
    
    // Day 3
    const char *day3_condition = get_var_ha_forecast_day3_condition();
    if (day3_condition && strlen(day3_condition) > 0) {
        const lv_image_dsc_t *icon = get_small_weather_icon(day3_condition);
        if (icon && objects.img_forecast_day3) {
            lv_obj_set_style_bg_color(objects.img_forecast_day3, lv_color_white(), 0);
            lv_obj_set_style_bg_opa(objects.img_forecast_day3, LV_OPA_COVER, 0);
            lv_image_set_src(objects.img_forecast_day3, icon);
        }
    }
    
    // Day 4
    const char *day4_condition = get_var_ha_forecast_day4_condition();
    if (day4_condition && strlen(day4_condition) > 0) {
        const lv_image_dsc_t *icon = get_small_weather_icon(day4_condition);
        if (icon && objects.img_forecast_day4) {
            lv_obj_set_style_bg_color(objects.img_forecast_day4, lv_color_white(), 0);
            lv_obj_set_style_bg_opa(objects.img_forecast_day4, LV_OPA_COVER, 0);
            lv_image_set_src(objects.img_forecast_day4, icon);
        }
    }
    
    // Day 5
    const char *day5_condition = get_var_ha_forecast_day5_condition();
    if (day5_condition && strlen(day5_condition) > 0) {
        const lv_image_dsc_t *icon = get_small_weather_icon(day5_condition);
        if (icon && objects.img_forecast_day5) {
            lv_obj_set_style_bg_color(objects.img_forecast_day5, lv_color_white(), 0);
            lv_obj_set_style_bg_opa(objects.img_forecast_day5, LV_OPA_COVER, 0);
            lv_image_set_src(objects.img_forecast_day5, icon);
        }
    }
    
    ESP_LOGD(TAG, "Updated forecast icons for 5 days");
}

// Helper function to center the date label (call after ui_tick() updates the label)
void eez_center_date_label(void)
{
    if (objects.ha_lbl_date) {
        // Force LVGL to recalculate label dimensions
        lv_obj_update_layout(objects.ha_lbl_date);
        
        // Get parent (screen) width
        lv_obj_t *parent = lv_obj_get_parent(objects.ha_lbl_date);
        int32_t parent_width = lv_obj_get_width(parent);
        
        // Get label width (calculated based on current text)
        int32_t label_width = lv_obj_get_width(objects.ha_lbl_date);
        
        // Calculate centered X position
        int32_t x_centered = (parent_width - label_width) / 2;
        
        // Keep original Y position, update X to center
        int32_t y_pos = lv_obj_get_y(objects.ha_lbl_date);
        lv_obj_set_pos(objects.ha_lbl_date, x_centered, y_pos);
        
        ESP_LOGD(TAG, "Date label centered: x=%ld (parent=%ld, label=%ld)",
                 x_centered, parent_width, label_width);
    }
}

// Color cycling removed - caused LVGL deadlocks and memory corruption
// Dashboard now focuses on displaying HA data reliably
