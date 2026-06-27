#include "wifi_task.h"
#include "Shared/shared_resources.h"
#include "Shared/config_manager.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "wifi_task";
static int s_retry_num = 0;
static const int MAX_RETRY = 5;

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        // Always retry connection (important for long-running dashboard)
        // Log differently for initial connection vs reconnection attempts
        if (s_retry_num < MAX_RETRY) {
            s_retry_num++;
            ESP_LOGI(TAG, "Initial connection: retry %d/%d", s_retry_num, MAX_RETRY);
        } else {
            ESP_LOGI(TAG, "Connection lost, retrying... (attempt %d)", s_retry_num + 1);
            s_retry_num++;  // Keep incrementing to show retry count
        }
        esp_wifi_connect();
        xEventGroupClearBits(s_event_group, WIFI_CONNECTED_BIT);
        
        // Update state
        if (xSemaphoreTake(dashboard_state_mutex, portMAX_DELAY) == pdTRUE) {
            dashboard_state.wifi_connected = false;
            xSemaphoreGive(dashboard_state_mutex);
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_event_group, WIFI_CONNECTED_BIT);
        
        // Update state
        if (xSemaphoreTake(dashboard_state_mutex, portMAX_DELAY) == pdTRUE) {
            dashboard_state.wifi_connected = true;
            xSemaphoreGive(dashboard_state_mutex);
        }
    }
}

void wifi_task(void *pvParameters)
{
    ESP_LOGI(TAG, "WiFi task starting on core %d", xPortGetCoreID());
    
    // Initialize network interface
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
    
    // Initialize WiFi
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    
    // Register event handlers
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));
    
    // Get WiFi credentials from config
    char ssid[64] = {0};
    char password[64] = {0};
    config_get_wifi(ssid, sizeof(ssid), password, sizeof(password));
    
    // Configure WiFi
    wifi_config_t wifi_config = {
        .sta = {
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .capable = true,
                .required = false
            },
        },
    };
    strncpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char *)wifi_config.sta.password, password, sizeof(wifi_config.sta.password) - 1);
    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    
    // Disable WiFi power save to prevent beacon timeout on e-paper device
    // E-paper refresh takes 25 seconds, causing missed beacons in power save mode
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
    ESP_LOGI(TAG, "WiFi power save disabled for e-paper compatibility");
    
    ESP_LOGI(TAG, "WiFi initialized, connecting to %s...", ssid);
    
    // Wait for connection with exponential backoff
    bool connected = false;
    for (int i = 0; i < MAX_RETRY; i++) {
        EventBits_t bits = xEventGroupWaitBits(s_event_group,
                                                WIFI_CONNECTED_BIT,
                                                pdFALSE,
                                                pdFALSE,
                                                pdMS_TO_TICKS((1 << i) * 1000)); // 1s, 2s, 4s, 8s, 16s
        
        if (bits & WIFI_CONNECTED_BIT) {
            ESP_LOGI(TAG, "Connected to WiFi successfully");
            connected = true;
            break;
        }
    }
    
    // Task complete - WiFi event handlers will manage reconnection
    if (connected) {
        ESP_LOGI(TAG, "WiFi task initialization complete - connected");
    } else {
        ESP_LOGW(TAG, "WiFi task initialization complete - still connecting in background");
        ESP_LOGW(TAG, "HA dashboard will start when WiFi connects");
    }
    vTaskDelete(NULL);
}
