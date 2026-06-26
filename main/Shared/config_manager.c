#include "config_manager.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <string.h>

static const char *TAG = "config_manager";
static const char *NVS_NAMESPACE = "ha_dashboard";

// Default values from Kconfig
#ifndef CONFIG_HA_DASHBOARD_WIFI_SSID
#define CONFIG_HA_DASHBOARD_WIFI_SSID "myssid"
#endif

#ifndef CONFIG_HA_DASHBOARD_WIFI_PASSWORD
#define CONFIG_HA_DASHBOARD_WIFI_PASSWORD "mypassword"
#endif

#ifndef CONFIG_HA_DASHBOARD_HA_URL
#define CONFIG_HA_DASHBOARD_HA_URL "http://homeassistant.local:8123"
#endif

#ifndef CONFIG_HA_DASHBOARD_HA_TOKEN
#define CONFIG_HA_DASHBOARD_HA_TOKEN ""
#endif

#ifndef CONFIG_HA_DASHBOARD_UPDATE_INTERVAL
#define CONFIG_HA_DASHBOARD_UPDATE_INTERVAL 600
#endif

#ifndef CONFIG_HA_DASHBOARD_ERROR_GRACE_PERIOD
#define CONFIG_HA_DASHBOARD_ERROR_GRACE_PERIOD 1800
#endif

#ifndef CONFIG_HA_DASHBOARD_TIMEZONE
#define CONFIG_HA_DASHBOARD_TIMEZONE "UTC"
#endif

// Cached configuration
static struct {
    char wifi_ssid[64];
    char wifi_password[64];
    char ha_url[256];
    char ha_token[256];
    char timezone[64];
    uint32_t update_interval;
    uint32_t error_grace_period;
    uint8_t power_mode;
} s_config;

esp_err_t config_init(void)
{
    ESP_LOGI(TAG, "Initializing configuration...");
    
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs_handle);
    
    if (err == ESP_OK) {
        // Load from NVS
        size_t len;
        
        // WiFi SSID
        len = sizeof(s_config.wifi_ssid);
        if (nvs_get_str(nvs_handle, "wifi_ssid", s_config.wifi_ssid, &len) != ESP_OK) {
            strncpy(s_config.wifi_ssid, CONFIG_HA_DASHBOARD_WIFI_SSID, sizeof(s_config.wifi_ssid) - 1);
        }
        
        // WiFi Password
        len = sizeof(s_config.wifi_password);
        if (nvs_get_str(nvs_handle, "wifi_pass", s_config.wifi_password, &len) != ESP_OK) {
            strncpy(s_config.wifi_password, CONFIG_HA_DASHBOARD_WIFI_PASSWORD, sizeof(s_config.wifi_password) - 1);
        }
        
        // HA URL
        len = sizeof(s_config.ha_url);
        if (nvs_get_str(nvs_handle, "ha_url", s_config.ha_url, &len) != ESP_OK) {
            strncpy(s_config.ha_url, CONFIG_HA_DASHBOARD_HA_URL, sizeof(s_config.ha_url) - 1);
        }
        
        // HA Token
        len = sizeof(s_config.ha_token);
        if (nvs_get_str(nvs_handle, "ha_token", s_config.ha_token, &len) != ESP_OK) {
            strncpy(s_config.ha_token, CONFIG_HA_DASHBOARD_HA_TOKEN, sizeof(s_config.ha_token) - 1);
        }
        
        // Update interval (convert seconds to milliseconds)
        if (nvs_get_u32(nvs_handle, "update_interval", &s_config.update_interval) != ESP_OK) {
            s_config.update_interval = CONFIG_HA_DASHBOARD_UPDATE_INTERVAL * 1000;
        }
        
        // Error grace period (convert seconds to milliseconds)
        if (nvs_get_u32(nvs_handle, "error_grace_period", &s_config.error_grace_period) != ESP_OK) {
            s_config.error_grace_period = CONFIG_HA_DASHBOARD_ERROR_GRACE_PERIOD * 1000;
        }
        
        // Power mode
        if (nvs_get_u8(nvs_handle, "power_mode", &s_config.power_mode) != ESP_OK) {
            s_config.power_mode = 0; // Default: always-on
        }
        
        // Timezone
        len = sizeof(s_config.timezone);
        if (nvs_get_str(nvs_handle, "timezone", s_config.timezone, &len) != ESP_OK) {
            strncpy(s_config.timezone, CONFIG_HA_DASHBOARD_TIMEZONE, sizeof(s_config.timezone) - 1);
        }
        
        nvs_close(nvs_handle);
    } else {
        // NVS not initialized or empty, use Kconfig defaults
        ESP_LOGW(TAG, "NVS not available, using Kconfig defaults");
        strncpy(s_config.wifi_ssid, CONFIG_HA_DASHBOARD_WIFI_SSID, sizeof(s_config.wifi_ssid) - 1);
        strncpy(s_config.wifi_password, CONFIG_HA_DASHBOARD_WIFI_PASSWORD, sizeof(s_config.wifi_password) - 1);
        strncpy(s_config.ha_url, CONFIG_HA_DASHBOARD_HA_URL, sizeof(s_config.ha_url) - 1);
        strncpy(s_config.ha_token, CONFIG_HA_DASHBOARD_HA_TOKEN, sizeof(s_config.ha_token) - 1);
        strncpy(s_config.timezone, CONFIG_HA_DASHBOARD_TIMEZONE, sizeof(s_config.timezone) - 1);
        s_config.update_interval = CONFIG_HA_DASHBOARD_UPDATE_INTERVAL * 1000; // seconds to ms
        s_config.error_grace_period = CONFIG_HA_DASHBOARD_ERROR_GRACE_PERIOD * 1000; // seconds to ms
        s_config.power_mode = 0;
    }
    
    // Validate required fields
    if (strlen(s_config.wifi_ssid) == 0) {
        ESP_LOGE(TAG, "WiFi SSID not configured!");
    }
    if (strlen(s_config.ha_url) == 0) {
        ESP_LOGE(TAG, "Home Assistant URL not configured!");
    }
    if (strlen(s_config.ha_token) == 0) {
        ESP_LOGW(TAG, "Home Assistant token not configured!");
    }
    
    ESP_LOGI(TAG, "Configuration loaded:");
    ESP_LOGI(TAG, "  WiFi SSID: %s", s_config.wifi_ssid);
    ESP_LOGI(TAG, "  HA URL: %s", s_config.ha_url);
    ESP_LOGI(TAG, "  Update interval: %lu ms (%lu sec)", s_config.update_interval, s_config.update_interval / 1000);
    ESP_LOGI(TAG, "  Error grace period: %lu ms (%lu sec)", s_config.error_grace_period, s_config.error_grace_period / 1000);
    ESP_LOGI(TAG, "  Power mode: %d", s_config.power_mode);
    ESP_LOGI(TAG, "  Timezone: %s", s_config.timezone);
    
    return ESP_OK;
}

esp_err_t config_get_timezone(char *tz, size_t len)
{
    if (!tz) {
        return ESP_ERR_INVALID_ARG;
    }
    
    strncpy(tz, s_config.timezone, len - 1);
    tz[len - 1] = '\0';
    
    return ESP_OK;
}

esp_err_t config_get_wifi(char *ssid, size_t ssid_len, char *password, size_t pass_len)
{
    if (!ssid || !password) {
        return ESP_ERR_INVALID_ARG;
    }
    
    strncpy(ssid, s_config.wifi_ssid, ssid_len - 1);
    ssid[ssid_len - 1] = '\0';
    
    strncpy(password, s_config.wifi_password, pass_len - 1);
    password[pass_len - 1] = '\0';
    
    return ESP_OK;
}

esp_err_t config_get_ha_credentials(char *url, size_t url_len, char *token, size_t token_len)
{
    if (!url || !token) {
        return ESP_ERR_INVALID_ARG;
    }
    
    strncpy(url, s_config.ha_url, url_len - 1);
    url[url_len - 1] = '\0';
    
    strncpy(token, s_config.ha_token, token_len - 1);
    token[token_len - 1] = '\0';
    
    return ESP_OK;
}

uint32_t config_get_update_interval(void)
{
    return s_config.update_interval;
}

uint32_t config_get_error_grace_period(void)
{
    return s_config.error_grace_period;
}

uint8_t config_get_power_mode(void)
{
    return s_config.power_mode;
}
