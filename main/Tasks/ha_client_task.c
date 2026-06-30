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

// Convert wind bearing (degrees) to cardinal direction
static const char* bearing_to_direction(int bearing)
{
    // Normalize bearing to 0-359 range
    bearing = bearing % 360;
    if (bearing < 0) bearing += 360;
    
    // 16-point compass rose
    if (bearing < 11 || bearing >= 349) return "N";
    if (bearing < 34) return "NNE";
    if (bearing < 56) return "NE";
    if (bearing < 79) return "ENE";
    if (bearing < 101) return "E";
    if (bearing < 124) return "ESE";
    if (bearing < 146) return "SE";
    if (bearing < 169) return "SSE";
    if (bearing < 191) return "S";
    if (bearing < 214) return "SSW";
    if (bearing < 236) return "SW";
    if (bearing < 259) return "WSW";
    if (bearing < 281) return "W";
    if (bearing < 304) return "WNW";
    if (bearing < 326) return "NW";
    if (bearing < 349) return "NNW";
    
    return "N";  // Fallback
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

// Fetch an attribute value from an entity (e.g., temperature from weather entity)
static esp_err_t ha_fetch_entity_attribute(const char *ha_url, const char *ha_token,
                                            const char *entity_id, const char *attribute_name,
                                            char *value_out, size_t value_len)
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
    esp_http_client_set_header(client, "Authorization", auth_header);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    
    err = esp_http_client_perform(client);
    
    if (err == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        if (status_code == 200) {
            ESP_LOGD(TAG, "Response (%d bytes): %s", s_http_response_len, s_http_response_buffer);
            
            // Find "attributes" object
            char *attr_start = strstr(s_http_response_buffer, "\"attributes\":");
            if (attr_start != NULL) {
                // Find the specific attribute within attributes
                char search_pattern[128];
                snprintf(search_pattern, sizeof(search_pattern), "\"%s\":", attribute_name);
                char *value_start = strstr(attr_start, search_pattern);
                
                if (value_start != NULL) {
                    value_start += strlen(search_pattern);
                    // Skip whitespace
                    while (*value_start == ' ') value_start++;
                    
                    // Handle both quoted strings and unquoted values (numbers)
                    char *value_end = NULL;
                    bool is_quoted = (*value_start == '"');
                    
                    if (is_quoted) {
                        value_start++; // Skip opening quote
                        value_end = strchr(value_start, '"');
                    } else {
                        // Unquoted value - find end (comma, brace, or bracket)
                        value_end = value_start;
                        while (*value_end != '\0' && *value_end != ',' &&
                               *value_end != '}' && *value_end != ']' && *value_end != '\n') {
                            value_end++;
                        }
                    }
                    
                    if (value_end != NULL && value_end > value_start) {
                        size_t len = value_end - value_start;
                        if (len < value_len) {
                            memcpy(value_out, value_start, len);
                            value_out[len] = '\0';
                            
                            // Strip any trailing quotes
                            size_t final_len = strlen(value_out);
                            if (final_len > 0 && value_out[final_len - 1] == '"') {
                                value_out[final_len - 1] = '\0';
                            }
                            
                            err = ESP_OK;
                            ESP_LOGI(TAG, "Fetched %s.%s: %s", entity_id, attribute_name, value_out);
                        } else {
                            ESP_LOGW(TAG, "Attribute value too long (%zu bytes)", len);
                            err = ESP_FAIL;
                        }
                    } else {
                        ESP_LOGW(TAG, "Malformed JSON - cannot find attribute value end");
                        err = ESP_FAIL;
                    }
                } else {
                    ESP_LOGW(TAG, "Attribute '%s' not found in entity %s", attribute_name, entity_id);
                    err = ESP_FAIL;
                }
            } else {
                ESP_LOGW(TAG, "No 'attributes' field in response");
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
        
        // Fetch current weather from weather entity
        // Entity: weather.forecast_langebaan
        // State = condition (sunny, cloudy, rainy, partlycloudy, etc.)
        // Attributes include temperature, humidity, etc.
        esp_err_t temp_err = ESP_FAIL;
        esp_err_t condition_err = ESP_FAIL;
        
        // Fetch current temperature from weather entity's temperature attribute
        temp_err = ha_fetch_entity_attribute(ha_url, ha_token, "weather.forecast_langebaan",
                                             "temperature", realfeel_temperature, sizeof(realfeel_temperature));
        
        if (temp_err == ESP_OK && strlen(realfeel_temperature) > 0) {
            // Add °C unit for main/current temperature display (no space)
            char temp_with_unit[40];
            snprintf(temp_with_unit, sizeof(temp_with_unit), "%s°C", realfeel_temperature);
            set_var_ha_home_realfeel_temperature(temp_with_unit);
            ESP_LOGI(TAG, "Current temperature: %s", temp_with_unit);
        } else {
            ESP_LOGW(TAG, "Failed to fetch temperature from weather entity");
        }
        
        // Fetch current weather condition (state of weather entity)
        condition_err = ha_fetch_entity(ha_url, ha_token, "weather.forecast_langebaan",
                                        condition_today, sizeof(condition_today));
        
        if (condition_err == ESP_OK && strlen(condition_today) > 0) {
            // Translate raw state (e.g., "partlycloudy" -> "Partly cloudy")
            const char *translated_condition = translate_weather_state(condition_today);
            set_var_ha_home_condition_today(translated_condition);
            ESP_LOGI(TAG, "Current condition: %s (raw: %s)", translated_condition, condition_today);
        } else {
            ESP_LOGW(TAG, "Failed to fetch weather condition");
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
        
        // ============================================
        // Fetch Additional Weather Data
        // ============================================
        
        // Fetch wind speed and bearing from weather entity
        char wind_speed[20], wind_bearing[20];
        esp_err_t wind_speed_err = ha_fetch_entity_attribute(ha_url, ha_token, "weather.forecast_langebaan",
                                                             "wind_speed", wind_speed, sizeof(wind_speed));
        esp_err_t wind_bearing_err = ha_fetch_entity_attribute(ha_url, ha_token, "weather.forecast_langebaan",
                                                               "wind_bearing", wind_bearing, sizeof(wind_bearing));
        
        if (wind_speed_err == ESP_OK && wind_bearing_err == ESP_OK) {
            // Combine speed and direction: "15 km/h NE"
            int bearing = atoi(wind_bearing);
            const char *direction = bearing_to_direction(bearing);
            char wind_combined[50];
            snprintf(wind_combined, sizeof(wind_combined), "%s km/h %s", wind_speed, direction);
            set_var_ha_wind(wind_combined);
            ESP_LOGI(TAG, "Wind: %s", wind_combined);
        } else if (wind_speed_err == ESP_OK) {
            // Only speed available - add km/h unit
            char wind_formatted[30];
            snprintf(wind_formatted, sizeof(wind_formatted), "%s km/h", wind_speed);
            set_var_ha_wind(wind_formatted);
            ESP_LOGI(TAG, "Wind: %s", wind_formatted);
        }
        
        // Fetch humidity from weather entity
        char humidity[20];
        esp_err_t humidity_err = ha_fetch_entity_attribute(ha_url, ha_token, "weather.forecast_langebaan",
                                                           "humidity", humidity, sizeof(humidity));
        if (humidity_err == ESP_OK) {
            // Add % symbol: "65%"
            char humidity_formatted[30];
            snprintf(humidity_formatted, sizeof(humidity_formatted), "%s%%", humidity);
            set_var_ha_humidity(humidity_formatted);
            ESP_LOGI(TAG, "Humidity: %s", humidity_formatted);
        }
        
        // Fetch PM2.5 from air quality sensor
        char pm25[20];
        esp_err_t pm25_err = ha_fetch_entity(ha_url, ha_token, "sensor.airquality_office_pms5003_pm2_5",
                                            pm25, sizeof(pm25));
        if (pm25_err == ESP_OK) {
            // Format with unit: "12 µg/m³" (µ = U+00B5, ³ = U+00B3)
            char pm25_formatted[30];
            snprintf(pm25_formatted, sizeof(pm25_formatted), "%s µg/m³", pm25);
            set_var_ha_air_quality_pm25(pm25_formatted);
            ESP_LOGI(TAG, "PM2.5: %s", pm25_formatted);
        } else {
            ESP_LOGW(TAG, "Failed to fetch PM2.5 sensor");
        }
        
        // ============================================
        // Fetch Power & Energy Data
        // ============================================
        
        // Fetch current power values (W)
        char house_power[20], pv_power[20], battery_power[20], grid_power[20];
        char battery_soc[20], grid_connected[20];  // Increased size to handle "unavailable"
        
        // House load power
        esp_err_t house_power_err = ha_fetch_entity(ha_url, ha_token, "sensor.deye_sunsynk_sol_ark_x_2_load_power_2",
                                                    house_power, sizeof(house_power));
        if (house_power_err == ESP_OK) {
            char house_power_formatted[30];
            snprintf(house_power_formatted, sizeof(house_power_formatted), "%s W", house_power);
            set_var_ha_house_power(house_power_formatted);
            ESP_LOGI(TAG, "House power: %s", house_power_formatted);
        }
        
        // PV generation power
        esp_err_t pv_power_err = ha_fetch_entity(ha_url, ha_token, "sensor.deye_sunsynk_sol_ark_x_2_pv_power_2",
                                                pv_power, sizeof(pv_power));
        if (pv_power_err == ESP_OK) {
            char pv_power_formatted[30];
            snprintf(pv_power_formatted, sizeof(pv_power_formatted), "%s W", pv_power);
            set_var_ha_pv_power(pv_power_formatted);
            ESP_LOGI(TAG, "PV power: %s", pv_power_formatted);
            
            // Update PV icon based on power (day/night)
            float pv_power_val = atof(pv_power);
            eez_update_pv_icon(pv_power_val);
        }
        
        // Battery power (with sign: - = charging, + = discharging)
        esp_err_t battery_power_err = ha_fetch_entity(ha_url, ha_token, "sensor.deye_sunsynk_sol_ark_x_2_battery_power_2",
                                                     battery_power, sizeof(battery_power));
        if (battery_power_err == ESP_OK) {
            char battery_power_formatted[30];
            float battery_power_val = atof(battery_power);
            
            // Format with explicit sign when charging (negative value)
            if (battery_power_val < 0) {
                snprintf(battery_power_formatted, sizeof(battery_power_formatted), "%.0f W", battery_power_val);
            } else {
                snprintf(battery_power_formatted, sizeof(battery_power_formatted), "%.0f W", battery_power_val);
            }
            
            set_var_ha_battery_power(battery_power_formatted);
            ESP_LOGI(TAG, "Battery power: %s%s", battery_power_formatted, 
                     battery_power_val < 0 ? " (charging)" : " (discharging)");
        }
        
        // Battery state of charge (for icon selection and display)
        esp_err_t battery_soc_err = ha_fetch_entity(ha_url, ha_token, "sensor.deye_sunsynk_sol_ark_x_2_battery_state_of_charge_2",
                                                   battery_soc, sizeof(battery_soc));
        if (battery_soc_err == ESP_OK) {
            int soc = atoi(battery_soc);
            eez_update_battery_icon(soc);
            
            // Display SOC percentage in the "day" label position
            char battery_soc_formatted[20];
            snprintf(battery_soc_formatted, sizeof(battery_soc_formatted), "%d%%", soc);
            set_var_ha_battery_power_day(battery_soc_formatted);
            
            ESP_LOGI(TAG, "Battery SOC: %s", battery_soc_formatted);
        }
        
        // Grid power
        esp_err_t grid_power_err = ha_fetch_entity(ha_url, ha_token, "sensor.deye_sunsynk_sol_ark_x_2_inverter_2_grid_power_2",
                                                  grid_power, sizeof(grid_power));
        if (grid_power_err == ESP_OK) {
            char grid_power_formatted[30];
            snprintf(grid_power_formatted, sizeof(grid_power_formatted), "%s W", grid_power);
            set_var_ha_grid_power(grid_power_formatted);
            ESP_LOGI(TAG, "Grid power: %s", grid_power_formatted);
        }
        
        // Grid connected status
        esp_err_t grid_connected_err = ha_fetch_entity(ha_url, ha_token, "binary_sensor.grid_connected",
                                                      grid_connected, sizeof(grid_connected));
        if (grid_connected_err == ESP_OK) {
            bool is_connected = (strcmp(grid_connected, "on") == 0);
            eez_update_grid_icon(is_connected);
            ESP_LOGI(TAG, "Grid connected: %s", is_connected ? "YES" : "NO");
        }
        
        // ============================================
        // Fetch Daily Energy Data (kWh)
        // ============================================
        
        char house_energy[20], pv_energy[20], battery_energy[20], grid_energy[20];
        
        // House daily consumption
        esp_err_t house_energy_err = ha_fetch_entity(ha_url, ha_token, "sensor.load_energy_daily",
                                                     house_energy, sizeof(house_energy));
        if (house_energy_err == ESP_OK) {
            char house_energy_formatted[30];
            snprintf(house_energy_formatted, sizeof(house_energy_formatted), "%s kWh", house_energy);
            set_var_ha_house_power_day(house_energy_formatted);
            ESP_LOGI(TAG, "House energy today: %s", house_energy_formatted);
        }
        
        // PV daily generation
        esp_err_t pv_energy_err = ha_fetch_entity(ha_url, ha_token, "sensor.pv_energy_daily",
                                                 pv_energy, sizeof(pv_energy));
        if (pv_energy_err == ESP_OK) {
            char pv_energy_formatted[30];
            snprintf(pv_energy_formatted, sizeof(pv_energy_formatted), "%s kWh", pv_energy);
            set_var_ha_pv_power_day(pv_energy_formatted);
            ESP_LOGI(TAG, "PV energy today: %s", pv_energy_formatted);
        }
        
        // Battery SOC is displayed instead of daily energy (see above where SOC is fetched)
        // The ha_battery_power_day variable now shows "65%" instead of "5.2 kWh"
        
        // Grid daily energy (using import for display)
        esp_err_t grid_energy_err = ha_fetch_entity(ha_url, ha_token, "sensor.grid_energy_daily",
                                                   grid_energy, sizeof(grid_energy));
        if (grid_energy_err == ESP_OK) {
            char grid_energy_formatted[30];
            snprintf(grid_energy_formatted, sizeof(grid_energy_formatted), "%s kWh", grid_energy);
            set_var_ha_grid_power_day(grid_energy_formatted);
            ESP_LOGI(TAG, "Grid energy today: %s", grid_energy_formatted);
        }
        
        ESP_LOGI(TAG, "Power & energy data updated");
        
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
