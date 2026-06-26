#include "eez_vars.h"
#include "ui/screens.h"
#include "ui/ui.h"
#include "lvgl.h"
#include "esp_log.h"
#include <stdio.h>

static const char *TAG = "eez_vars";

void eez_set_date(const char *date)
{
    // Note: You removed lbl_date from EEZ Studio
    // If you want to display date separately, add a lblDate widget in EEZ Studio
    // For now, just log it
    if (date) {
        ESP_LOGD(TAG, "Date update: %s", date);
    }
}

void eez_set_time(const char *time)
{
    if (objects.lbl_time && time) {
        lv_label_set_text(objects.lbl_time, time);
        ESP_LOGD(TAG, "Updated time: %s", time);
    } else {
        ESP_LOGW(TAG, "Time label not available");
    }
}

void eez_set_wifi_status(bool connected)
{
    if (objects.lbl_wifi_status) {
        lv_label_set_text(objects.lbl_wifi_status, connected ? "WiFi: OK" : "WiFi: ERROR");
        // E-paper 4-color: Black for normal, Red for errors
        lv_obj_set_style_text_color(objects.lbl_wifi_status,
                                     connected ? lv_color_black() : lv_color_make(255, 0, 0),
                                     0);
        ESP_LOGD(TAG, "Updated WiFi status: %s", connected ? "connected" : "disconnected");
    } else {
        ESP_LOGW(TAG, "WiFi status label not available");
    }
}

void eez_set_ha_status(bool connected)
{
    if (objects.lbl_ha_status) {
        lv_label_set_text(objects.lbl_ha_status, connected ? "HA: OK" : "HA: ERROR");
        // E-paper 4-color: Black for normal, Red for errors
        lv_obj_set_style_text_color(objects.lbl_ha_status,
                                     connected ? lv_color_black() : lv_color_make(255, 0, 0),
                                     0);
        ESP_LOGD(TAG, "Updated HA status: %s", connected ? "connected" : "disconnected");
    } else {
        ESP_LOGW(TAG, "HA status label not available");
    }
}

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
    if (objects.lbl_time) {
        lv_label_set_text(objects.lbl_time, "HA Error - Check logs");
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

// Color cycling removed - caused LVGL deadlocks and memory corruption
// Dashboard now focuses on displaying HA data reliably
