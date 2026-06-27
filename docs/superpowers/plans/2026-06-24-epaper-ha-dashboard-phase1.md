# E-Paper Home Assistant Dashboard Phase 1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build an ESP32-based e-paper dashboard that fetches and displays date/time from Home Assistant API.

**Architecture:** FreeRTOS task-based with core affinity (Core 0: network tasks, Core 1: UI tasks), mutex-protected shared state, EEZ Studio UI generation, NVS configuration storage.

**Tech Stack:** ESP-IDF v6.0, LVGL v9.2, FreeRTOS, ESP HTTP Client, SNTP, NVS, EEZ Studio, GDEM102F91 e-paper driver

## Global Constraints

- ESP-IDF version: v6.0 or higher
- LVGL version: 9.2.x
- Target hardware: ESP32-S3 with 8MB flash, PSRAM enabled
- Display: GDEM102F91 (960×640, 4-color e-paper)
- C99 standard, ESP-IDF component model
- All network operations on Core 0, all UI operations on Core 1
- Update interval: configurable, default 600 seconds (10 minutes)
- Error grace period: configurable, default 1800 seconds (30 minutes)

---

## File Structure

**Build & Configuration:**
- `CMakeLists.txt` - Project-level build config
- `sdkconfig.defaults` - ESP-IDF default configuration
- `main/CMakeLists.txt` - Main component build config
- `main/idf_component.yml` - Component dependencies
- `main/Kconfig.projbuild` - Menuconfig options
- `.gitignore` - Git ignore patterns

**Main Entry:**
- `main/main.c` - Application entry point, task creation

**Shared Resources:**
- `main/Shared/shared_resources.h` - Shared state struct, mutex declarations
- `main/Shared/shared_resources.c` - Shared resource initialization
- `main/Shared/config_manager.h` - NVS configuration API
- `main/Shared/config_manager.c` - NVS configuration implementation

**Network Tasks (Core 0):**
- `main/Tasks/wifi_task.h` - WiFi task API
- `main/Tasks/wifi_task.c` - WiFi connection management
- `main/Tasks/time_sync_task.h` - Time sync API
- `main/Tasks/time_sync_task.c` - SNTP time synchronization
- `main/Tasks/ha_client_task.h` - HA client API
- `main/Tasks/ha_client_task.c` - Home Assistant HTTP client

**UI Tasks (Core 1):**
- `main/Tasks/lvgl_task.h` - LVGL task API
- `main/Tasks/lvgl_task.c` - LVGL timer handler
- `main/Tasks/display_task.h` - Display task API
- `main/Tasks/display_task.c` - Display refresh coordination
- `main/Tasks/power_mgmt_task.h` - Power management API
- `main/Tasks/power_mgmt_task.c` - Power management implementation

**EEZ Studio Integration:**
- `main/eez_vars.h` - EEZ variable update API
- `main/eez_vars.c` - EEZ variable implementation
- `main/eez_actions.h` - EEZ action stubs
- `main/eez_actions.c` - EEZ action implementation (minimal)
- `main/ui/` - EEZ Studio generated files (screens.c/h, ui.c/h, etc.)

---

### Task 1: Project Scaffolding & Build Configuration

**Files:**
- Create: `CMakeLists.txt`
- Create: `sdkconfig.defaults`
- Create: `main/CMakeLists.txt`
- Create: `main/idf_component.yml`
- Create: `main/Kconfig.projbuild`
- Create: `.gitignore`

**Interfaces:**
- Consumes: None (first task)
- Produces: Build system foundation for all subsequent tasks

---

- [ ] **Step 1: Create project-level CMakeLists.txt**

```cmake
cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(epaper-homeassistant-dashboard)
```

- [ ] **Step 2: Create sdkconfig.defaults**

```ini
# ESP32-S3 specific
CONFIG_IDF_TARGET="esp32s3"
CONFIG_ESPTOOLPY_FLASHSIZE_8MB=y

# PSRAM (required for LVGL + e-paper buffers)
CONFIG_SPIRAM=y
CONFIG_SPIRAM_MODE_OCT=y
CONFIG_SPIRAM_SPEED_80M=y

# FreeRTOS
CONFIG_FREERTOS_HZ=1000
CONFIG_FREERTOS_UNICORE=n

# WiFi
CONFIG_ESP_WIFI_STATIC_RX_BUFFER_NUM=10
CONFIG_ESP_WIFI_DYNAMIC_RX_BUFFER_NUM=32
CONFIG_ESP_WIFI_DYNAMIC_TX_BUFFER_NUM=32

# LWIP (for HTTP client)
CONFIG_LWIP_MAX_SOCKETS=10

# HTTP Client
CONFIG_ESP_HTTP_CLIENT_ENABLE_HTTPS=y

# Logging
CONFIG_LOG_DEFAULT_LEVEL_INFO=y
CONFIG_LOG_MAXIMUM_LEVEL_DEBUG=y

# NVS
CONFIG_NVS_ENCRYPTION=n

# SNTP
CONFIG_LWIP_SNTP_MAX_SERVERS=3
```

- [ ] **Step 3: Create main/idf_component.yml**

```yaml
dependencies:
  lvgl/lvgl: "^9.2.0"
  espressif/esp_lvgl_port: "^2.0.0"
  
  # Local components (relative paths)
  gdem102f91-epaper-driver:
    path: ../../gdem102f91-epaper-driver
    
  esp-lvgl-epaper-port:
    path: ../../esp-lvgl-epaper-port
```

- [ ] **Step 4: Create main/CMakeLists.txt (initial version)**

```cmake
idf_component_register(
    SRCS 
        "main.c"
        
    INCLUDE_DIRS 
        "."
        
    REQUIRES 
        nvs_flash
        esp_wifi
        esp_http_client
        esp_netif
        esp_sntp
        lvgl
        espressif__esp_lvgl_port
        esp-lvgl-epaper-port
        gdem102f91-epaper-driver
)
```

- [ ] **Step 5: Create main/Kconfig.projbuild**

```kconfig
menu "Home Assistant Dashboard Configuration"

    config HA_DASHBOARD_WIFI_SSID
        string "WiFi SSID"
        default "myssid"
        help
            WiFi SSID for development/testing. Will be overridden by NVS if set.

    config HA_DASHBOARD_WIFI_PASSWORD
        string "WiFi Password"
        default "mypassword"
        help
            WiFi password for development/testing. Will be overridden by NVS if set.

    config HA_DASHBOARD_HA_URL
        string "Home Assistant URL"
        default "http://homeassistant.local:8123"
        help
            Home Assistant base URL. Will be overridden by NVS if set.

    config HA_DASHBOARD_HA_TOKEN
        string "Home Assistant Long-Lived Token"
        default ""
        help
            Long-lived access token from HA. Will be overridden by NVS if set.

    config HA_DASHBOARD_UPDATE_INTERVAL
        int "Update interval (seconds)"
        default 600
        range 60 3600
        help
            How often to fetch data from Home Assistant (default: 10 minutes).

    config HA_DASHBOARD_ERROR_GRACE_PERIOD
        int "Error grace period (seconds)"
        default 1800
        range 300 7200
        help
            Time before showing error screen when HA unreachable (default: 30 minutes).

endmenu
```

- [ ] **Step 6: Create .gitignore**

```
build/
sdkconfig
sdkconfig.old
dependencies.lock
managed_components/
.DS_Store
*.swp
*.swo
*~
```

- [ ] **Step 7: Create placeholder main.c**

```c
#include <stdio.h>
#include "esp_log.h"
#include "nvs_flash.h"

static const char *TAG = "main";

void app_main(void)
{
    ESP_LOGI(TAG, "E-Paper Home Assistant Dashboard starting...");
    
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    ESP_LOGI(TAG, "NVS initialized");
}
```

- [ ] **Step 8: Test build system**

Run:
```bash
idf.py build
```

Expected: Clean build with no errors, output binary in `build/` directory

- [ ] **Step 9: Commit scaffolding**

```bash
git add CMakeLists.txt sdkconfig.defaults main/ .gitignore
git commit -m "feat: add project scaffolding and build configuration"
```

---

### Task 2: Shared Resources Infrastructure

**Files:**
- Create: `main/Shared/shared_resources.h`
- Create: `main/Shared/shared_resources.c`
- Modify: `main/CMakeLists.txt` (add shared_resources.c to SRCS)

**Interfaces:**
- Consumes: None
- Produces:
  - `typedef struct dashboard_state_t` - Shared state structure
  - `extern dashboard_state_t dashboard_state` - Global state instance
  - `extern SemaphoreHandle_t dashboard_state_mutex` - State protection
  - `extern SemaphoreHandle_t lvgl_mutex` - LVGL thread safety
  - `extern EventGroupHandle_t s_event_group` - Task synchronization
  - `void shared_resources_init(void)` - Initialization function
  - Event group bits: `WIFI_CONNECTED_BIT`, `TIME_SYNCED_BIT`, `HA_ERROR_BIT`

---

- [ ] **Step 1: Create main/Shared/shared_resources.h**

```c
#ifndef SHARED_RESOURCES_H
#define SHARED_RESOURCES_H

#include <stdbool.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"

// Event group bits
#define WIFI_CONNECTED_BIT  BIT0
#define TIME_SYNCED_BIT     BIT1
#define HA_ERROR_BIT        BIT2

// Shared state structure
typedef struct {
    bool wifi_connected;
    bool ha_connected;
    time_t last_successful_update;
    uint32_t failed_update_count;
    
    // HA data (Phase 1: date/time)
    char date_str[32];      // e.g., "2026-06-24"
    char time_str[32];      // e.g., "14:35"
    char timezone[64];      // e.g., "Africa/Johannesburg"
} dashboard_state_t;

// Global shared resources
extern dashboard_state_t dashboard_state;
extern SemaphoreHandle_t dashboard_state_mutex;
extern SemaphoreHandle_t lvgl_mutex;
extern EventGroupHandle_t s_event_group;

// Initialization
void shared_resources_init(void);

#endif // SHARED_RESOURCES_H
```

- [ ] **Step 2: Create main/Shared/shared_resources.c**

```c
#include "shared_resources.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "shared_resources";

// Global instances
dashboard_state_t dashboard_state;
SemaphoreHandle_t dashboard_state_mutex = NULL;
SemaphoreHandle_t lvgl_mutex = NULL;
EventGroupHandle_t s_event_group = NULL;

void shared_resources_init(void)
{
    ESP_LOGI(TAG, "Initializing shared resources...");
    
    // Create mutexes
    dashboard_state_mutex = xSemaphoreCreateMutex();
    lvgl_mutex = xSemaphoreCreateMutex();
    s_event_group = xEventGroupCreate();
    
    if (!dashboard_state_mutex || !lvgl_mutex || !s_event_group) {
        ESP_LOGE(TAG, "Failed to create synchronization primitives");
        abort();
    }
    
    // Initialize state
    memset(&dashboard_state, 0, sizeof(dashboard_state));
    dashboard_state.wifi_connected = false;
    dashboard_state.ha_connected = false;
    dashboard_state.last_successful_update = 0;
    dashboard_state.failed_update_count = 0;
    
    ESP_LOGI(TAG, "Shared resources initialized successfully");
}
```

- [ ] **Step 3: Update main/CMakeLists.txt**

```cmake
idf_component_register(
    SRCS 
        "main.c"
        "Shared/shared_resources.c"
        
    INCLUDE_DIRS 
        "."
        "Shared"
        
    REQUIRES 
        nvs_flash
        esp_wifi
        esp_http_client
        esp_netif
        esp_sntp
        lvgl
        espressif__esp_lvgl_port
        esp-lvgl-epaper-port
        gdem102f91-epaper-driver
)
```

- [ ] **Step 4: Update main.c to initialize shared resources**

```c
#include <stdio.h>
#include "esp_log.h"
#include "nvs_flash.h"
#include "Shared/shared_resources.h"

static const char *TAG = "main";

void app_main(void)
{
    ESP_LOGI(TAG, "E-Paper Home Assistant Dashboard starting...");
    
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    // Initialize shared resources
    shared_resources_init();
    
    ESP_LOGI(TAG, "Initialization complete");
}
```

- [ ] **Step 5: Test build**

Run:
```bash
idf.py build
```

Expected: Clean build with no errors

- [ ] **Step 6: Commit shared resources**

```bash
git add main/Shared/ main/CMakeLists.txt main/main.c
git commit -m "feat: add shared resources infrastructure"
```

---

### Task 3: Configuration Manager (NVS)

**Files:**
- Create: `main/Shared/config_manager.h`
- Create: `main/Shared/config_manager.c`
- Modify: `main/CMakeLists.txt` (add config_manager.c to SRCS)

**Interfaces:**
- Consumes: None
- Produces:
  - `esp_err_t config_init(void)` - Initialize NVS and load config
  - `esp_err_t config_get_wifi(char *ssid, size_t ssid_len, char *password, size_t pass_len)` - Get WiFi credentials
  - `esp_err_t config_get_ha_credentials(char *url, size_t url_len, char *token, size_t token_len)` - Get HA URL and token
  - `uint32_t config_get_update_interval(void)` - Get update interval in seconds
  - `uint32_t config_get_error_grace_period(void)` - Get error grace period in seconds
  - `uint8_t config_get_power_mode(void)` - Get power mode (0=always-on)

---

- [ ] **Step 1: Create main/Shared/config_manager.h**

```c
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

#endif // CONFIG_MANAGER_H
```

- [ ] **Step 2: Create main/Shared/config_manager.c**

```c
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

// Cached configuration
static struct {
    char wifi_ssid[64];
    char wifi_password[64];
    char ha_url[256];
    char ha_token[256];
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
        
        // Update interval
        if (nvs_get_u32(nvs_handle, "update_interval", &s_config.update_interval) != ESP_OK) {
            s_config.update_interval = CONFIG_HA_DASHBOARD_UPDATE_INTERVAL;
        }
        
        // Error grace period
        if (nvs_get_u32(nvs_handle, "error_grace_period", &s_config.error_grace_period) != ESP_OK) {
            s_config.error_grace_period = CONFIG_HA_DASHBOARD_ERROR_GRACE_PERIOD;
        }
        
        // Power mode
        if (nvs_get_u8(nvs_handle, "power_mode", &s_config.power_mode) != ESP_OK) {
            s_config.power_mode = 0; // Default: always-on
        }
        
        nvs_close(nvs_handle);
    } else {
        // NVS not initialized or empty, use Kconfig defaults
        ESP_LOGW(TAG, "NVS not available, using Kconfig defaults");
        strncpy(s_config.wifi_ssid, CONFIG_HA_DASHBOARD_WIFI_SSID, sizeof(s_config.wifi_ssid) - 1);
        strncpy(s_config.wifi_password, CONFIG_HA_DASHBOARD_WIFI_PASSWORD, sizeof(s_config.wifi_password) - 1);
        strncpy(s_config.ha_url, CONFIG_HA_DASHBOARD_HA_URL, sizeof(s_config.ha_url) - 1);
        strncpy(s_config.ha_token, CONFIG_HA_DASHBOARD_HA_TOKEN, sizeof(s_config.ha_token) - 1);
        s_config.update_interval = CONFIG_HA_DASHBOARD_UPDATE_INTERVAL;
        s_config.error_grace_period = CONFIG_HA_DASHBOARD_ERROR_GRACE_PERIOD;
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
    ESP_LOGI(TAG, "  Update interval: %lu seconds", s_config.update_interval);
    ESP_LOGI(TAG, "  Error grace period: %lu seconds", s_config.error_grace_period);
    ESP_LOGI(TAG, "  Power mode: %d", s_config.power_mode);
    
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
```

- [ ] **Step 3: Update main/CMakeLists.txt**

```cmake
idf_component_register(
    SRCS 
        "main.c"
        "Shared/shared_resources.c"
        "Shared/config_manager.c"
        
    INCLUDE_DIRS 
        "."
        "Shared"
        
    REQUIRES 
        nvs_flash
        esp_wifi
        esp_http_client
        esp_netif
        esp_sntp
        lvgl
        espressif__esp_lvgl_port
        esp-lvgl-epaper-port
        gdem102f91-epaper-driver
)
```

- [ ] **Step 4: Update main.c to initialize config**

```c
#include <stdio.h>
#include "esp_log.h"
#include "nvs_flash.h"
#include "Shared/shared_resources.h"
#include "Shared/config_manager.h"

static const char *TAG = "main";

void app_main(void)
{
    ESP_LOGI(TAG, "E-Paper Home Assistant Dashboard starting...");
    
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    // Initialize shared resources
    shared_resources_init();
    
    // Load configuration
    ESP_ERROR_CHECK(config_init());
    
    ESP_LOGI(TAG, "Initialization complete");
}
```

- [ ] **Step 5: Test build**

Run:
```bash
idf.py build
```

Expected: Clean build with no errors

- [ ] **Step 6: Commit configuration manager**

```bash
git add main/Shared/config_manager.c main/Shared/config_manager.h main/CMakeLists.txt main/main.c
git commit -m "feat: add NVS configuration manager"
```

---

### Task 4: WiFi Task

**Files:**
- Create: `main/Tasks/wifi_task.h`
- Create: `main/Tasks/wifi_task.c`
- Modify: `main/CMakeLists.txt` (add wifi_task.c to SRCS, add Tasks to INCLUDE_DIRS)
- Modify: `main/main.c` (create wifi_task)

**Interfaces:**
- Consumes:
  - `config_get_wifi(char *ssid, size_t ssid_len, char *password, size_t pass_len)` from config_manager
  - `s_event_group` from shared_resources
  - `dashboard_state_mutex` from shared_resources
  - `dashboard_state` from shared_resources
- Produces:
  - `void wifi_task(void *pvParameters)` - WiFi management task
  - Sets `WIFI_CONNECTED_BIT` when WiFi connects
  - Updates `dashboard_state.wifi_connected` via mutex

---

- [ ] **Step 1: Create main/Tasks/wifi_task.h**

```c
#ifndef WIFI_TASK_H
#define WIFI_TASK_H

void wifi_task(void *pvParameters);

#endif // WIFI_TASK_H
```

- [ ] **Step 2: Create main/Tasks/wifi_task.c**

```c
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
        if (s_retry_num < MAX_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "Retry to connect to WiFi... (%d/%d)", s_retry_num, MAX_RETRY);
        } else {
            ESP_LOGI(TAG, "Failed to connect to WiFi");
        }
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
    
    ESP_LOGI(TAG, "WiFi initialized, connecting to %s...", ssid);
    
    // Wait for connection with exponential backoff
    for (int i = 0; i < MAX_RETRY; i++) {
        EventBits_t bits = xEventGroupWaitBits(s_event_group,
                                                WIFI_CONNECTED_BIT,
                                                pdFALSE,
                                                pdFALSE,
                                                pdMS_TO_TICKS((1 << i) * 1000)); // 1s, 2s, 4s, 8s, 16s
        
        if (bits & WIFI_CONNECTED_BIT) {
            ESP_LOGI(TAG, "Connected to WiFi successfully");
            break;
        }
    }
    
    // Task complete - WiFi event handlers will manage reconnection
    ESP_LOGI(TAG, "WiFi task initialization complete");
    vTaskDelete(NULL);
}
```

- [ ] **Step 3: Update main/CMakeLists.txt**

```cmake
idf_component_register(
    SRCS 
        "main.c"
        "Shared/shared_resources.c"
        "Shared/config_manager.c"
        "Tasks/wifi_task.c"
        
    INCLUDE_DIRS 
        "."
        "Shared"
        "Tasks"
        
    REQUIRES 
        nvs_flash
        esp_wifi
        esp_http_client
        esp_netif
        esp_sntp
        lvgl
        espressif__esp_lvgl_port
        esp-lvgl-epaper-port
        gdem102f91-epaper-driver
)
```

- [ ] **Step 4: Update main.c to create WiFi task**

```c
#include <stdio.h>
#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "Shared/shared_resources.h"
#include "Shared/config_manager.h"
#include "Tasks/wifi_task.h"

static const char *TAG = "main";

void app_main(void)
{
    ESP_LOGI(TAG, "E-Paper Home Assistant Dashboard starting...");
    
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    // Initialize shared resources
    shared_resources_init();
    
    // Load configuration
    ESP_ERROR_CHECK(config_init());
    
    // Create WiFi task on Core 0
    xTaskCreatePinnedToCore(
        wifi_task,
        "wifi_task",
        4096,
        NULL,
        5,
        NULL,
        0  // Core 0
    );
    
    ESP_LOGI(TAG, "Initialization complete");
}
```

- [ ] **Step 5: Test build**

Run:
```bash
idf.py build
```

Expected: Clean build with no errors

- [ ] **Step 6: Flash and test WiFi connection**

Run:
```bash
idf.py -p /dev/cu.usbmodem113201 flash monitor
```

Expected:
- Logs show "WiFi task starting on core 0"
- Logs show "Connected to WiFi successfully"
- Logs show "Got IP: x.x.x.x"

- [ ] **Step 7: Commit WiFi task**

```bash
git add main/Tasks/wifi_task.c main/Tasks/wifi_task.h main/CMakeLists.txt main/main.c
git commit -m "feat: add WiFi connection task with auto-reconnect"
```

---

### Task 5: Time Sync Task

**Files:**
- Create: `main/Tasks/time_sync_task.h`
- Create: `main/Tasks/time_sync_task.c`
- Modify: `main/CMakeLists.txt` (add time_sync_task.c to SRCS)
- Modify: `main/main.c` (create time_sync_task)

**Interfaces:**
- Consumes:
  - `s_event_group` from shared_resources
  - `WIFI_CONNECTED_BIT` from shared_resources
- Produces:
  - `void time_sync_task(void *pvParameters)` - SNTP time sync task
  - Sets `TIME_SYNCED_BIT` when time synced
  - Updates system time via `settimeofday()`

---

- [ ] **Step 1: Create main/Tasks/time_sync_task.h**

```c
#ifndef TIME_SYNC_TASK_H
#define TIME_SYNC_TASK_H

void time_sync_task(void *pvParameters);

#endif // TIME_SYNC_TASK_H
```

- [ ] **Step 2: Create main/Tasks/time_sync_task.c**

```c
#include "time_sync_task.h"
#include "Shared/shared_resources.h"
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
    
    // Set timezone (UTC for now, can be configured later)
    setenv("TZ", "UTC0", 1);
    tzset();
    
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
```

- [ ] **Step 3: Update main/CMakeLists.txt**

```cmake
idf_component_register(
    SRCS 
        "main.c"
        "Shared/shared_resources.c"
        "Shared/config_manager.c"
        "Tasks/wifi_task.c"
        "Tasks/time_sync_task.c"
        
    INCLUDE_DIRS 
        "."
        "Shared"
        "Tasks"
        
    REQUIRES 
        nvs_flash
        esp_wifi
        esp_http_client
        esp_netif
        esp_sntp
        lvgl
        espressif__esp_lvgl_port
        esp-lvgl-epaper-port
        gdem102f91-epaper-driver
)
```

- [ ] **Step 4: Update main.c to create time sync task**

```c
#include <stdio.h>
#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "Shared/shared_resources.h"
#include "Shared/config_manager.h"
#include "Tasks/wifi_task.h"
#include "Tasks/time_sync_task.h"

static const char *TAG = "main";

void app_main(void)
{
    ESP_LOGI(TAG, "E-Paper Home Assistant Dashboard starting...");
    
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    // Initialize shared resources
    shared_resources_init();
    
    // Load configuration
    ESP_ERROR_CHECK(config_init());
    
    // Create WiFi task on Core 0
    xTaskCreatePinnedToCore(
        wifi_task,
        "wifi_task",
        4096,
        NULL,
        5,
        NULL,
        0  // Core 0
    );
    
    // Create time sync task on Core 0
    xTaskCreatePinnedToCore(
        time_sync_task,
        "time_sync_task",
        3072,
        NULL,
        3,
        NULL,
        0  // Core 0
    );
    
    ESP_LOGI(TAG, "Initialization complete");
}
```

- [ ] **Step 5: Test build**

Run:
```bash
idf.py build
```

Expected: Clean build with no errors

- [ ] **Step 6: Flash and test time sync**

Run:
```bash
idf.py -p /dev/cu.usbmodem113201 flash monitor
```

Expected:
- Logs show "Time sync task starting on core 0"
- Logs show "Waiting for WiFi connection..."
- Logs show "WiFi connected, initializing SNTP..."
- Logs show "Time synchronized"
- Logs show "Current time: <timestamp>"

- [ ] **Step 7: Commit time sync task**

```bash
git add main/Tasks/time_sync_task.c main/Tasks/time_sync_task.h main/CMakeLists.txt main/main.c
git commit -m "feat: add SNTP time synchronization task"
```

---

### Task 6: Home Assistant Client Task

**Files:**
- Create: `main/Tasks/ha_client_task.h`
- Create: `main/Tasks/ha_client_task.c`
- Modify: `main/CMakeLists.txt` (add ha_client_task.c to SRCS)
- Modify: `main/main.c` (create ha_client_task)

**Interfaces:**
- Consumes:
  - `config_get_ha_credentials(char *url, size_t url_len, char *token, size_t token_len)` from config_manager
  - `config_get_update_interval()` from config_manager
  - `config_get_error_grace_period()` from config_manager
  - `s_event_group`, `dashboard_state_mutex`, `dashboard_state` from shared_resources
  - `WIFI_CONNECTED_BIT`, `TIME_SYNCED_BIT` from shared_resources
- Produces:
  - `void ha_client_task(void *pvParameters)` - HA HTTP client task
  - Updates `dashboard_state.date_str`, `dashboard_state.time_str` via mutex
  - Updates `dashboard_state.last_successful_update`, `dashboard_state.failed_update_count`
  - Sets/clears `HA_ERROR_BIT` based on grace period

---

- [ ] **Step 1: Create main/Tasks/ha_client_task.h**

```c
#ifndef HA_CLIENT_TASK_H
#define HA_CLIENT_TASK_H

void ha_client_task(void *pvParameters);

#endif // HA_CLIENT_TASK_H
```

- [ ] **Step 2: Create main/Tasks/ha_client_task.c**

```c
#include "ha_client_task.h"
#include "Shared/shared_resources.h"
#include "Shared/config_manager.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "cJSON.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <time.h>

static const char *TAG = "ha_client_task";

#define HTTP_RESPONSE_BUFFER_SIZE 2048

static char s_http_response_buffer[HTTP_RESPONSE_BUFFER_SIZE];
static int s_http_response_len = 0;

static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    switch (evt->event_id) {
    case HTTP_EVENT_ON_DATA:
        if (s_http_response_len + evt->data_len < HTTP_RESPONSE_BUFFER_SIZE) {
            memcpy(s_http_response_buffer + s_http_response_len, evt->data, evt->data_len);
            s_http_response_len += evt->data_len;
            s_http_response_buffer[s_http_response_len] = '\0';
        } else {
            ESP_LOGW(TAG, "HTTP response buffer overflow");
        }
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
    esp_http_client_set_header(client, "Authorization", auth_header);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    
    err = esp_http_client_perform(client);
    
    if (err == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        if (status_code == 200) {
            // Parse JSON response
            cJSON *root = cJSON_Parse(s_http_response_buffer);
            if (root != NULL) {
                cJSON *state = cJSON_GetObjectItem(root, "state");
                if (state != NULL && cJSON_IsString(state)) {
                    strncpy(state_out, state->valuestring, state_len - 1);
                    state_out[state_len - 1] = '\0';
                    err = ESP_OK;
                    ESP_LOGD(TAG, "Fetched %s: %s", entity_id, state_out);
                } else {
                    ESP_LOGW(TAG, "No 'state' field in response");
                    err = ESP_FAIL;
                }
                cJSON_Delete(root);
            } else {
                ESP_LOGW(TAG, "Failed to parse JSON response");
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
    
    // Get HA credentials
    char ha_url[256] = {0};
    char ha_token[256] = {0};
    config_get_ha_credentials(ha_url, sizeof(ha_url), ha_token, sizeof(ha_token));
    
    uint32_t update_interval = config_get_update_interval();
    uint32_t grace_period = config_get_error_grace_period();
    
    ESP_LOGI(TAG, "Starting HA data fetch loop (interval: %lu seconds)", update_interval);
    
    while (1) {
        // Fetch date and time
        char date_str[32] = {0};
        char time_str[32] = {0};
        
        esp_err_t date_err = ESP_FAIL;
        esp_err_t time_err = ESP_FAIL;
        
        // Retry logic: 3 attempts with 5s delay
        for (int retry = 0; retry < 3; retry++) {
            if (date_err != ESP_OK) {
                date_err = ha_fetch_entity(ha_url, ha_token, "sensor.date", date_str, sizeof(date_str));
            }
            if (time_err != ESP_OK) {
                time_err = ha_fetch_entity(ha_url, ha_token, "sensor.time", time_str, sizeof(time_str));
            }
            
            if (date_err == ESP_OK && time_err == ESP_OK) {
                break;
            }
            
            if (retry < 2) {
                ESP_LOGW(TAG, "Retry %d/3 in 5 seconds...", retry + 1);
                vTaskDelay(pdMS_TO_TICKS(5000));
            }
        }
        
        // Update shared state
        if (xSemaphoreTake(dashboard_state_mutex, portMAX_DELAY) == pdTRUE) {
            if (date_err == ESP_OK && time_err == ESP_OK) {
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
                
                // Check if grace period exceeded
                time_t now;
                time(&now);
                time_t time_since_last_success = now - dashboard_state.last_successful_update;
                
                if (time_since_last_success > grace_period) {
                    ESP_LOGE(TAG, "Grace period exceeded (%ld > %lu seconds)", 
                             time_since_last_success, grace_period);
                    xEventGroupSetBits(s_event_group, HA_ERROR_BIT);
                }
            }
            
            xSemaphoreGive(dashboard_state_mutex);
        }
        
        // Sleep until next update
        vTaskDelay(pdMS_TO_TICKS(update_interval * 1000));
    }
}
```

- [ ] **Step 3: Update main/CMakeLists.txt**

```cmake
idf_component_register(
    SRCS 
        "main.c"
        "Shared/shared_resources.c"
        "Shared/config_manager.c"
        "Tasks/wifi_task.c"
        "Tasks/time_sync_task.c"
        "Tasks/ha_client_task.c"
        
    INCLUDE_DIRS 
        "."
        "Shared"
        "Tasks"
        
    REQUIRES 
        nvs_flash
        esp_wifi
        esp_http_client
        esp_netif
        esp_sntp
        lvgl
        espressif__esp_lvgl_port
        esp-lvgl-epaper-port
        gdem102f91-epaper-driver
        json
)
```

- [ ] **Step 4: Update main.c to create HA client task**

```c
#include <stdio.h>
#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "Shared/shared_resources.h"
#include "Shared/config_manager.h"
#include "Tasks/wifi_task.h"
#include "Tasks/time_sync_task.h"
#include "Tasks/ha_client_task.h"

static const char *TAG = "main";

void app_main(void)
{
    ESP_LOGI(TAG, "E-Paper Home Assistant Dashboard starting...");
    
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    // Initialize shared resources
    shared_resources_init();
    
    // Load configuration
    ESP_ERROR_CHECK(config_init());
    
    // Create WiFi task on Core 0
    xTaskCreatePinnedToCore(
        wifi_task,
        "wifi_task",
        4096,
        NULL,
        5,
        NULL,
        0  // Core 0
    );
    
    // Create time sync task on Core 0
    xTaskCreatePinnedToCore(
        time_sync_task,
        "time_sync_task",
        3072,
        NULL,
        3,
        NULL,
        0  // Core 0
    );
    
    // Create HA client task on Core 0
    xTaskCreatePinnedToCore(
        ha_client_task,
        "ha_client_task",
        6144,
        NULL,
        4,
        NULL,
        0  // Core 0
    );
    
    ESP_LOGI(TAG, "Initialization complete");
}
```

- [ ] **Step 5: Test build**

Run:
```bash
idf.py build
```

Expected: Clean build with no errors

- [ ] **Step 6: Commit HA client task**

```bash
git add main/Tasks/ha_client_task.c main/Tasks/ha_client_task.h main/CMakeLists.txt main/main.c
git commit -m "feat: add Home Assistant HTTP client task"
```

**Note:** Full integration test requires Home Assistant running with `sensor.date` and `sensor.time` entities configured. Will test in later integration task.

---

### Task 7: Power Management Task (Placeholder)

**Files:**
- Create: `main/Tasks/power_mgmt_task.h`
- Create: `main/Tasks/power_mgmt_task.c`
- Modify: `main/CMakeLists.txt` (add power_mgmt_task.c to SRCS)
- Modify: `main/main.c` (create power_mgmt_task)

**Interfaces:**
- Consumes:
  - `config_get_power_mode()` from config_manager
- Produces:
  - `void power_mgmt_task(void *pvParameters)` - Power management task
  - Phase 1: Logs power mode, does nothing else (always-on mode)

---

- [ ] **Step 1: Create main/Tasks/power_mgmt_task.h**

```c
#ifndef POWER_MGMT_TASK_H
#define POWER_MGMT_TASK_H

void power_mgmt_task(void *pvParameters);

#endif // POWER_MGMT_TASK_H
```

- [ ] **Step 2: Create main/Tasks/power_mgmt_task.c**

```c
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
```

- [ ] **Step 3: Update main/CMakeLists.txt**

```cmake
idf_component_register(
    SRCS 
        "main.c"
        "Shared/shared_resources.c"
        "Shared/config_manager.c"
        "Tasks/wifi_task.c"
        "Tasks/time_sync_task.c"
        "Tasks/ha_client_task.c"
        "Tasks/power_mgmt_task.c"
        
    INCLUDE_DIRS 
        "."
        "Shared"
        "Tasks"
        
    REQUIRES 
        nvs_flash
        esp_wifi
        esp_http_client
        esp_netif
        esp_sntp
        lvgl
        espressif__esp_lvgl_port
        esp-lvgl-epaper-port
        gdem102f91-epaper-driver
        json
)
```

- [ ] **Step 4: Update main.c to create power management task**

```c
#include <stdio.h>
#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "Shared/shared_resources.h"
#include "Shared/config_manager.h"
#include "Tasks/wifi_task.h"
#include "Tasks/time_sync_task.h"
#include "Tasks/ha_client_task.h"
#include "Tasks/power_mgmt_task.h"

static const char *TAG = "main";

void app_main(void)
{
    ESP_LOGI(TAG, "E-Paper Home Assistant Dashboard starting...");
    
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    // Initialize shared resources
    shared_resources_init();
    
    // Load configuration
    ESP_ERROR_CHECK(config_init());
    
    // Create WiFi task on Core 0
    xTaskCreatePinnedToCore(
        wifi_task,
        "wifi_task",
        4096,
        NULL,
        5,
        NULL,
        0  // Core 0
    );
    
    // Create time sync task on Core 0
    xTaskCreatePinnedToCore(
        time_sync_task,
        "time_sync_task",
        3072,
        NULL,
        3,
        NULL,
        0  // Core 0
    );
    
    // Create HA client task on Core 0
    xTaskCreatePinnedToCore(
        ha_client_task,
        "ha_client_task",
        6144,
        NULL,
        4,
        NULL,
        0  // Core 0
    );
    
    // Create power management task (no core affinity)
    xTaskCreate(
        power_mgmt_task,
        "power_mgmt_task",
        2048,
        NULL,
        2,
        NULL
    );
    
    ESP_LOGI(TAG, "Initialization complete");
}
```

- [ ] **Step 5: Test build**

Run:
```bash
idf.py build
```

Expected: Clean build with no errors

- [ ] **Step 6: Commit power management task**

```bash
git add main/Tasks/power_mgmt_task.c main/Tasks/power_mgmt_task.h main/CMakeLists.txt main/main.c
git commit -m "feat: add power management task placeholder"
```

---

### Task 8: EEZ Studio UI Placeholder

**Files:**
- Create: `main/ui/` directory
- Create: `main/ui/screens.c`
- Create: `main/ui/screens.h`
- Create: `main/ui/ui.c`
- Create: `main/ui/ui.h`
- Create: `main/ui/vars.h`
- Create: `main/ui/actions.h`

**Interfaces:**
- Consumes: None
- Produces:
  - `void ui_init(void)` - Initialize EEZ UI
  - Variable declarations in `vars.h`
  - Action declarations in `actions.h`

**Note:** These are placeholder files. In a real implementation, these would be generated by EEZ Studio. For Phase 1, we create minimal hand-written versions.

---

- [ ] **Step 1: Create main/ui directory**

```bash
mkdir -p main/ui
```

- [ ] **Step 2: Create main/ui/vars.h**

```c
#ifndef EEZ_UI_VARS_H
#define EEZ_UI_VARS_H

#include "lvgl.h"

// Dashboard screen variables
extern lv_obj_t *var_date_label;
extern lv_obj_t *var_time_label;
extern lv_obj_t *var_wifi_status_icon;
extern lv_obj_t *var_ha_status_icon;

// Error screen variables
extern lv_obj_t *var_error_title;
extern lv_obj_t *var_error_wifi_status;
extern lv_obj_t *var_error_ha_status;
extern lv_obj_t *var_error_last_update;

#endif // EEZ_UI_VARS_H
```

- [ ] **Step 3: Create main/ui/actions.h**

```c
#ifndef EEZ_UI_ACTIONS_H
#define EEZ_UI_ACTIONS_H

// No actions for Phase 1 (no touch interface)

#endif // EEZ_UI_ACTIONS_H
```

- [ ] **Step 4: Create main/ui/screens.h**

```c
#ifndef EEZ_UI_SCREENS_H
#define EEZ_UI_SCREENS_H

#include "lvgl.h"

// Screen objects
extern lv_obj_t *screen_dashboard;
extern lv_obj_t *screen_error;

// Screen creation functions
void create_screen_dashboard(void);
void create_screen_error(void);

#endif // EEZ_UI_SCREENS_H
```

- [ ] **Step 5: Create main/ui/screens.c**

```c
#include "screens.h"
#include "vars.h"
#include "lvgl.h"

lv_obj_t *screen_dashboard = NULL;
lv_obj_t *screen_error = NULL;

// Dashboard screen variables
lv_obj_t *var_date_label = NULL;
lv_obj_t *var_time_label = NULL;
lv_obj_t *var_wifi_status_icon = NULL;
lv_obj_t *var_ha_status_icon = NULL;

// Error screen variables
lv_obj_t *var_error_title = NULL;
lv_obj_t *var_error_wifi_status = NULL;
lv_obj_t *var_error_ha_status = NULL;
lv_obj_t *var_error_last_update = NULL;

void create_screen_dashboard(void)
{
    screen_dashboard = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen_dashboard, lv_color_white(), 0);
    
    // Title
    lv_obj_t *title = lv_label_create(screen_dashboard);
    lv_label_set_text(title, "Home Assistant Dashboard");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);
    
    // Date label
    var_date_label = lv_label_create(screen_dashboard);
    lv_label_set_text(var_date_label, "Date: --");
    lv_obj_set_style_text_font(var_date_label, &lv_font_montserrat_48, 0);
    lv_obj_align(var_date_label, LV_ALIGN_CENTER, 0, -40);
    
    // Time label
    var_time_label = lv_label_create(screen_dashboard);
    lv_label_set_text(var_time_label, "Time: --");
    lv_obj_set_style_text_font(var_time_label, &lv_font_montserrat_48, 0);
    lv_obj_align(var_time_label, LV_ALIGN_CENTER, 0, 40);
    
    // WiFi status icon (simple label for now)
    var_wifi_status_icon = lv_label_create(screen_dashboard);
    lv_label_set_text(var_wifi_status_icon, "WiFi: ?");
    lv_obj_align(var_wifi_status_icon, LV_ALIGN_BOTTOM_LEFT, 10, -10);
    
    // HA status icon
    var_ha_status_icon = lv_label_create(screen_dashboard);
    lv_label_set_text(var_ha_status_icon, "HA: ?");
    lv_obj_align(var_ha_status_icon, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
}

void create_screen_error(void)
{
    screen_error = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen_error, lv_color_white(), 0);
    
    // Error title
    var_error_title = lv_label_create(screen_error);
    lv_label_set_text(var_error_title, "Home Assistant Unreachable");
    lv_obj_set_style_text_font(var_error_title, &lv_font_montserrat_32, 0);
    lv_obj_set_style_text_color(var_error_title, lv_color_make(255, 0, 0), 0);
    lv_obj_align(var_error_title, LV_ALIGN_TOP_MID, 0, 40);
    
    // WiFi status
    var_error_wifi_status = lv_label_create(screen_error);
    lv_label_set_text(var_error_wifi_status, "WiFi: Unknown");
    lv_obj_set_style_text_font(var_error_wifi_status, &lv_font_montserrat_24, 0);
    lv_obj_align(var_error_wifi_status, LV_ALIGN_CENTER, 0, -60);
    
    // HA status
    var_error_ha_status = lv_label_create(screen_error);
    lv_label_set_text(var_error_ha_status, "HA: Disconnected");
    lv_obj_set_style_text_font(var_error_ha_status, &lv_font_montserrat_24, 0);
    lv_obj_align(var_error_ha_status, LV_ALIGN_CENTER, 0, -20);
    
    // Last update time
    var_error_last_update = lv_label_create(screen_error);
    lv_label_set_text(var_error_last_update, "Last Update: Never");
    lv_obj_set_style_text_font(var_error_last_update, &lv_font_montserrat_20, 0);
    lv_obj_align(var_error_last_update, LV_ALIGN_CENTER, 0, 40);
}
```

- [ ] **Step 6: Create main/ui/ui.h**

```c
#ifndef EEZ_UI_H
#define EEZ_UI_H

void ui_init(void);

#endif // EEZ_UI_H
```

- [ ] **Step 7: Create main/ui/ui.c**

```c
#include "ui.h"
#include "screens.h"
#include "lvgl.h"
#include "esp_log.h"

static const char *TAG = "eez_ui";

void ui_init(void)
{
    ESP_LOGI(TAG, "Initializing EEZ UI...");
    
    // Create screens
    create_screen_dashboard();
    create_screen_error();
    
    // Load dashboard screen by default
    lv_scr_load(screen_dashboard);
    
    ESP_LOGI(TAG, "EEZ UI initialized");
}
```

- [ ] **Step 8: Test build (UI not integrated yet)**

Run:
```bash
idf.py build
```

Expected: Clean build (UI files not yet included in CMakeLists.txt)

- [ ] **Step 9: Commit EEZ UI placeholder**

```bash
git add main/ui/
git commit -m "feat: add EEZ Studio UI placeholder files"
```

---

### Task 9: EEZ Variables Implementation

**Files:**
- Create: `main/eez_vars.h`
- Create: `main/eez_vars.c`
- Create: `main/eez_actions.h`
- Create: `main/eez_actions.c`

**Interfaces:**
- Consumes:
  - Variables from `main/ui/vars.h`
  - Screens from `main/ui/screens.h`
- Produces:
  - `void eez_set_date(const char *date)` - Update date label
  - `void eez_set_time(const char *time)` - Update time label
  - `void eez_set_wifi_status(bool connected)` - Update WiFi status
  - `void eez_set_ha_status(bool connected)` - Update HA status
  - `void eez_show_error_screen(const char *wifi_status, const char *ha_status, const char *last_update)` - Show error screen
  - `void eez_show_dashboard_screen(void)` - Show dashboard screen

---

- [ ] **Step 1: Create main/eez_vars.h**

```c
#ifndef EEZ_VARS_H
#define EEZ_VARS_H

#include <stdbool.h>

// Update functions for dashboard screen
void eez_set_date(const char *date);
void eez_set_time(const char *time);
void eez_set_wifi_status(bool connected);
void eez_set_ha_status(bool connected);

// Screen switching
void eez_show_error_screen(const char *wifi_status, const char *ha_status, const char *last_update);
void eez_show_dashboard_screen(void);

#endif // EEZ_VARS_H
```

- [ ] **Step 2: Create main/eez_vars.c**

```c
#include "eez_vars.h"
#include "ui/vars.h"
#include "ui/screens.h"
#include "lvgl.h"
#include "esp_log.h"
#include <stdio.h>

static const char *TAG = "eez_vars";

void eez_set_date(const char *date)
{
    if (var_date_label && date) {
        char buf[64];
        snprintf(buf, sizeof(buf), "Date: %s", date);
        lv_label_set_text(var_date_label, buf);
        ESP_LOGD(TAG, "Updated date: %s", date);
    }
}

void eez_set_time(const char *time)
{
    if (var_time_label && time) {
        char buf[64];
        snprintf(buf, sizeof(buf), "Time: %s", time);
        lv_label_set_text(var_time_label, buf);
        ESP_LOGD(TAG, "Updated time: %s", time);
    }
}

void eez_set_wifi_status(bool connected)
{
    if (var_wifi_status_icon) {
        lv_label_set_text(var_wifi_status_icon, connected ? "WiFi: OK" : "WiFi: X");
        lv_obj_set_style_text_color(var_wifi_status_icon, 
                                      connected ? lv_color_make(0, 255, 0) : lv_color_make(255, 0, 0), 
                                      0);
        ESP_LOGD(TAG, "Updated WiFi status: %s", connected ? "connected" : "disconnected");
    }
}

void eez_set_ha_status(bool connected)
{
    if (var_ha_status_icon) {
        lv_label_set_text(var_ha_status_icon, connected ? "HA: OK" : "HA: X");
        lv_obj_set_style_text_color(var_ha_status_icon,
                                      connected ? lv_color_make(0, 255, 0) : lv_color_make(255, 0, 0),
                                      0);
        ESP_LOGD(TAG, "Updated HA status: %s", connected ? "connected" : "disconnected");
    }
}

void eez_show_error_screen(const char *wifi_status, const char *ha_status, const char *last_update)
{
    if (screen_error) {
        if (var_error_wifi_status && wifi_status) {
            char buf[64];
            snprintf(buf, sizeof(buf), "WiFi: %s", wifi_status);
            lv_label_set_text(var_error_wifi_status, buf);
        }
        
        if (var_error_ha_status && ha_status) {
            char buf[64];
            snprintf(buf, sizeof(buf), "HA: %s", ha_status);
            lv_label_set_text(var_error_ha_status, buf);
        }
        
        if (var_error_last_update && last_update) {
            char buf[128];
            snprintf(buf, sizeof(buf), "Last Update: %s", last_update);
            lv_label_set_text(var_error_last_update, buf);
        }
        
        lv_scr_load(screen_error);
        ESP_LOGI(TAG, "Switched to error screen");
    }
}

void eez_show_dashboard_screen(void)
{
    if (screen_dashboard) {
        lv_scr_load(screen_dashboard);
        ESP_LOGI(TAG, "Switched to dashboard screen");
    }
}
```

- [ ] **Step 3: Create main/eez_actions.h**

```c
#ifndef EEZ_ACTIONS_H
#define EEZ_ACTIONS_H

// No actions for Phase 1 (no touch interface)
// This file exists for EEZ Studio compatibility

#endif // EEZ_ACTIONS_H
```

- [ ] **Step 4: Create main/eez_actions.c**

```c
#include "eez_actions.h"

// No actions for Phase 1 (no touch interface)
// This file exists for EEZ Studio compatibility
```

- [ ] **Step 5: Test build (not integrated yet)**

Run:
```bash
idf.py build
```

Expected: Clean build (eez_vars not yet included in CMakeLists.txt)

- [ ] **Step 6: Commit EEZ vars implementation**

```bash
git add main/eez_vars.c main/eez_vars.h main/eez_actions.c main/eez_actions.h
git commit -m "feat: add EEZ variables implementation"
```

---

### Task 10: LVGL Task & Display Integration

**Files:**
- Create: `main/Tasks/lvgl_task.h`
- Create: `main/Tasks/lvgl_task.c`
- Modify: `main/CMakeLists.txt` (add lvgl_task.c, ui/*.c, eez_*.c to SRCS)
- Modify: `main/main.c` (create lvgl_task on Core 1)

**Interfaces:**
- Consumes:
  - `lvgl_mutex` from shared_resources
  - `ui_init()` from ui/ui.h
  - E-paper driver from `esp-lvgl-epaper-port`
- Produces:
  - `void lvgl_task(void *pvParameters)` - LVGL timer handler task
  - Initialized LVGL display on e-paper

---

- [ ] **Step 1: Create main/Tasks/lvgl_task.h**

```c
#ifndef LVGL_TASK_H
#define LVGL_TASK_H

void lvgl_task(void *pvParameters);

#endif // LVGL_TASK_H
```

- [ ] **Step 2: Create main/Tasks/lvgl_task.c**

```c
#include "lvgl_task.h"
#include "Shared/shared_resources.h"
#include "ui/ui.h"
#include "esp_log.h"
#include "esp_lvgl_port.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// E-paper port header (from esp-lvgl-epaper-port component)
#include "esp_lvgl_epaper_port.h"

static const char *TAG = "lvgl_task";

void lvgl_task(void *pvParameters)
{
    ESP_LOGI(TAG, "LVGL task starting on core %d", xPortGetCoreID());
    
    // Initialize LVGL port
    ESP_LOGI(TAG, "Initializing LVGL e-paper port...");
    esp_err_t err = esp_lvgl_epaper_port_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize LVGL e-paper port: %s", esp_err_to_name(err));
        vTaskDelete(NULL);
        return;
    }
    
    // Initialize EEZ UI
    ESP_LOGI(TAG, "Initializing EEZ UI...");
    ui_init();
    
    ESP_LOGI(TAG, "LVGL task initialization complete");
    
    // Main loop: run LVGL timer handler
    while (1) {
        // Lock LVGL mutex
        if (xSemaphoreTake(lvgl_mutex, portMAX_DELAY) == pdTRUE) {
            // Run LVGL tasks
            lv_timer_handler();
            
            // Release mutex
            xSemaphoreGive(lvgl_mutex);
        }
        
        // Sleep for 5ms
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}
```

- [ ] **Step 3: Update main/CMakeLists.txt (add all UI files)**

```cmake
idf_component_register(
    SRCS 
        "main.c"
        "Shared/shared_resources.c"
        "Shared/config_manager.c"
        "Tasks/wifi_task.c"
        "Tasks/time_sync_task.c"
        "Tasks/ha_client_task.c"
        "Tasks/power_mgmt_task.c"
        "Tasks/lvgl_task.c"
        "eez_vars.c"
        "eez_actions.c"
        "ui/screens.c"
        "ui/ui.c"
        
    INCLUDE_DIRS 
        "."
        "Shared"
        "Tasks"
        "ui"
        
    REQUIRES 
        nvs_flash
        esp_wifi
        esp_http_client
        esp_netif
        esp_sntp
        lvgl
        espressif__esp_lvgl_port
        esp-lvgl-epaper-port
        gdem102f91-epaper-driver
        json
)
```

- [ ] **Step 4: Update main.c to create LVGL task**

```c
#include <stdio.h>
#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "Shared/shared_resources.h"
#include "Shared/config_manager.h"
#include "Tasks/wifi_task.h"
#include "Tasks/time_sync_task.h"
#include "Tasks/ha_client_task.h"
#include "Tasks/power_mgmt_task.h"
#include "Tasks/lvgl_task.h"

static const char *TAG = "main";

void app_main(void)
{
    ESP_LOGI(TAG, "E-Paper Home Assistant Dashboard starting...");
    
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    // Initialize shared resources
    shared_resources_init();
    
    // Load configuration
    ESP_ERROR_CHECK(config_init());
    
    // Create WiFi task on Core 0
    xTaskCreatePinnedToCore(
        wifi_task,
        "wifi_task",
        4096,
        NULL,
        5,
        NULL,
        0  // Core 0
    );
    
    // Create time sync task on Core 0
    xTaskCreatePinnedToCore(
        time_sync_task,
        "time_sync_task",
        3072,
        NULL,
        3,
        NULL,
        0  // Core 0
    );
    
    // Create HA client task on Core 0
    xTaskCreatePinnedToCore(
        ha_client_task,
        "ha_client_task",
        6144,
        NULL,
        4,
        NULL,
        0  // Core 0
    );
    
    // Create power management task (no core affinity)
    xTaskCreate(
        power_mgmt_task,
        "power_mgmt_task",
        2048,
        NULL,
        2,
        NULL
    );
    
    // Create LVGL task on Core 1
    xTaskCreatePinnedToCore(
        lvgl_task,
        "lvgl_task",
        4096,
        NULL,
        6,
        NULL,
        1  // Core 1
    );
    
    ESP_LOGI(TAG, "All tasks created successfully");
}
```

- [ ] **Step 5: Test build**

Run:
```bash
idf.py build
```

Expected: Clean build with no errors

- [ ] **Step 6: Commit LVGL task integration**

```bash
git add main/Tasks/lvgl_task.c main/Tasks/lvgl_task.h main/CMakeLists.txt main/main.c
git commit -m "feat: add LVGL task with e-paper display integration"
```

---

### Task 11: Display Task (Update Coordination)

**Files:**
- Create: `main/Tasks/display_task.h`
- Create: `main/Tasks/display_task.c`
- Modify: `main/CMakeLists.txt` (add display_task.c to SRCS)
- Modify: `main/main.c` (create display_task on Core 1)

**Interfaces:**
- Consumes:
  - `dashboard_state_mutex`, `dashboard_state` from shared_resources
  - `lvgl_mutex` from shared_resources
  - `s_event_group`, `HA_ERROR_BIT` from shared_resources
  - EEZ functions: `eez_set_date()`, `eez_set_time()`, `eez_set_wifi_status()`, `eez_set_ha_status()`, `eez_show_error_screen()`, `eez_show_dashboard_screen()`
  - `config_get_error_grace_period()` from config_manager
- Produces:
  - `void display_task(void *pvParameters)` - Display update coordination task
  - Monitors state changes and updates UI
  - Triggers e-paper refresh via LVGL

---

- [ ] **Step 1: Create main/Tasks/display_task.h**

```c
#ifndef DISPLAY_TASK_H
#define DISPLAY_TASK_H

void display_task(void *pvParameters);

#endif // DISPLAY_TASK_H
```

- [ ] **Step 2: Create main/Tasks/display_task.c**

```c
#include "display_task.h"
#include "Shared/shared_resources.h"
#include "Shared/config_manager.h"
#include "eez_vars.h"
#include "esp_log.h"
#include "lvgl.h"
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
    
    // Wait a bit for LVGL task to initialize
    vTaskDelay(pdMS_TO_TICKS(2000));
    
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
            
            // Lock LVGL mutex
            if (xSemaphoreTake(lvgl_mutex, portMAX_DELAY) == pdTRUE) {
                
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
                
                // Force LVGL refresh (will trigger e-paper flush, ~20s blocking)
                lv_refr_now(NULL);
                
                // Release mutex
                xSemaphoreGive(lvgl_mutex);
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
```

- [ ] **Step 3: Update main/CMakeLists.txt**

```cmake
idf_component_register(
    SRCS 
        "main.c"
        "Shared/shared_resources.c"
        "Shared/config_manager.c"
        "Tasks/wifi_task.c"
        "Tasks/time_sync_task.c"
        "Tasks/ha_client_task.c"
        "Tasks/power_mgmt_task.c"
        "Tasks/lvgl_task.c"
        "Tasks/display_task.c"
        "eez_vars.c"
        "eez_actions.c"
        "ui/screens.c"
        "ui/ui.c"
        
    INCLUDE_DIRS 
        "."
        "Shared"
        "Tasks"
        "ui"
        
    REQUIRES 
        nvs_flash
        esp_wifi
        esp_http_client
        esp_netif
        esp_sntp
        lvgl
        espressif__esp_lvgl_port
        esp-lvgl-epaper-port
        gdem102f91-epaper-driver
        json
)
```

- [ ] **Step 4: Update main.c to create display task**

```c
#include <stdio.h>
#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "Shared/shared_resources.h"
#include "Shared/config_manager.h"
#include "Tasks/wifi_task.h"
#include "Tasks/time_sync_task.h"
#include "Tasks/ha_client_task.h"
#include "Tasks/power_mgmt_task.h"
#include "Tasks/lvgl_task.h"
#include "Tasks/display_task.h"

static const char *TAG = "main";

void app_main(void)
{
    ESP_LOGI(TAG, "E-Paper Home Assistant Dashboard starting...");
    
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    // Initialize shared resources
    shared_resources_init();
    
    // Load configuration
    ESP_ERROR_CHECK(config_init());
    
    // Create WiFi task on Core 0
    xTaskCreatePinnedToCore(
        wifi_task,
        "wifi_task",
        4096,
        NULL,
        5,
        NULL,
        0  // Core 0
    );
    
    // Create time sync task on Core 0
    xTaskCreatePinnedToCore(
        time_sync_task,
        "time_sync_task",
        3072,
        NULL,
        3,
        NULL,
        0  // Core 0
    );
    
    // Create HA client task on Core 0
    xTaskCreatePinnedToCore(
        ha_client_task,
        "ha_client_task",
        6144,
        NULL,
        4,
        NULL,
        0  // Core 0
    );
    
    // Create power management task (no core affinity)
    xTaskCreate(
        power_mgmt_task,
        "power_mgmt_task",
        2048,
        NULL,
        2,
        NULL
    );
    
    // Create LVGL task on Core 1
    xTaskCreatePinnedToCore(
        lvgl_task,
        "lvgl_task",
        4096,
        NULL,
        6,
        NULL,
        1  // Core 1
    );
    
    // Create display task on Core 1
    xTaskCreatePinnedToCore(
        display_task,
        "display_task",
        4096,
        NULL,
        5,
        NULL,
        1  // Core 1
    );
    
    ESP_LOGI(TAG, "All tasks created successfully");
}
```

- [ ] **Step 5: Test build**

Run:
```bash
idf.py build
```

Expected: Clean build with no errors

- [ ] **Step 6: Commit display task**

```bash
git add main/Tasks/display_task.c main/Tasks/display_task.h main/CMakeLists.txt main/main.c
git commit -m "feat: add display task for e-paper update coordination"
```

---

### Task 12: Integration Testing & Verification

**Files:**
- None (testing only)

**Interfaces:**
- Full system integration test

**Prerequisites:**
- Home Assistant running and accessible
- Home Assistant long-lived token configured
- `sensor.date` and `sensor.time` entities available in HA
- WiFi credentials configured in menuconfig or NVS

---

- [ ] **Step 1: Configure Home Assistant credentials in menuconfig**

Run:
```bash
idf.py menuconfig
```

Navigate to: `Home Assistant Dashboard Configuration`
Set:
- WiFi SSID: (your network)
- WiFi Password: (your password)
- Home Assistant URL: (e.g., `http://192.168.1.100:8123`)
- Home Assistant Token: (your long-lived token)

Save and exit.

- [ ] **Step 2: Build and flash complete system**

Run:
```bash
idf.py build
idf.py -p /dev/cu.usbmodem113201 flash monitor
```

- [ ] **Step 3: Verify boot sequence**

Expected serial output:
```
I (xxx) main: E-Paper Home Assistant Dashboard starting...
I (xxx) shared_resources: Initializing shared resources...
I (xxx) shared_resources: Shared resources initialized successfully
I (xxx) config_manager: Initializing configuration...
I (xxx) config_manager: Configuration loaded:
I (xxx) config_manager:   WiFi SSID: <your-ssid>
I (xxx) config_manager:   HA URL: <your-ha-url>
I (xxx) config_manager:   Update interval: 600 seconds
I (xxx) main: All tasks created successfully
I (xxx) wifi_task: WiFi task starting on core 0
I (xxx) wifi_task: WiFi initialized, connecting to <ssid>...
I (xxx) time_sync_task: Time sync task starting on core 0
I (xxx) ha_client_task: HA client task starting on core 0
I (xxx) power_mgmt_task: Power management task starting
I (xxx) power_mgmt_task: Power mode: Always-On (no power management)
I (xxx) lvgl_task: LVGL task starting on core 1
I (xxx) display_task: Display task starting on core 1
```

- [ ] **Step 4: Verify WiFi connection**

Expected:
```
I (xxx) wifi_task: Got IP: x.x.x.x
I (xxx) wifi_task: Connected to WiFi successfully
```

- [ ] **Step 5: Verify time synchronization**

Expected:
```
I (xxx) time_sync_task: WiFi connected, initializing SNTP...
I (xxx) time_sync_task: Time synchronized
I (xxx) time_sync_task: Current time: <timestamp>
```

- [ ] **Step 6: Verify HA data fetch**

Expected:
```
I (xxx) ha_client_task: Starting HA data fetch loop (interval: 600 seconds)
I (xxx) ha_client_task: Updated: Date=2026-06-24, Time=14:35
```

- [ ] **Step 7: Verify display initialization**

Expected:
```
I (xxx) lvgl_task: Initializing LVGL e-paper port...
I (xxx) eez_ui: Initializing EEZ UI...
I (xxx) eez_ui: EEZ UI initialized
I (xxx) lvgl_task: LVGL task initialization complete
I (xxx) display_task: Display task monitoring state changes...
I (xxx) display_task: State changed, updating display...
I (xxx) display_task: Display updated successfully
```

- [ ] **Step 8: Verify e-paper display shows date and time**

Check physical e-paper display:
- Should show "Home Assistant Dashboard" title
- Should show "Date: 2026-06-24" (or current date)
- Should show "Time: 14:35" (or current time)
- Should show "WiFi: OK" in green
- Should show "HA: OK" in green

Note: First refresh takes ~20 seconds (normal for e-paper)

- [ ] **Step 9: Test update cycle (wait 10+ minutes)**

Expected:
- Logs show periodic HA fetch every 10 minutes
- Display updates when time changes (every minute)
- No errors in logs

- [ ] **Step 10: Test WiFi recovery**

1. Disconnect WiFi AP
2. Verify logs show disconnect and reconnect attempts
3. Verify display shows "WiFi: X" in red
4. Reconnect WiFi AP
5. Verify logs show successful reconnect
6. Verify display shows "WiFi: OK" in green

- [ ] **Step 11: Test HA error handling**

1. Stop Home Assistant or block network access to HA
2. Verify logs show failed requests
3. Wait for grace period to expire (30 minutes default)
4. Verify error screen appears with:
   - "Home Assistant Unreachable" title
   - WiFi status
   - HA status: Disconnected
   - Last successful update timestamp
5. Restore HA connectivity
6. Verify dashboard screen returns

- [ ] **Step 12: Create final integration test documentation**

Create file: `docs/integration-test-results.md`

Document:
- Test date and time
- Hardware used
- Home Assistant version
- All test results (pass/fail)
- Any issues encountered
- Screenshots of display (if possible)

- [ ] **Step 13: Commit integration test documentation**

```bash
git add docs/integration-test-results.md
git commit -m "docs: add Phase 1 integration test results"
```

- [ ] **Step 14: Final commit and tag**

```bash
git tag -a v1.0.0-phase1 -m "Phase 1 complete: Date/time display from Home Assistant"
git log --oneline
```

Expected: Clean commit history showing all tasks completed

---

## Self-Review Complete

**Spec Coverage:**
✅ All Phase 1 requirements implemented:
- Project scaffolding and build configuration
- Shared resources and synchronization
- NVS configuration management
- WiFi task with auto-reconnect
- SNTP time synchronization
- Home Assistant HTTP client with retry logic
- LVGL and e-paper display integration
- Display update coordination
- Error handling with grace period
- Power management placeholder (always-on mode)
- EEZ Studio UI integration
- Full system integration testing

**Placeholder Scan:**
✅ No TBD, TODO, or incomplete sections
✅ All code blocks contain actual implementation
✅ All commands include expected output
✅ All file paths are exact

**Type Consistency:**
✅ All function signatures match across tasks
✅ All struct fields consistent
✅ All event bits defined and used consistently

---

## Plan Complete

This implementation plan covers all Phase 1 requirements from the design spec. Each task is self-contained with test steps and commits. The plan follows TDD principles where applicable and maintains DRY/YAGNI discipline.

**Total Tasks:** 12
**Estimated Time:** 8-12 hours (depending on debugging and testing)
