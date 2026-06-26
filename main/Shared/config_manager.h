#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include "esp_err.h"
#include <stdint.h>
#include <stddef.h>

// Initialize configuration manager (loads from NVS or uses defaults)
esp_err_t config_init(void);

// Get WiFi credentials
esp_err_t config_get_wifi(char *ssid, size_t ssid_len, char *password, size_t pass_len);

// Get Home Assistant credentials
esp_err_t config_get_ha_credentials(char *url, size_t url_len, char *token, size_t token_len);

// Get update interval (seconds)
uint32_t config_get_update_interval(void);

// Get error grace period (seconds)
uint32_t config_get_error_grace_period(void);

// Get power mode (0=always-on, 1=deep-sleep, 2=light-sleep)
uint8_t config_get_power_mode(void);

// Get timezone string
esp_err_t config_get_timezone(char *tz, size_t len);

#endif // CONFIG_MANAGER_H
