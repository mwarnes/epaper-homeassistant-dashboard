#include <stdio.h>
#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "Shared/shared_resources.h"
#include "Shared/config_manager.h"
#include "Tasks/wifi_task.h"
#include "Tasks/ha_client_task.h"
#include "Tasks/power_mgmt_task.h"
#include "lvgl_epaper_port.h"
#include "ui/ui.h"
#include "esp_lvgl_port.h"
#include "Tasks/display_task.h"
#include "Tasks/epaper_diagnostics.h"
#include "gdem102_driver.h"  // For direct driver access in diagnostics mode

// ========================================
// DIAGNOSTICS MODE
// Uncomment the line below to run display diagnostics instead of normal operation
// This will run a series of test patterns to diagnose display issues
// ========================================
// #define ENABLE_EPAPER_DIAGNOSTICS

static const char *TAG = "main";

void app_main(void)
{
#ifdef ENABLE_EPAPER_DIAGNOSTICS
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "E-PAPER DIAGNOSTICS MODE ENABLED");
    ESP_LOGI(TAG, "========================================");
    
    // Initialize NVS (required for driver)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    // Initialize e-paper driver directly (no LVGL needed for diagnostics)
    ESP_LOGI(TAG, "Initializing e-paper driver for diagnostics...");
    gdem102_config_t epaper_config = GDEM102_DEFAULT_CONFIG();
    ESP_ERROR_CHECK(gdem102_init(&epaper_config));
    
    ESP_LOGI(TAG, "Starting diagnostics in 5 seconds...");
    ESP_LOGI(TAG, "Watch the display and monitor logs for instructions");
    vTaskDelay(pdMS_TO_TICKS(5000));
    
    // Run diagnostics
    epaper_run_diagnostics();
    
    ESP_LOGI(TAG, "Diagnostics complete. System will now idle.");
    ESP_LOGI(TAG, "To exit diagnostics mode, comment out ENABLE_EPAPER_DIAGNOSTICS and reflash");
    
    // Idle forever
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
#else
    ESP_LOGI(TAG, "E-Paper Home Assistant Dashboard starting...");
    
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    // Initialize shared resources
    shared_resources_init();
    
    // Load configuration
    ESP_ERROR_CHECK(config_init());
    
    // Initialize LVGL e-paper port (creates LVGL task automatically)
    ESP_LOGI(TAG, "Initializing LVGL e-paper port...");
    ESP_ERROR_CHECK(lvgl_epaper_port_init());
    
    // Initialize EEZ UI (must lock LVGL port)
    ESP_LOGI(TAG, "Initializing EEZ UI...");
    if (lvgl_port_lock(10000)) {
        ui_init();
        lvgl_port_unlock();
        ESP_LOGI(TAG, "EEZ UI initialized");
    } else {
        ESP_LOGE(TAG, "Failed to lock LVGL for UI init");
        abort();
    }
    
    // Create WiFi task on Core 0
    xTaskCreatePinnedToCore(
        wifi_task,
        "wifi_task",
        4096,
        NULL,
        5,
        NULL,
        0  // Core 0
    );
    
    // Create HA client task on Core 0 (fetches date/time from Home Assistant)
    xTaskCreatePinnedToCore(
        ha_client_task,
        "ha_client_task",
        6144,
        NULL,
        4,
        NULL,
        0  // Core 0
    );
    
    // Create power management task (no core affinity)
    xTaskCreate(
        power_mgmt_task,
        "power_mgmt_task",
        2048,
        NULL,
        2,
        NULL
    );
    
    // Note: LVGL task is auto-created by lvgl_port_init() inside lvgl_epaper_port_init()
    
    // Create display task on Core 1 (larger stack for LVGL + e-paper operations)
    xTaskCreatePinnedToCore(
        display_task,
        "display_task",
        8192,  // Increased from 4096 - LVGL operations are stack-intensive
        NULL,
        5,
        NULL,
        1  // Core 1
    );
    
    ESP_LOGI(TAG, "Initialization complete");
#endif  // ENABLE_EPAPER_DIAGNOSTICS
}
