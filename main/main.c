#include <stdio.h>
#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "Shared/shared_resources.h"
#include "Shared/config_manager.h"
#include "Tasks/wifi_task.h"
#include "Tasks/time_sync_task.h"
#include "Tasks/ha_client_task.h"
#include "Tasks/power_mgmt_task.h"
#include "lvgl_epaper_port.h"
#include "ui/ui.h"
#include "esp_lvgl_port.h"
#include "Tasks/display_task.h"

static const char *TAG = "main";

void app_main(void)
{
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
    
    // Create time sync task on Core 0
    xTaskCreatePinnedToCore(
        time_sync_task,
        "time_sync_task",
        3072,
        NULL,
        3,
        NULL,
        0  // Core 0
    );
    
    // Create HA client task on Core 0
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
}
