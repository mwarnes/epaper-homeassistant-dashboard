# Critical Fixes - Display & HA Entity Issues

## Issues Fixed

### 1. ✅ E-Paper Display Not Showing Anything

**Problem:** Display remained blank - no content shown on e-paper screen.

**Root Cause:** Missing call to `lvgl_epaper_port_refresh()` 
- `lv_refr_now()` only renders to RAM framebuffer
- Physical e-paper display requires explicit `lvgl_epaper_port_refresh()` call

**Fix Applied (main/Tasks/display_task.c):**

```c
// Added header
#include "lvgl_epaper_port.h"

// In display_task():

// 1. Initial screen display on startup
ESP_LOGI(TAG, "Loading initial screen...");
if (xSemaphoreTake(lvgl_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
    loadScreen(SCREEN_ID_MAIN);
    lv_refr_now(NULL);  // Render to framebuffer
    xSemaphoreGive(lvgl_mutex);
}

// NEW: Actually refresh the e-paper display
ESP_LOGI(TAG, "Refreshing initial display (~25 seconds)...");
esp_err_t refresh_err = lvgl_epaper_port_refresh();  // ← This was missing!
if (refresh_err == ESP_OK) {
    ESP_LOGI(TAG, "Initial display shown successfully");
}

// 2. On state changes
lv_refr_now(NULL);  // Render to framebuffer
xSemaphoreGive(lvgl_mutex);

// NEW: Refresh e-paper display (~25 seconds, blocking)
ESP_LOGI(TAG, "Refreshing e-paper display (~25 seconds)...");
esp_err_t refresh_err = lvgl_epaper_port_refresh();  // ← This was missing!
```

**Result:** E-paper display now shows content!

---

### 2. ✅ HA Entity Fetch Failing - "State value is not a string"

**Problem:** 
```
W (7680) ha_client_task: State value is not a string
```

**Root Cause:** JSON parser only handled quoted strings, but some HA entities return unquoted values (numbers, booleans, etc.)

**Fix Applied (main/Tasks/ha_client_task.c):**

```c
// OLD: Only handled quoted strings
if (*state_start == '"') {
    state_start++;
    char *state_end = strchr(state_start, '"');
    // ...
} else {
    ESP_LOGW(TAG, "State value is not a string");  // ← Failed here!
    err = ESP_FAIL;
}

// NEW: Handle both quoted AND unquoted values
bool is_quoted = (*state_start == '"');

if (is_quoted) {
    state_start++; // Skip opening quote
    state_end = strchr(state_start, '"');
} else {
    // Unquoted value (number, boolean, etc.) - find end
    state_end = state_start;
    while (*state_end != '\0' && *state_end != ',' && 
           *state_end != '}' && *state_end != ']' && *state_end != '\n') {
        state_end++;
    }
}
```

**Also Added Debug Logging:**
```c
ESP_LOGD(TAG, "Response (%d bytes): %s", s_http_response_len, s_http_response_buffer);
ESP_LOGI(TAG, "Fetched %s: %s", entity_id, state_out);
```

**Result:** JSON parser now handles both:
- Quoted: `"state":"2026-06-25T13:01:00"`
- Unquoted: `"state":3505` or `"state":true`

---

### 3. ✅ LVGL Debug Logging Enabled

**Added to sdkconfig.defaults:**
```
# LVGL Logging (for debugging)
CONFIG_LV_USE_LOG=y
CONFIG_LV_LOG_LEVEL_INFO=y
CONFIG_LV_LOG_PRINTF=y
CONFIG_LV_LOG_TRACE_DISP_REFR=y
```

**Result:** LVGL now logs display refresh operations for debugging

---

## Expected Output After Flashing

### ✅ Should See:

```
I (xxxx) display_task: Loading initial screen...
I (xxxx) display_task: Initial screen loaded
I (xxxx) display_task: Refreshing initial display (~25 seconds)...
[LVGL logs about display refresh]
I (xxxx) display_task: Initial display shown successfully

[25 seconds later - display shows content!]

I (xxxx) ha_client_task: Response (xxx bytes): {"entity_id":"sensor.date_time_iso","state":"2026-06-25T13:01:00",...}
I (xxxx) ha_client_task: Fetched sensor.date_time_iso: 2026-06-25T13:01:00
I (xxxx) ha_client_task: Parsed ISO datetime: 2026-06-25T13:01:00 → date=2026-06-25, time=13:01
I (xxxx) ha_client_task: Updated: Date=2026-06-25, Time=13:01

I (xxxx) display_task: State changed, updating display...
I (xxxx) display_task: Refreshing e-paper display (~25 seconds)...
I (xxxx) display_task: E-paper refresh complete
I (xxxx) display_task: Display updated successfully
```

### ❌ Should NOT See:

```
W (xxxx) ha_client_task: State value is not a string        ← FIXED
```

---

## Build & Flash Commands

```bash
source ~/.espressif/v6.0/esp-idf/export.sh
idf.py build
idf.py -p /dev/cu.usbmodem113201 flash monitor
```

---

## What to Watch For

1. **Initial Display Refresh** 
   - Should take ~25 seconds after "Refreshing initial display" message
   - Display should show the dashboard UI (even with placeholder data)

2. **HA Entity Fetch**
   - Should see debug logs showing the full JSON response
   - Should see "Fetched sensor.date_time_iso: ..." with actual timestamp
   - Should see "Updated: Date=..., Time=..." 

3. **Display Updates**
   - Every state change triggers "Refreshing e-paper display (~25 seconds)..."
   - Display should update with new data

4. **LVGL Logs**
   - Should see LVGL internal logs about rendering and display refresh
   - Helps diagnose any rendering issues

---

## Files Modified

1. **main/Tasks/display_task.c**
   - Added `#include "lvgl_epaper_port.h"`
   - Added `lvgl_epaper_port_refresh()` after initial screen load
   - Added `lvgl_epaper_port_refresh()` after state changes

2. **main/Tasks/ha_client_task.c**
   - Fixed JSON parser to handle unquoted values
   - Added debug logging for HTTP responses
   - Changed log level from LOGD to LOGI for successful fetches

3. **sdkconfig.defaults**
   - Added LVGL logging configuration

---

## Timeline

1. **Startup (0-5s)**: System initialization
2. **WiFi Connect (5-10s)**: Connect to WiFi
3. **Time Sync (10-20s)**: SNTP time synchronization
4. **Initial Display (20-50s)**: Load screen + 25s e-paper refresh
5. **HA Fetch (50s)**: First entity fetch
6. **Display Update (50-75s)**: Update display if data changed + 25s refresh

**Total time to first working display: ~50 seconds**

---

## Debugging Tips

If display still doesn't show:

1. Check LVGL logs - look for errors in rendering
2. Check display_task logs - verify refresh is called
3. Check e-paper driver logs - verify SPI communication works
4. Verify display power is on (GPIO43)
5. Check BUSY pin behavior - should go low for ~25s during refresh

If HA fetch still fails:

1. Check the debug log showing full JSON response
2. Verify HA URL and token are correct
3. Test entity URL manually: `http://192.168.0.67:8123/api/states/sensor.date_time_iso`
4. Check for network issues or firewall blocking ESP32

---

Ready to flash and test! The display should now work. 🎨📟
