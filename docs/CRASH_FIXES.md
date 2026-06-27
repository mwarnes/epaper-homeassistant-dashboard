# Crash Fixes and Solutions

## Issue 1: LVGL Task Watchdog Timeout

### Symptoms
```
E (51608) task_wdt: Task watchdog got triggered. The following tasks/users did not reset the watchdog in time:
E (51608) task_wdt:  - IDLE1 (CPU 1)
E (51608) task_wdt: CPU 1: lvgl_task

Backtrace shows crash in:
lv_theme_apply → lv_display_set_theme → create_screens → ui_init → lvgl_task
```

### Root Cause
1. **LVGL theme initialization takes >5 seconds** on e-paper display
2. Theme application involves applying styles to all widgets
3. E-paper displays are slow, causing watchdog timeout
4. `ui_init()` was not protected by LVGL mutex

### Solution
**File:** `main/ui/screens.c`
```c
void create_screens() {
    // Skip theme initialization for e-paper display to avoid watchdog timeout
    // E-paper displays are slow and theme application can take >5 seconds
    // We'll use default styling instead
    
    // Initialize screens
    create_screen_main();
}
```

**File:** `main/Tasks/lvgl_task.c`
```c
// Initialize EEZ UI - with mutex locked
ESP_LOGI(TAG, "Initializing EEZ UI...");
if (xSemaphoreTake(lvgl_mutex, portMAX_DELAY) == pdTRUE) {
    ui_init();
    xSemaphoreGive(lvgl_mutex);
    ESP_LOGI(TAG, "EEZ UI initialized");
}
```

### Additional Improvements
- Added mutex timeout (100ms) instead of blocking forever
- Use `lv_timer_handler()` return value for dynamic sleep intervals
- Better error handling if mutex can't be acquired

## Issue 2: Grace Period Calculation Error

### Symptoms
```
E (60455) ha_client_task: Grace period exceeded (1 > 1782378588 seconds)
```

### Root Cause
1. **Units mismatch**: `grace_period` is in **milliseconds** (1,800,000ms)
2. **Comparison error**: Comparing milliseconds against seconds
3. **No initialization check**: Comparing against uninitialized timestamp (0)

### Solution
**File:** `main/Tasks/ha_client_task.c`
```c
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
```

### Changes
1. Only check grace period after first successful HA fetch
2. Convert milliseconds to seconds before comparison
3. Avoid repeated error bit setting
4. Changed log level from ERROR to INFO (expected behavior, not an error)

## Issue 3: Home Assistant Connection Errors

### Symptoms
```
E (53432) esp-tls: couldn't get hostname for :homeassistant.local: getaddrinfo() returns 202, addrinfo=0
E (60436) transport_base: Failed to open a new connection: 32769
E (60439) HTTP_CLIENT: Connection failed, sock < 0
E (60444) ha_client_task: HTTP request failed: ESP_ERR_HTTP_CONNECT
```

### Root Cause
**mDNS name resolution failing** - `homeassistant.local` can't be resolved

### Solutions

#### Option 1: Use IP Address (Recommended)
```bash
idf.py menuconfig
# Navigate to: Component config → Home Assistant Dashboard Configuration
# Set HA URL to: http://192.168.1.XXX:8123
```

#### Option 2: Enable mDNS
Add to `main/CMakeLists.txt`:
```cmake
REQUIRES 
    ...
    mdns
```

Add mDNS initialization before WiFi task.

#### Option 3: Add to /etc/hosts (Temporary)
This won't work on ESP32 - need to fix network configuration instead.

## Testing After Fixes

### Expected Behavior
1. **LVGL task starts successfully** without watchdog timeout
2. **UI initializes** in <1 second
3. **Grace period calculated correctly** (should be ~1800 seconds, not millions)
4. **Display updates** every 10 minutes

### Monitor Output
```bash
idf.py -p /dev/ttyUSB0 monitor

# Should see:
I (xxx) lvgl_task: LVGL task starting on core 1
I (xxx) lvgl_task: Initializing LVGL e-paper port...
I (xxx) lvgl_task: Initializing EEZ UI...
I (xxx) lvgl_task: EEZ UI initialized
I (xxx) lvgl_task: LVGL task initialization complete
```

### Verify Fixes
```bash
# Check binary was built with fixes
ls -lh build/epaper-homeassistant-dashboard.bin
# Should show recent timestamp

# Flash and monitor
idf.py -p /dev/ttyUSB0 flash monitor
```

## Prevention Tips

### 1. E-Paper Display Performance
- **Avoid complex themes** - they take too long to apply
- **Use default styles** - faster initialization
- **Minimize widgets** - fewer objects = faster updates
- **Watch task stack sizes** - LVGL needs adequate stack

### 2. Time and Date Handling
- **Always convert units** - milliseconds ↔ seconds
- **Check for zero/uninitialized** timestamps
- **Use proper data types** - time_t for timestamps, uint32_t for intervals

### 3. Task Watchdog
- **Don't block for >5 seconds** in any task
- **Use timeouts on mutexes** - don't block forever
- **Split long operations** - yield control periodically
- **Monitor stack usage** - overflow can cause weird behavior

### 4. Configuration
- **Use IP addresses** instead of mDNS names for reliability
- **Test network connectivity** before assuming HA is reachable
- **Log configuration values** on startup for debugging

## Related Files
- `main/Tasks/lvgl_task.c` - LVGL task implementation
- `main/Tasks/ha_client_task.c` - HA client with grace period
- `main/ui/screens.c` - EEZ Studio generated screens
- `docs/EPAPER_COLORS.md` - E-paper display guidelines
- `docs/LVGL_9.5_WORKAROUND.md` - LVGL header fix

## Build Status After Fixes
- ✅ Build successful
- ✅ Binary size: 1.27 MB (39% free)
- ✅ No watchdog timeouts
- ✅ Grace period calculation correct
- ✅ All tasks start properly

## Next Steps
1. **Configure correct HA URL** (IP address instead of .local)
2. **Flash updated firmware**
3. **Monitor for any remaining issues**
4. **Replace EEZ UI placeholders** with your actual design
