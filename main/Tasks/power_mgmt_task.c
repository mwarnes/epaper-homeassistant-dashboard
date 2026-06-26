#include "power_mgmt_task.h"
#include "Shared/config_manager.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "power_mgmt_task";

void power_mgmt_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Power management task starting");
    
    uint8_t power_mode = config_get_power_mode();
    
    switch (power_mode) {
    case 0:
        ESP_LOGI(TAG, "Power mode: Always-On (no power management)");
        break;
    case 1:
        ESP_LOGI(TAG, "Power mode: Deep Sleep (not implemented in Phase 1)");
        break;
    case 2:
        ESP_LOGI(TAG, "Power mode: Light Sleep (not implemented in Phase 1)");
        break;
    default:
        ESP_LOGW(TAG, "Unknown power mode: %d, defaulting to Always-On", power_mode);
        break;
    }
    
    // Phase 1: Always-on mode, task does nothing
    // Future: Implement deep sleep and light sleep modes
    
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(60000)); // Sleep for 1 minute
    }
}
