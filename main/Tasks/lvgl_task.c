#include "lvgl_task.h"
#include "Shared/shared_resources.h"
#include "ui/ui.h"
#include "esp_log.h"
#include "esp_lvgl_port.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// E-paper port header (from esp-lvgl-epaper-port component)
#include "lvgl_epaper_port.h"

static const char *TAG = "lvgl_task";

void lvgl_task(void *pvParameters)
{
    ESP_LOGI(TAG, "LVGL task starting on core %d", xPortGetCoreID());
    
    // Initialize LVGL port
    ESP_LOGI(TAG, "Initializing LVGL e-paper port...");
    esp_err_t err = lvgl_epaper_port_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize LVGL e-paper port: %s", esp_err_to_name(err));
        vTaskDelete(NULL);
        return;
    }
    
    // Initialize EEZ UI - with mutex locked
    ESP_LOGI(TAG, "Initializing EEZ UI...");
    if (xSemaphoreTake(lvgl_mutex, portMAX_DELAY) == pdTRUE) {
        ui_init();
        xSemaphoreGive(lvgl_mutex);
        ESP_LOGI(TAG, "EEZ UI initialized");
    }
    
    ESP_LOGI(TAG, "LVGL task initialization complete");
    
    // Main loop: run LVGL timer handler
    while (1) {
        // Lock LVGL mutex
        if (xSemaphoreTake(lvgl_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            // Run LVGL tasks
            uint32_t time_till_next = lv_timer_handler();
            
            // Release mutex
            xSemaphoreGive(lvgl_mutex);
            
            // Sleep until next timer event (but at least 1ms, max 10ms)
            if (time_till_next > 10) time_till_next = 10;
            if (time_till_next < 1) time_till_next = 1;
            vTaskDelay(pdMS_TO_TICKS(time_till_next));
        } else {
            // Couldn't get mutex, wait a bit
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
}
