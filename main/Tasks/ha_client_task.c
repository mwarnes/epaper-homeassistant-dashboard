#include "ha_client_task.h"
#include "Shared/shared_resources.h"
#include "Shared/config_manager.h"
#include "eez_vars.h"
#include "ui/vars.h"
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

// Translate raw weather state to friendly name
static const char* translate_weather_state(const char *raw_state)
{
    if (strcmp(raw_state, "partlycloudy") == 0) return "Partly cloudy";
    if (strcmp(raw_state, "sunny") == 0) return "Sunny";
    if (strcmp(raw_state, "cloudy") == 0) return "Cloudy";
    if (strcmp(raw_state, "rainy") == 0) return "Rainy";
    if (strcmp(raw_state, "pouring") == 0) return "Pouring";
    if (strcmp(raw_state, "snowy") == 0) return "Snowy";
    if (strcmp(raw_state, "lightning") == 0) return "Lightning";
    if (strcmp(raw_state, "lightning-rainy") == 0) return "Lightning & Rain";
    if (strcmp(raw_state, "fog") == 0) return "Foggy";
    if (strcmp(raw_state, "windy") == 0) return "Windy";
    if (strcmp(raw_state, "clear-night") == 0) return "Clear night";
    if (strcmp(raw_state, "windy-variant") == 0) return "Windy";
    if (strcmp(raw_state, "hail") == 0) return "Hail";
    if (strcmp(raw_state, "snowy-rainy") == 0) return "Snow & Rain";
    if (strcmp(raw_state, "exceptional") == 0) return "Exceptional";
    
    // If unknown, return raw state (fallback)
    return raw_state;
}

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
                        
                        // Strip any trailing quotes that might have been included
                        size_t final_len = strlen(state_out);
                        if (final_len > 0 && state_out[final_len - 1] == '"') {
                            state_out[final_len - 1] = '\0';
                            ESP_LOGD(TAG, "Stripped trailing quote from state value");
                        }
                        
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
    
    // Wait for WiFi connection
    ESP_LOGI(TAG, "Waiting for WiFi connection...");
    xEventGroupWaitBits(s_event_group,
                        WIFI_CONNECTED_BIT,
                        pdFALSE,
                        pdFALSE,
                        portMAX_DELAY);
    
    // Let WiFi settle before starting HTTP requests
    ESP_LOGI(TAG, "WiFi connected, waiting 2 seconds before starting HA fetch...");
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
        
        // Additional HA entities
        char realfeel_temperature[32] = {0};
        char condition_today[64] = {0};
        
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
                    // Parse ISO datetime: "2026-06-25T13:01:00" → date="Sunday Jun 27, 2026", time="13:01"
                    char *t_separator = strchr(datetime_iso, 'T');
                    if (t_separator != NULL) {
                        // Extract ISO date (everything before 'T') into temp buffer
                        char iso_date[16] = {0};
                        size_t date_len = t_separator - datetime_iso;
                        if (date_len < sizeof(iso_date)) {
                            memcpy(iso_date, datetime_iso, date_len);
                            iso_date[date_len] = '\0';
                            
                            // Parse ISO date "2026-06-25" into struct tm
                            struct tm timeinfo = {0};
                            if (strptime(iso_date, "%Y-%m-%d", &timeinfo) != NULL) {
                                // Format as "Sunday Jun 27, 2026"
                                strftime(date_str, sizeof(date_str), "%A %b %d, %Y", &timeinfo);
                            } else {
                                // Fallback: use ISO date if parsing fails
                                strncpy(date_str, iso_date, sizeof(date_str) - 1);
                                ESP_LOGW(TAG, "Failed to parse date, using ISO format");
                            }
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
        
        // Fetch additional entities (temperature and weather condition)
        esp_err_t temp_err = ESP_FAIL;
        esp_err_t condition_err = ESP_FAIL;
        
        // Fetch realfeel temperature
        // TODO: Update entity_id to match your actual HA sensor name
        // Common patterns: "sensor.home_realfeel_temperature", "sensor.realfeel_temp", etc.
        temp_err = ha_fetch_entity(ha_url, ha_token, "sensor.home_realfeel_temperature", 
                                    realfeel_temperature, sizeof(realfeel_temperature));
        
        if (temp_err == ESP_OK && strlen(realfeel_temperature) > 0) {
            // Add °C unit for main/current temperature display (no space)
            char temp_with_unit[40];
            snprintf(temp_with_unit, sizeof(temp_with_unit), "%s°C", realfeel_temperature);
            set_var_ha_home_realfeel_temperature(temp_with_unit);
            ESP_LOGI(TAG, "Temperature: %s", temp_with_unit);
        } else {
            ESP_LOGW(TAG, "Failed to fetch temperature sensor");
        }
        
        // Fetch weather condition from weather entity
        // Entity: weather.forecast_langebaan
        // State values: sunny, cloudy, rainy, partlycloudy, etc.
        condition_err = ha_fetch_entity(ha_url, ha_token, "weather.forecast_langebaan",
                                        condition_today, sizeof(condition_today));
        
        if (condition_err == ESP_OK && strlen(condition_today) > 0) {
            // Translate raw state (e.g., "partlycloudy" -> "Partly cloudy")
            const char *translated_condition = translate_weather_state(condition_today);
            set_var_ha_home_condition_today(translated_condition);
            ESP_LOGI(TAG, "Weather condition: %s (raw: %s)", translated_condition, condition_today);
        } else {
            ESP_LOGW(TAG, "Failed to fetch weather condition sensor");
        }
        
        // ============================================
        // Fetch 5-Day Forecast Data
        // ============================================
        // From HA template sensors created in HA_FORECAST_SENSORS.yaml
        
        char forecast_temp[20], forecast_condition[50], forecast_name[10];
        esp_err_t f_err;
        
        // Day 1 Forecast (Note: HA normalizes to day_1, not day1)
        f_err = ha_fetch_entity(ha_url, ha_token, "sensor.forecast_day_1_temperature", forecast_temp, sizeof(forecast_temp));
        if (f_err == ESP_OK) {
            set_var_ha_forecast_day1_temp(forecast_temp);  // Just numeric value
        }
        
        f_err = ha_fetch_entity(ha_url, ha_token, "sensor.forecast_day_1_condition", forecast_condition, sizeof(forecast_condition));
        if (f_err == ESP_OK) {
            const char *translated = translate_weather_state(forecast_condition);
            set_var_ha_forecast_day1_condition(translated);
        }
        
        f_err = ha_fetch_entity(ha_url, ha_token, "sensor.forecast_day_1_name", forecast_name, sizeof(forecast_name));
        if (f_err == ESP_OK) {
            set_var_ha_forecast_day1_name(forecast_name);
        }
        
        // Day 2 Forecast
        f_err = ha_fetch_entity(ha_url, ha_token, "sensor.forecast_day_2_temperature", forecast_temp, sizeof(forecast_temp));
        if (f_err == ESP_OK) {
            set_var_ha_forecast_day2_temp(forecast_temp);  // Just numeric value
        }
        
        f_err = ha_fetch_entity(ha_url, ha_token, "sensor.forecast_day_2_condition", forecast_condition, sizeof(forecast_condition));
        if (f_err == ESP_OK) {
            const char *translated = translate_weather_state(forecast_condition);
            set_var_ha_forecast_day2_condition(translated);
        }
        
        f_err = ha_fetch_entity(ha_url, ha_token, "sensor.forecast_day_2_name", forecast_name, sizeof(forecast_name));
        if (f_err == ESP_OK) {
            set_var_ha_forecast_day2_name(forecast_name);
        }
        
        // Day 3 Forecast
        f_err = ha_fetch_entity(ha_url, ha_token, "sensor.forecast_day_3_temperature", forecast_temp, sizeof(forecast_temp));
        if (f_err == ESP_OK) {
            set_var_ha_forecast_day3_temp(forecast_temp);  // Just numeric value
        }
        
        f_err = ha_fetch_entity(ha_url, ha_token, "sensor.forecast_day_3_condition", forecast_condition, sizeof(forecast_condition));
        if (f_err == ESP_OK) {
            const char *translated = translate_weather_state(forecast_condition);
            set_var_ha_forecast_day3_condition(translated);
        }
        
        f_err = ha_fetch_entity(ha_url, ha_token, "sensor.forecast_day_3_name", forecast_name, sizeof(forecast_name));
        if (f_err == ESP_OK) {
            set_var_ha_forecast_day3_name(forecast_name);
        }
        
        // Day 4 Forecast
        f_err = ha_fetch_entity(ha_url, ha_token, "sensor.forecast_day_4_temperature", forecast_temp, sizeof(forecast_temp));
        if (f_err == ESP_OK) {
            set_var_ha_forecast_day4_temp(forecast_temp);  // Just numeric value
        }
        
        f_err = ha_fetch_entity(ha_url, ha_token, "sensor.forecast_day_4_condition", forecast_condition, sizeof(forecast_condition));
        if (f_err == ESP_OK) {
            const char *translated = translate_weather_state(forecast_condition);
            set_var_ha_forecast_day4_condition(translated);
        }
        
        f_err = ha_fetch_entity(ha_url, ha_token, "sensor.forecast_day_4_name", forecast_name, sizeof(forecast_name));
        if (f_err == ESP_OK) {
            set_var_ha_forecast_day4_name(forecast_name);
        }
        
        // Day 5 Forecast
        f_err = ha_fetch_entity(ha_url, ha_token, "sensor.forecast_day_5_temperature", forecast_temp, sizeof(forecast_temp));
        if (f_err == ESP_OK) {
            set_var_ha_forecast_day5_temp(forecast_temp);  // Just numeric value
        }
        
        f_err = ha_fetch_entity(ha_url, ha_token, "sensor.forecast_day_5_condition", forecast_condition, sizeof(forecast_condition));
        if (f_err == ESP_OK) {
            const char *translated = translate_weather_state(forecast_condition);
            set_var_ha_forecast_day5_condition(translated);
        }
        
        f_err = ha_fetch_entity(ha_url, ha_token, "sensor.forecast_day_5_name", forecast_name, sizeof(forecast_name));
        if (f_err == ESP_OK) {
            set_var_ha_forecast_day5_name(forecast_name);
        }
        
        ESP_LOGI(TAG, "Forecast data updated");
        
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
