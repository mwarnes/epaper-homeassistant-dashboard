#include "display_task.h"
#include "Shared/shared_resources.h"
#include "Shared/config_manager.h"
#include "eez_vars.h"
#include "ui/ui.h"
#include "esp_log.h"
#include "esp_task_wdt.h"
#include "lvgl.h"
#include "esp_lvgl_port.h"        // For lvgl_port_lock/unlock
#include "lvgl_epaper_port.h"    // For lvgl_epaper_port_refresh() and _get_display()
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <time.h>

static const char *TAG = "display_task";

// Cache of last displayed values
static struct {
    char date_str[32];
    char time_str[32];
    bool wifi_connected;
    bool ha_connected;
    bool error_screen_shown;
    bool first_update;
} s_last_displayed = {
    .date_str = "",
    .time_str = "",
    .wifi_connected = false,
    .ha_connected = false,
    .error_screen_shown = false,
    .first_update = true
};

void display_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Display task starting on core %d", xPortGetCoreID());
    
    // Note: Task watchdog monitoring of IDLE1 is disabled via sdkconfig
    // (CONFIG_ESP_TASK_WDT_CHECK_IDLE_TASK_CPU1=n) to allow 25s e-paper refresh
    // Display task itself is not subscribed to watchdog by default
    ESP_LOGI(TAG, "Display task ready for long-running e-paper operations");
    
    // Wait for LVGL to be ready (initialized in app_main)
    // Give it 1 second to settle
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    // Load the initial screen
    ESP_LOGI(TAG, "Loading initial screen...");
    
    if (lvgl_port_lock(10000)) {
        loadScreen(SCREEN_ID_MAIN);
        lvgl_port_unlock();
        ESP_LOGI(TAG, "Screen loaded successfully");
    } else {
        ESP_LOGE(TAG, "Could not lock LVGL to load screen - aborting");
        vTaskDelete(NULL);
        return;
    }
    
    // Step 2: Wait 100ms
    ESP_LOGI(TAG, "Rendering to framebuffer...");
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // Step 3: Refresh framebuffer with lock
    if (lvgl_port_lock(10000)) {
        lv_display_t *display = lvgl_epaper_port_get_display();
        if (display != NULL) {
            lv_refr_now(display);
        } else {
            ESP_LOGE(TAG, "Display object is NULL!");
        }
        lvgl_port_unlock();
    }
    
    // Step 4: Wait 500ms for flush callbacks to complete
    vTaskDelay(pdMS_TO_TICKS(500));
    
    // Step 6: Refresh e-paper display
    ESP_LOGI(TAG, "Refreshing e-paper display (~25 seconds)...");
    esp_err_t refresh_err = lvgl_epaper_port_refresh();
    if (refresh_err == ESP_OK) {
        ESP_LOGI(TAG, "Initial display shown successfully");
    } else {
        ESP_LOGE(TAG, "Initial display refresh failed: %s", esp_err_to_name(refresh_err));
    }
    
    ESP_LOGI(TAG, "Display task monitoring state changes...");
    
    while (1) {
        bool needs_update = false;
        bool show_error = false;
        
        // Read current state
        char current_date[32] = {0};
        char current_time[32] = {0};
        bool wifi_connected = false;
        bool ha_connected = false;
        time_t last_update = 0;
        
        if (xSemaphoreTake(dashboard_state_mutex, portMAX_DELAY) == pdTRUE) {
            strncpy(current_date, dashboard_state.date_str, sizeof(current_date) - 1);
            strncpy(current_time, dashboard_state.time_str, sizeof(current_time) - 1);
            wifi_connected = dashboard_state.wifi_connected;
            ha_connected = dashboard_state.ha_connected;
            last_update = dashboard_state.last_successful_update;
            
            xSemaphoreGive(dashboard_state_mutex);
        }
        
        // Check if error bit is set
        EventBits_t bits = xEventGroupGetBits(s_event_group);
        show_error = (bits & HA_ERROR_BIT) != 0;
        
        // Detect changes
        if (s_last_displayed.first_update ||
            strcmp(current_date, s_last_displayed.date_str) != 0 ||
            strcmp(current_time, s_last_displayed.time_str) != 0 ||
            wifi_connected != s_last_displayed.wifi_connected ||
            ha_connected != s_last_displayed.ha_connected ||
            show_error != s_last_displayed.error_screen_shown) {
            
            needs_update = true;
        }
        
        if (needs_update) {
            ESP_LOGI(TAG, "State changed, updating display...");
            
            // Lock LVGL port
            if (lvgl_port_lock(10000)) {  // 10 second timeout
                
                if (show_error && !s_last_displayed.error_screen_shown) {
                    // Show error screen
                    char wifi_status[32];
                    char ha_status[32];
                    char last_update_str[64];
                    
                    snprintf(wifi_status, sizeof(wifi_status), "%s", wifi_connected ? "Connected" : "Disconnected");
                    snprintf(ha_status, sizeof(ha_status), "%s", ha_connected ? "Connected" : "Disconnected");
                    
                    if (last_update > 0) {
                        struct tm timeinfo;
                        localtime_r(&last_update, &timeinfo);
                        strftime(last_update_str, sizeof(last_update_str), "%Y-%m-%d %H:%M:%S", &timeinfo);
                    } else {
                        strncpy(last_update_str, "Never", sizeof(last_update_str) - 1);
                    }
                    
                    eez_show_error_screen(wifi_status, ha_status, last_update_str);
                    s_last_displayed.error_screen_shown = true;
                    
                } else if (!show_error && s_last_displayed.error_screen_shown) {
                    // Return to dashboard screen
                    eez_show_dashboard_screen();
                    s_last_displayed.error_screen_shown = false;
                }
                
                if (!show_error) {
                    // Update dashboard screen variables
                    if (strlen(current_date) > 0) {
                        eez_set_date(current_date);
                    }
                    if (strlen(current_time) > 0) {
                        eez_set_time(current_time);
                    }
                    eez_set_wifi_status(wifi_connected);
                    eez_set_ha_status(ha_connected);
                }
                
                lvgl_port_unlock();
            } else {
                ESP_LOGW(TAG, "Could not acquire LVGL port lock for update");
                continue;  // Skip this update
            }
            
            // Step 2: Wait 100ms
            vTaskDelay(pdMS_TO_TICKS(100));
            
            // Step 3: Refresh framebuffer with lock
            if (lvgl_port_lock(10000)) {
                lv_display_t *display = lvgl_epaper_port_get_display();
                if (display != NULL) {
                    lv_refr_now(display);
                }
                lvgl_port_unlock();
            }
            
            // Step 4: Wait 500ms for flush callbacks
            vTaskDelay(pdMS_TO_TICKS(500));
            
            // Step 5: Refresh e-paper display
            ESP_LOGI(TAG, "Refreshing e-paper display (~25 seconds)...");
            esp_err_t refresh_err = lvgl_epaper_port_refresh();
            if (refresh_err == ESP_OK) {
                ESP_LOGI(TAG, "E-paper refresh complete");
            } else {
                ESP_LOGE(TAG, "E-paper refresh failed: %s", esp_err_to_name(refresh_err));
            }
            
            // Update cache
            strncpy(s_last_displayed.date_str, current_date, sizeof(s_last_displayed.date_str) - 1);
            strncpy(s_last_displayed.time_str, current_time, sizeof(s_last_displayed.time_str) - 1);
            s_last_displayed.wifi_connected = wifi_connected;
            s_last_displayed.ha_connected = ha_connected;
            s_last_displayed.first_update = false;
            
            ESP_LOGI(TAG, "Display updated successfully");
        }
        
        // Poll every 1 second
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
