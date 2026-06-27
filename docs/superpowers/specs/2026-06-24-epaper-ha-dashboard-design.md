# E-Paper Home Assistant Dashboard - Design Document

**Date:** 2026-06-24  
**Project:** epaper-homeassistant-dashboard  
**Status:** Approved for Implementation

---

## 1. Project Overview & Goals

### Project Name
E-Paper Home Assistant Dashboard

### Primary Goal
Create an ESP32-based e-paper dashboard that displays Home Assistant data on a GDEM102F91 10.2" display, starting with date/time, expandable to weather, sensors, and IoT data.

### Phase 1 Scope (MVP)
- WiFi connectivity with configurable credentials
- SNTP time synchronization for reliable timekeeping
- Home Assistant REST API integration (long-term token auth)
- Fetch and display date/time formatted per Home Assistant timezone
- Configurable update interval (default: 10 minutes)
- UI designed in EEZ Studio
- Configuration stored in NVS
- FreeRTOS task architecture with core affinity (Core 0: network, Core 1: UI)

### Hardware
- **Board:** Seeed Studio XIAO ESP32-S3 ePaper Display Board (EE04 v1.2)
- **Display:** Goodisplay GDEM102F91 (960×640, 4-color e-paper)
- **Power:** USB powered (configurable power management for future battery projects)

### Future Expansion
Weather, air quality, UV index, geyser, solar batteries, pool sensors - all via Home Assistant REST API.

---

## 2. Architecture & Component Organization

### Project Structure

```
epaper-homeassistant-dashboard/
├── main/
│   ├── main.c                          # App entry, initialization, task creation
│   ├── CMakeLists.txt                  # Component registration
│   ├── idf_component.yml               # Dependencies (LVGL, drivers)
│   ├── Kconfig.projbuild               # Build-time configuration options
│   │
│   ├── Tasks/                          # FreeRTOS tasks (one file per task)
│   │   ├── wifi_task.c/h              # WiFi connection management (Core 0)
│   │   ├── time_sync_task.c/h         # SNTP time synchronization (Core 0)
│   │   ├── ha_client_task.c/h         # Home Assistant HTTP client (Core 0)
│   │   ├── lvgl_task.c/h              # LVGL timer handler (Core 1)
│   │   ├── display_task.c/h           # E-paper refresh coordination (Core 1)
│   │   └── power_mgmt_task.c/h        # Power management (configurable)
│   │
│   ├── Shared/                         # Shared resources & state
│   │   ├── shared_resources.c/h       # Mutexes, event groups, shared structs
│   │   └── config_manager.c/h         # NVS configuration read/write
│   │
│   └── ui/                             # EEZ Studio generated files
│       ├── screens.c/h                 # Generated UI screens
│       ├── ui.c/h                      # Generated UI framework
│       └── ...                         # Other EEZ generated files
│
├── CMakeLists.txt                      # Project-level build config
├── sdkconfig.defaults                  # Default ESP-IDF configuration
└── dependencies.lock                   # Component version lock (from EEZ/IDF)
```

### Component Dependencies
- `gdem102f91-epaper-driver` (local component, relative path: `../../gdem102f91-epaper-driver`)
- `esp-lvgl-epaper-port` (local component, relative path: `../../esp-lvgl-epaper-port`)
- `lvgl` (v9.2 from ESP Component Registry)
- `esp_http_client` (ESP-IDF built-in)
- `nvs_flash` (ESP-IDF built-in)
- `esp_sntp` (ESP-IDF built-in)

### Task Architecture

| Task Name | Core | Priority | Stack Size | Purpose |
|-----------|------|----------|------------|---------|
| `wifi_task` | 0 | 5 | 4KB | Manage WiFi connection, reconnection logic |
| `time_sync_task` | 0 | 3 | 3KB | SNTP sync on startup, periodic re-sync |
| `ha_client_task` | 0 | 4 | 6KB | HTTP requests to Home Assistant API |
| `lvgl_task` | 1 | 6 | 4KB | LVGL timer handler (`lv_timer_handler()`) |
| `display_task` | 1 | 5 | 4KB | Coordinate e-paper refreshes, handle 20s blocking flush |
| `power_mgmt_task` | - | 2 | 2KB | Monitor power state, future sleep/wake logic |

### Shared Resources (mutex-protected)

```c
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
```

---

## 3. Configuration Management & NVS

### NVS Storage Schema

The configuration manager stores settings in NVS namespace `"ha_dashboard"`:

| Key | Type | Default | Description |
|-----|------|---------|-------------|
| `wifi_ssid` | string | "" | WiFi network SSID |
| `wifi_pass` | string | "" | WiFi password |
| `ha_url` | string | "" | Home Assistant URL (e.g., "http://192.168.1.100:8123") |
| `ha_token` | string | "" | Long-lived access token |
| `update_interval` | uint32 | 600 | Update interval in seconds (default: 10 min) |
| `power_mode` | uint8 | 0 | Power mode: 0=always-on, 1=deep-sleep, 2=light-sleep |
| `error_grace_period` | uint32 | 1800 | Seconds before showing error screen (default: 30 min) |

### Configuration API

```c
// config_manager.h
esp_err_t config_init(void);
esp_err_t config_set_wifi(const char *ssid, const char *password);
esp_err_t config_get_wifi(char *ssid, char *password);
esp_err_t config_set_ha_credentials(const char *url, const char *token);
esp_err_t config_get_ha_credentials(char *url, char *token);
esp_err_t config_set_update_interval(uint32_t seconds);
uint32_t config_get_update_interval(void);
esp_err_t config_set_power_mode(uint8_t mode);
uint8_t config_get_power_mode(void);
```

### Initial Configuration

For Phase 1, configuration will be set via:
1. **Menuconfig defaults** (Kconfig) - for development
2. **Hardcoded fallbacks** in `config_manager.c` - if NVS is empty
3. **Future:** Serial console menu or web portal for end-user setup

The system will:
- Initialize NVS on first boot
- Load configuration or use defaults
- Validate required fields (wifi_ssid, ha_url, ha_token must not be empty)
- Log warning if using default/hardcoded values

---

## 4. Network Tasks (Core 0)

### WiFi Task

**Responsibilities:**
- Initialize WiFi in station mode
- Connect using NVS credentials
- Handle disconnect/reconnect with exponential backoff
- Set event bits for other tasks to monitor

**Behavior:**
- On startup: attempt connection, retry up to 5 times with backoff (1s, 2s, 4s, 8s, 16s)
- On disconnect: automatic reconnection attempts
- Sets `WIFI_CONNECTED_BIT` in event group when connected
- Updates `dashboard_state.wifi_connected` via mutex

**Interface:**
```c
void wifi_task(void *pvParameters);
// Depends on: config_manager (for credentials)
// Updates: s_wifi_event_group, dashboard_state.wifi_connected
```

### Time Sync Task

**Responsibilities:**
- Initialize SNTP client on WiFi connection
- Sync time from NTP servers (e.g., `pool.ntp.org`)
- Provide reliable system time for logging and display
- Periodic re-sync (every 24 hours)

**Behavior:**
- Waits for `WIFI_CONNECTED_BIT`
- Configures SNTP with multiple servers
- Blocks until first sync completes (or 30s timeout)
- Then sleeps for 24h between re-syncs
- Sets `TIME_SYNCED_BIT` in event group

**Interface:**
```c
void time_sync_task(void *pvParameters);
// Depends on: WiFi connected
// Updates: system time (settimeofday), event group TIME_SYNCED_BIT
```

### Home Assistant Client Task

**Responsibilities:**
- Fetch date/time (and future: weather, sensors) from HA REST API
- Authenticate using long-lived token
- Parse JSON responses
- Update shared dashboard state
- Handle errors with retry logic and grace period

**Behavior:**
- Waits for `WIFI_CONNECTED_BIT` and `TIME_SYNCED_BIT`
- Sleeps for `config_get_update_interval()` seconds between updates
- On wake: HTTP GET to `{ha_url}/api/states/sensor.date` and `sensor.time`
- Parses JSON, extracts state values
- Updates `dashboard_state` with mutex
- On failure: retry 3 times with 5s delay, increment `failed_update_count`
- If `failed_update_count * update_interval > error_grace_period`: set error flag

**HTTP Request Example:**
```
GET /api/states/sensor.date HTTP/1.1
Host: homeassistant.local:8123
Authorization: Bearer YOUR_LONG_LIVED_TOKEN
Content-Type: application/json
```

**Response Parsing:**
```json
{
  "entity_id": "sensor.date",
  "state": "2026-06-24",
  "attributes": { ... }
}
```

**Interface:**
```c
void ha_client_task(void *pvParameters);
esp_err_t ha_fetch_entity(const char *entity_id, char *state_buf, size_t buf_len);
// Depends on: WiFi connected, time synced, config (HA URL & token)
// Updates: dashboard_state (date_str, time_str, last_successful_update, failed_update_count)
```

---

## 5. UI & Display Tasks (Core 1)

### LVGL Task

**Responsibilities:**
- Initialize LVGL library and e-paper display adapter
- Run LVGL timer handler every 5ms
- Handle LVGL mutex for thread-safety
- Load EEZ Studio generated UI

**Behavior:**
- One-time initialization:
  - Initialize LVGL (`lv_init()`)
  - Register e-paper display driver (via `esp-lvgl-epaper-port`)
  - Call EEZ Studio `ui_init()` to load generated screens
  - Create initial dashboard screen
- Main loop:
  - Acquire LVGL mutex
  - Call `lv_timer_handler()`
  - Release mutex
  - Sleep 5ms
  
**Interface:**
```c
void lvgl_task(void *pvParameters);
// Depends on: e-paper driver, LVGL port, EEZ UI (ui.c/h)
// Updates: LVGL UI elements via EEZ framework
```

### Display Task

**Responsibilities:**
- Monitor `dashboard_state` for changes
- Update EEZ variables when data changes (via `eez_vars.c`)
- Trigger e-paper refresh (blocking ~20s)
- Handle refresh timing and coordination

**Behavior:**
- Polls `dashboard_state` every 1 second
- Compares current values with last displayed values
- On change detected:
  - Acquire LVGL mutex
  - Call EEZ variable update functions (e.g., `eez_set_date()`, `eez_set_time()`)
  - Call `lv_refr_now()` to trigger LVGL refresh
  - E-paper flush callback blocks for ~20s
  - Release mutex
  - Update "last displayed" cache
- Special case: first display after boot (always refresh)

**Interface:**
```c
void display_task(void *pvParameters);
// Depends on: dashboard_state (via mutex), eez_vars, LVGL initialized
// Updates: e-paper display via EEZ variables
```

**Error Display Logic:**
- If `dashboard_state.failed_update_count * update_interval > error_grace_period`:
  - Call `eez_show_error_screen()` 
  - Update error status variables (WiFi status, HA status, last update time)
- Once connectivity restored:
  - Reset `failed_update_count`
  - Call `eez_show_dashboard_screen()`

### EEZ Studio Integration

**Generated Files (from EEZ Studio):**
```
main/ui/
├── screens.c/h          # Screen definitions
├── styles.c/h           # Style definitions  
├── images.c/h           # Image assets
├── vars.h               # Variable declarations (generated)
├── actions.h            # Action function declarations (generated)
└── ui.c/h               # Main UI framework
```

**Implementation Files:**
```
main/
├── eez_vars.c/h         # Variable definitions and update functions
└── eez_actions.c/h      # Action implementations (mostly empty for this project)
```

**eez_vars.c Interface Example:**
```c
// eez_vars.c - implements variables declared in ui/vars.h
#include "ui/vars.h"

// Define the actual LVGL variables
lv_obj_t *var_date_label;
lv_obj_t *var_time_label;
lv_obj_t *var_wifi_status_icon;
lv_obj_t *var_ha_status_icon;

// Update functions called by display_task
void eez_set_date(const char *date);
void eez_set_time(const char *time);
void eez_set_wifi_status(bool connected);
void eez_show_error_screen(void);
void eez_show_dashboard_screen(void);
```

**eez_actions.c:**
- Implements actions declared in `ui/actions.h`
- Since there's no touch interface, most actions will be empty stubs
- Included for completeness with EEZ Studio workflow

### Power Management Task

**Responsibilities:**
- Monitor power mode configuration
- Execute power-saving strategies (Phase 1: minimal, just monitoring)
- Future: deep sleep scheduling, wake timers

**Phase 1 Behavior (Always-On Mode):**
- Reads `config_get_power_mode()`
- If mode == 0 (always-on): does nothing, just sleeps
- Logs current power mode on startup
- Placeholder for future sleep logic

**Future Modes (not implemented in Phase 1):**
- Mode 1 (Deep Sleep): Calculate next wake time, enter deep sleep, wake on timer
- Mode 2 (Light Sleep): Enter light sleep between updates, maintain WiFi

**Interface:**
```c
void power_mgmt_task(void *pvParameters);
// Depends on: config (power_mode)
// Future: controls sleep/wake cycles
```

---

## 6. Data Flow & Synchronization

### Startup Sequence

```
1. main() initializes NVS, loads configuration
2. Initialize shared resources (mutexes, event groups, dashboard_state)
3. Create all tasks with core affinity:
   - Core 0: wifi_task, time_sync_task, ha_client_task, power_mgmt_task
   - Core 1: lvgl_task, display_task
4. wifi_task connects to WiFi → sets WIFI_CONNECTED_BIT
5. time_sync_task waits for WiFi → syncs SNTP → sets TIME_SYNCED_BIT
6. lvgl_task initializes display and loads EEZ UI
7. ha_client_task waits for WiFi+Time → fetches HA data
8. display_task detects data changes → updates UI → refreshes e-paper
```

### Normal Operation Data Flow

```
[Every 10 minutes]
ha_client_task:
  1. HTTP GET /api/states/sensor.date
  2. HTTP GET /api/states/sensor.time
  3. Parse JSON responses
  4. Acquire dashboard_state_mutex
  5. Update dashboard_state.date_str, time_str
  6. Update dashboard_state.last_successful_update = now
  7. Reset dashboard_state.failed_update_count = 0
  8. Release mutex
  
[Every 1 second]
display_task:
  1. Acquire dashboard_state_mutex
  2. Compare current values with last_displayed cache
  3. Release mutex
  4. If changed:
     a. Acquire lvgl_mutex
     b. Call eez_set_date()/eez_set_time()
     c. Call lv_refr_now() → e-paper refresh (blocks 20s)
     d. Release lvgl_mutex
     e. Update last_displayed cache
```

### Mutex Hierarchy (prevent deadlocks)

- Never hold multiple mutexes simultaneously
- `dashboard_state_mutex`: Short critical sections only (read/write shared state)
- `lvgl_mutex`: Held during LVGL API calls and e-paper refresh (can be long ~20s)
- **Rule:** Always release `dashboard_state_mutex` before acquiring `lvgl_mutex`

### Event Group Bits

```c
#define WIFI_CONNECTED_BIT  BIT0
#define TIME_SYNCED_BIT     BIT1
#define HA_ERROR_BIT        BIT2  // Set when grace period exceeded
```

---

## 7. Error Handling & Recovery

### WiFi Connection Failures

- **Initial connection:** Retry 5 times with exponential backoff (1s, 2s, 4s, 8s, 16s)
- **After successful connection:** ESP-IDF WiFi stack handles auto-reconnect
- **Display impact:** Show "WiFi disconnected" icon, keep last known data visible
- **Logging:** Log all connection attempts and failures with timestamps

### SNTP Sync Failures

- **Initial sync:** Retry 3 times with 10s delay, 30s timeout per attempt
- **If all retries fail:** Log error, proceed anyway (system time will be inaccurate)
- **Periodic re-sync:** Every 24 hours, silent failure (just log warning)
- **Display impact:** None (time continues from last sync or RTC)

### Home Assistant API Failures

**Retry Logic (per request):**
1. Attempt HTTP request
2. If fails (network error, timeout, HTTP error):
   - Retry up to 3 times with 5s delay between attempts
3. If all retries fail:
   - Increment `dashboard_state.failed_update_count`
   - Log error with HTTP status code / error details
   - Keep existing data displayed

**Grace Period Logic:**
```c
time_t grace_period = config_get_error_grace_period();  // default 1800s (30 min)
time_t time_since_last_success = now - dashboard_state.last_successful_update;
uint32_t failed_count = dashboard_state.failed_update_count;

if (time_since_last_success > grace_period) {
    // Show error screen
    xEventGroupSetBits(s_event_group, HA_ERROR_BIT);
}
```

**Error Screen Display:**
- Triggered when grace period exceeded
- Shows:
  - "Home Assistant Unreachable" title
  - WiFi status: Connected / Disconnected
  - Last successful update: "2026-06-24 14:30"
  - Failed attempts: "5"
  - Current time (from SNTP)
- Automatically returns to dashboard when:
  - HA request succeeds
  - `failed_update_count` reset to 0
  - `HA_ERROR_BIT` cleared

### HTTP Client Error Categories

| Error Type | HTTP Code | Action |
|------------|-----------|--------|
| Network error | N/A | Retry 3x, increment fail count |
| Timeout | N/A | Retry 3x, increment fail count |
| Unauthorized | 401 | Log error, retry 3x (token may be temp invalid) |
| Not Found | 404 | Log error, don't retry (entity doesn't exist) |
| Server Error | 500-599 | Retry 3x, increment fail count |
| Success | 200 | Parse response, reset fail count |

### JSON Parse Failures

- Log the raw response body
- Treat as failed request (increment fail count)
- Don't update dashboard_state

### E-Paper Driver Failures

- If `epaper_refresh()` returns error:
  - Log error with driver-specific code
  - Retry once after 5s delay
  - If still fails: continue operation, try again on next update
  - Don't block system operation

### Logging Strategy

Use ESP-IDF logging with appropriate levels:
- `ESP_LOGE`: Connection failures, API errors, driver errors
- `ESP_LOGW`: Retries, timeouts, degraded operation
- `ESP_LOGI`: Successful operations, state changes
- `ESP_LOGD`: Detailed data (HTTP responses, JSON parsing)
- Include timestamps in all logs (from SNTP time)

---

## 8. Testing Strategy

### Component-Level Testing

Since this is an embedded ESP32 project, testing will be primarily integration-based rather than unit tests. However, we can structure the code to be testable:

**Testable Components:**

1. **config_manager**: Can be tested on host with NVS emulation
   - Test read/write of all config values
   - Test default fallbacks
   - Test validation (empty strings, invalid values)

2. **JSON parsing (in ha_client_task)**: Extract to separate function
   ```c
   esp_err_t parse_ha_entity_response(const char *json, char *state_out, size_t len);
   ```
   - Test with valid JSON responses
   - Test with malformed JSON
   - Test with missing fields

3. **eez_vars functions**: Can be tested with LVGL simulator on host
   - Test variable updates
   - Test screen switching

### Integration Testing (on-device)

**Phase 1 Test Plan:**

1. **Initial Boot Test:**
   - Flash device, observe serial output
   - Verify: NVS init, config load, all tasks created
   - Verify: WiFi connects, SNTP syncs, HA request succeeds
   - Verify: Display shows date/time within 2 minutes of boot

2. **Update Cycle Test:**
   - Let device run for 30+ minutes
   - Verify: Updates occur at configured interval
   - Verify: Display refreshes only when data changes
   - Verify: Logs show successful HTTP requests

3. **WiFi Recovery Test:**
   - Disconnect WiFi access point mid-operation
   - Verify: Device detects disconnect, attempts reconnect
   - Verify: Last known data remains on display
   - Reconnect WiFi
   - Verify: Device reconnects, resumes updates

4. **HA Unreachable Test:**
   - Stop Home Assistant or block network access
   - Verify: Failed requests logged
   - Verify: Data remains displayed during grace period
   - Wait for grace period to expire
   - Verify: Error screen appears with correct status
   - Restore HA connectivity
   - Verify: Returns to dashboard, updates resume

5. **Power Cycle Test:**
   - Unplug device, wait 10s, plug back in
   - Verify: Device boots, connects, displays data
   - Verify: Configuration persists from NVS

6. **Configuration Change Test:**
   - Modify update_interval in NVS
   - Reboot device
   - Verify: New interval takes effect

### Manual Validation Checklist

- [ ] Display shows correct date in HA's timezone
- [ ] Display shows correct time in HA's timezone
- [ ] Time updates every minute (when time changes)
- [ ] Date updates at midnight
- [ ] WiFi icon reflects connection status
- [ ] HA icon reflects API connectivity
- [ ] Error screen appears after grace period
- [ ] Error screen shows accurate "last successful update" timestamp
- [ ] E-paper refresh takes ~20s (expected)
- [ ] No screen tearing or corruption
- [ ] All 4 colors render correctly (black, white, yellow, red)

### Development Testing Tools

- Serial monitor with timestamps for log analysis
- Wireshark/tcpdump to capture HTTP traffic
- HA Developer Tools to verify entity states match display
- Power meter to validate power consumption (future battery mode)

---

## 9. Build Configuration & Dependencies

### ESP-IDF Configuration (`sdkconfig.defaults`)

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

### Component Dependencies (`main/idf_component.yml`)

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

### Project CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(epaper-homeassistant-dashboard)
```

### Main Component CMakeLists.txt (`main/CMakeLists.txt`)

```cmake
idf_component_register(
    SRCS 
        "main.c"
        "Tasks/wifi_task.c"
        "Tasks/time_sync_task.c"
        "Tasks/ha_client_task.c"
        "Tasks/lvgl_task.c"
        "Tasks/display_task.c"
        "Tasks/power_mgmt_task.c"
        "Shared/shared_resources.c"
        "Shared/config_manager.c"
        "eez_vars.c"
        "eez_actions.c"
        # EEZ Studio generated files will be added here
        "ui/screens.c"
        "ui/styles.c"
        "ui/images.c"
        "ui/ui.c"
        
    INCLUDE_DIRS 
        "."
        "Tasks"
        "Shared"
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
)
```

### Kconfig Options (`main/Kconfig.projbuild`)

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

    config HA_DASHBOARD_LOG_LEVEL
        int "Dashboard Log Level"
        default 3
        range 0 5
        help
            Log level: 0=None, 1=Error, 2=Warn, 3=Info, 4=Debug, 5=Verbose

endmenu
```

### Build & Flash Commands

```bash
# Setup ESP-IDF environment (v6.0)
source ~/esp/esp-idf/export.sh

# Configure project (first time)
idf.py menuconfig

# Build
idf.py build

# Flash and monitor
idf.py -p /dev/cu.usbmodem113201 flash monitor

# Clean build
idf.py fullclean
```

### Git Structure

```gitignore
build/
sdkconfig
sdkconfig.old
dependencies.lock
managed_components/
.DS_Store
```

---

## 10. Future Expandability Considerations

The architecture is designed to easily add more Home Assistant entities:

1. Add new fields to `dashboard_state_t` (e.g., `temperature`, `humidity`, `uv_index`)
2. Add fetch calls in `ha_client_task` (e.g., `ha_fetch_entity("sensor.temperature", ...)`)
3. Add corresponding variables in EEZ Studio UI
4. Add update functions in `eez_vars.c` (e.g., `eez_set_temperature()`)
5. Update `display_task` to monitor new fields

**No changes needed to:**
- WiFi task
- Time sync task
- Power management task
- Core synchronization mechanisms

**Planned Future Entities:**
- Weather: `sensor.weather_temperature`, `sensor.weather_condition`, `sensor.weather_forecast`
- Air Quality: `sensor.air_quality_index`, `sensor.pm25`
- UV Index: `sensor.uv_index`
- Geyser: `sensor.geyser_temperature`, `sensor.geyser_power`
- Solar: `sensor.battery_voltage`, `sensor.battery_soc`, `sensor.solar_power`
- Pool: `sensor.pool_temperature`, `sensor.pool_ph`, `sensor.pool_chlorine`

---

## Conclusion

This design provides a solid foundation for Phase 1 (date/time display) while maintaining clear extension points for future sensor integrations. The task-based architecture with core affinity ensures network operations don't interfere with UI updates, and the EEZ Studio integration provides a professional UI development workflow.

Next step: Create detailed implementation plan.
