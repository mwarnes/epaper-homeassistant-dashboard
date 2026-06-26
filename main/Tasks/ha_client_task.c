#include "ha_client_task.h"
#include "Shared/shared_resources.h"
#include "Shared/config_manager.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <time.h>

static const char *TAG = "ha_client_task";

#define HTTP_RESPONSE_BUFFER_SIZE 4096  // Increased from 2048 to avoid overflow

static char s_http_response_buffer[HTTP_RESPONSE_BUFFER_SIZE];
static int s_http_response_len = 0;

static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    switch (evt->event_id) {
    case HTTP_EVENT_ON_DATA:
        if (s_http_response_len + evt->data_len < HTTP_RESPONSE_BUFFER_SIZE - 1) {
            memcpy(s_http_response_buffer + s_http_response_len, evt->data, evt->data_len);
            s_http_response_len += evt->data_len;
            s_http_response_buffer[s_http_response_len] = '\0';
        } else {
            ESP_LOGE(TAG, "HTTP response buffer overflow: current=%d, incoming=%d, max=%d",
                     s_http_response_len, evt->data_len, HTTP_RESPONSE_BUFFER_SIZE);
            return ESP_FAIL;
        }
        break;
    case HTTP_EVENT_ERROR:
        ESP_LOGE(TAG, "HTTP_EVENT_ERROR");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
        break;
    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
        break;
    default:
        break;
    }
    return ESP_OK;
}

static esp_err_t ha_fetch_entity(const char *ha_url, const char *ha_token, 
                                   const char *entity_id, char *state_out, size_t state_len)
{
    esp_err_t err = ESP_FAIL;
    char url[512];
    snprintf(url, sizeof(url), "%s/api/states/%s", ha_url, entity_id);
    
    s_http_response_len = 0;
    memset(s_http_response_buffer, 0, sizeof(s_http_response_buffer));
    
    char auth_header[512];
    snprintf(auth_header, sizeof(auth_header), "Bearer %s", ha_token);
    
    esp_http_client_config_t config = {
        .url = url,
        .event_handler = http_event_handler,
        .timeout_ms = 10000,
    };
    
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        ESP_LOGE(TAG, "Failed to initialize HTTP client");
        return ESP_FAIL;
    }
    
    esp_http_client_set_header(client, "Authorization", auth_header);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    
    ESP_LOGD(TAG, "Performing HTTP GET to %s", url);
    err = esp_http_client_perform(client);
    
    if (err == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        if (status_code == 200) {
            // Debug: log the response for troubleshooting
            ESP_LOGD(TAG, "Response (%d bytes): %s", s_http_response_len, s_http_response_buffer);
            
            // Simple JSON parsing - extract "state" field value
            // Response format: {"entity_id":"sensor.date_time_iso","state":"2026-06-25T13:01:00",...}
            char *state_start = strstr(s_http_response_buffer, "\"state\":");
            if (state_start != NULL) {
                state_start += 9; // Skip "state":
                // Skip whitespace
                while (*state_start == ' ') state_start++;
                
                // Handle both quoted strings and unquoted values (numbers, etc.)
                char *state_end = NULL;
                bool is_quoted = (*state_start == '"');
                
                if (is_quoted) {
                    state_start++; // Skip opening quote
                    state_end = strchr(state_start, '"');
                } else {
                    // Unquoted value - find end (comma, brace, or bracket)
                    state_end = state_start;
                    while (*state_end != '\0' && *state_end != ',' && 
                           *state_end != '}' && *state_end != ']' && *state_end != '\n') {
                        state_end++;
                    }
                }
                
                if (state_end != NULL && state_end > state_start) {
                    size_t len = state_end - state_start;
                    if (len < state_len) {
                        memcpy(state_out, state_start, len);
                        state_out[len] = '\0';
                        err = ESP_OK;
                        ESP_LOGI(TAG, "Fetched %s: %s", entity_id, state_out);
                    } else {
                        ESP_LOGW(TAG, "State value too long (%zu bytes)", len);
                        err = ESP_FAIL;
                    }
                } else {
                    ESP_LOGW(TAG, "Malformed JSON - cannot find state value end");
                    err = ESP_FAIL;
                }
            } else {
                ESP_LOGW(TAG, "No 'state' field in response");
                ESP_LOGW(TAG, "Response snippet: %.200s...", s_http_response_buffer);
                err = ESP_FAIL;
            }
        } else if (status_code == 404) {
            ESP_LOGE(TAG, "Entity %s not found (404)", entity_id);
            err = ESP_ERR_NOT_FOUND;
        } else if (status_code == 401) {
            ESP_LOGE(TAG, "Unauthorized (401) - check token");
            err = ESP_ERR_INVALID_STATE;
        } else {
            ESP_LOGE(TAG, "HTTP error %d", status_code);
            err = ESP_FAIL;
        }
    } else {
        ESP_LOGE(TAG, "HTTP request failed: %s", esp_err_to_name(err));
    }
    
    esp_http_client_cleanup(client);
    return err;
}

void ha_client_task(void *pvParameters)
{
    ESP_LOGI(TAG, "HA client task starting on core %d", xPortGetCoreID());
    
    // Wait for WiFi and time sync
    ESP_LOGI(TAG, "Waiting for WiFi and time sync...");
    xEventGroupWaitBits(s_event_group,
                        WIFI_CONNECTED_BIT | TIME_SYNCED_BIT,
                        pdFALSE,
                        pdTRUE,
                        portMAX_DELAY);
    
    // Let system settle after time sync
    ESP_LOGI(TAG, "Waiting 2 seconds for system to stabilize...");
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    // Get HA credentials
    char ha_url[256] = {0};
    char ha_token[256] = {0};
    config_get_ha_credentials(ha_url, sizeof(ha_url), ha_token, sizeof(ha_token));
    
    uint32_t update_interval = config_get_update_interval();
    uint32_t grace_period = config_get_error_grace_period();
    
    ESP_LOGI(TAG, "Starting HA data fetch loop (interval: %lu ms / %lu seconds)", update_interval, update_interval / 1000);
    ESP_LOGI(TAG, "Free heap before loop: %lu bytes", esp_get_free_heap_size());
    
    while (1) {
        // Fetch datetime from single ISO entity (e.g., "2026-06-25T13:01:00")
        char datetime_iso[32] = {0};
        char date_str[32] = {0};
        char time_str[32] = {0};
        
        esp_err_t datetime_err = ESP_FAIL;
        
        // Log heap before fetch
        ESP_LOGI(TAG, "Free heap: %lu bytes (largest block: %lu)", 
                 esp_get_free_heap_size(), heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT));
        
        // Retry logic: 3 attempts with 5s delay
        for (int retry = 0; retry < 3; retry++) {
            if (datetime_err != ESP_OK) {
                ESP_LOGD(TAG, "Attempt %d/3: Fetching sensor.date_time_iso", retry + 1);
                datetime_err = ha_fetch_entity(ha_url, ha_token, "sensor.date_time_iso", datetime_iso, sizeof(datetime_iso));
                
                if (datetime_err == ESP_OK) {
                    // Parse ISO datetime: "2026-06-25T13:01:00" → date="2026-06-25", time="13:01"
                    char *t_separator = strchr(datetime_iso, 'T');
                    if (t_separator != NULL) {
                        // Extract date (everything before 'T')
                        size_t date_len = t_separator - datetime_iso;
                        if (date_len < sizeof(date_str)) {
                            memcpy(date_str, datetime_iso, date_len);
                            date_str[date_len] = '\0';
                        }
                        
                        // Extract time (HH:MM from after 'T')
                        char *time_start = t_separator + 1;
                        char *time_end = strchr(time_start, ':');
                        if (time_end != NULL) {
                            time_end = strchr(time_end + 1, ':'); // Find second colon (before seconds)
                            if (time_end != NULL) {
                                size_t time_len = time_end - time_start;
                                if (time_len < sizeof(time_str)) {
                                    memcpy(time_str, time_start, time_len);
                                    time_str[time_len] = '\0';
                                }
                            }
                        }
                        
                        ESP_LOGD(TAG, "Parsed ISO datetime: %s → date=%s, time=%s", datetime_iso, date_str, time_str);
                    } else {
                        ESP_LOGW(TAG, "Invalid ISO datetime format: %s", datetime_iso);
                        datetime_err = ESP_FAIL;
                    }
                }
            }
            
            if (datetime_err == ESP_OK && strlen(date_str) > 0 && strlen(time_str) > 0) {
                break;
            }
            
            if (retry < 2) {
                ESP_LOGW(TAG, "Retry %d/3 in 5 seconds...", retry + 1);
                vTaskDelay(pdMS_TO_TICKS(5000));
            }
        }
        
        // Update shared state
        if (xSemaphoreTake(dashboard_state_mutex, portMAX_DELAY) == pdTRUE) {
            if (datetime_err == ESP_OK && strlen(date_str) > 0 && strlen(time_str) > 0) {
                // Success
                strncpy(dashboard_state.date_str, date_str, sizeof(dashboard_state.date_str) - 1);
                strncpy(dashboard_state.time_str, time_str, sizeof(dashboard_state.time_str) - 1);
                time(&dashboard_state.last_successful_update);
                dashboard_state.failed_update_count = 0;
                dashboard_state.ha_connected = true;
                
                ESP_LOGI(TAG, "Updated: Date=%s, Time=%s", date_str, time_str);
                
                // Clear error bit
                xEventGroupClearBits(s_event_group, HA_ERROR_BIT);
            } else {
                // Failure
                dashboard_state.failed_update_count++;
                dashboard_state.ha_connected = false;
                
                ESP_LOGE(TAG, "Failed to fetch HA data (attempt %lu)", dashboard_state.failed_update_count);
                
                // Check if grace period exceeded (only if we had at least one successful update)
                if (dashboard_state.last_successful_update > 0) {
                    time_t now;
                    time(&now);
                    time_t time_since_last_success = now - dashboard_state.last_successful_update;
                    
                    // Convert grace_period from milliseconds to seconds
                    uint32_t grace_period_sec = grace_period / 1000;
                    
                    if (time_since_last_success > grace_period_sec) {
                        if (!(xEventGroupGetBits(s_event_group) & HA_ERROR_BIT)) {
                            ESP_LOGI(TAG, "Grace period exceeded (%ld > %lu seconds)", 
                                     time_since_last_success, grace_period_sec);
                            xEventGroupSetBits(s_event_group, HA_ERROR_BIT);
                        }
                    }
                }
            }
            
            xSemaphoreGive(dashboard_state_mutex);
        }
        
        // Sleep until next update (update_interval is already in milliseconds)
        vTaskDelay(pdMS_TO_TICKS(update_interval));
    }
}
