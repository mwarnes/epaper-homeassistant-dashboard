#include "time_sync_task.h"
#include "Shared/shared_resources.h"
#include "Shared/config_manager.h"
#include "esp_sntp.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <time.h>
#include <sys/time.h>

static const char *TAG = "time_sync_task";

static void time_sync_notification_cb(struct timeval *tv)
{
    ESP_LOGI(TAG, "Time synchronized");
    xEventGroupSetBits(s_event_group, TIME_SYNCED_BIT);
}

void time_sync_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Time sync task starting on core %d", xPortGetCoreID());
    
    // Wait for WiFi connection
    ESP_LOGI(TAG, "Waiting for WiFi connection...");
    xEventGroupWaitBits(s_event_group,
                        WIFI_CONNECTED_BIT,
                        pdFALSE,
                        pdTRUE,
                        portMAX_DELAY);
    
    ESP_LOGI(TAG, "WiFi connected, initializing SNTP...");
    
    // Set timezone from config
    char timezone[64];
    if (config_get_timezone(timezone, sizeof(timezone)) == ESP_OK) {
        ESP_LOGI(TAG, "Setting timezone: %s", timezone);
        setenv("TZ", timezone, 1);
        tzset();
    } else {
        ESP_LOGW(TAG, "Failed to get timezone, using UTC");
        setenv("TZ", "UTC0", 1);
        tzset();
    }
    
    // Configure SNTP
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_setservername(1, "time.google.com");
    esp_sntp_setservername(2, "time.cloudflare.com");
    esp_sntp_set_time_sync_notification_cb(time_sync_notification_cb);
    esp_sntp_init();
    
    // Wait for time synchronization (30s timeout per attempt, 3 attempts)
    int retry = 0;
    const int retry_count = 3;
    while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && retry < retry_count) {
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry + 1, retry_count);
        vTaskDelay(pdMS_TO_TICKS(10000)); // 10 second delay
        retry++;
    }
    
    if (sntp_get_sync_status() == SNTP_SYNC_STATUS_COMPLETED) {
        time_t now;
        struct tm timeinfo;
        time(&now);
        localtime_r(&now, &timeinfo);
        char strftime_buf[64];
        strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
        ESP_LOGI(TAG, "Current time: %s", strftime_buf);
    } else {
        ESP_LOGW(TAG, "Failed to sync time after %d attempts, continuing anyway", retry_count);
    }
    
    // Periodic re-sync every 24 hours
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(24 * 60 * 60 * 1000)); // 24 hours
        
        if (sntp_get_sync_status() != SNTP_SYNC_STATUS_COMPLETED) {
            ESP_LOGW(TAG, "Time sync lost, will retry automatically");
        } else {
            ESP_LOGI(TAG, "Time sync OK (periodic check)");
        }
    }
}
