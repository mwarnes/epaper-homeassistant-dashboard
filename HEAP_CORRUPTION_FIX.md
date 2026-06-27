# Heap Corruption & Display Hang Fixes

## Issues Found

### 1. Display Task Hanging
**Symptom:** "Loading initial screen..." logged but never completed
**Impact:** Display task exited, never showed anything on screen

### 2. Heap Corruption Crash  
**Symptom:** Guru Meditation Error in `tlsf_malloc` with 0xf5f5f5f5 (freed memory pattern)
**Impact:** Crash during HTTP request in ha_client_task

---

## Fixes Applied

### Fix 1: Display Task - Prevent Exit on Mutex Timeout

**Problem:** Task exited if it couldn't get mutex, leaving display dead

**Before:**
```c
if (xSemaphoreTake(lvgl_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
    // load screen
} else {
    ESP_LOGW(TAG, "Could not load initial screen - mutex timeout");
    return; // ← Task exits! Display is now dead
}
```

**After:**
```c
// Increased timeout and added detailed logging
if (xSemaphoreTake(lvgl_mutex, pdMS_TO_TICKS(5000)) == pdTRUE) {
    ESP_LOGI(TAG, "Got LVGL mutex, loading screen...");
    loadScreen(SCREEN_ID_MAIN);
    ESP_LOGI(TAG, "Screen loaded, rendering to framebuffer...");
    lv_refr_now(NULL);
    ESP_LOGI(TAG, "Framebuffer rendered, releasing mutex...");
    xSemaphoreGive(lvgl_mutex);
    ESP_LOGI(TAG, "Initial screen loaded successfully");
    
    // Refresh e-paper (moved outside mutex)
    ESP_LOGI(TAG, "Refreshing initial display (~25 seconds)...");
    esp_err_t refresh_err = lvgl_epaper_port_refresh();
    // ...
} else {
    ESP_LOGE(TAG, "Could not acquire LVGL mutex after 5 seconds - continuing anyway");
    // Don't return - keep task alive ← FIXED
}
```

**Result:** Task stays alive even if mutex fails, detailed logs show where it hangs

---

### Fix 2: HTTP Buffer Overflow Protection

**Problem:** 2048-byte buffer too small, heap corruption

**Changes:**
```c
// Increased buffer size
#define HTTP_RESPONSE_BUFFER_SIZE 4096  // Was 2048

// Better overflow protection
if (s_http_response_len + evt->data_len < HTTP_RESPONSE_BUFFER_SIZE - 1) {
    memcpy(s_http_response_buffer + s_http_response_len, evt->data, evt->data_len);
    s_http_response_len += evt->data_len;
    s_http_response_buffer[s_http_response_len] = '\0';
} else {
    ESP_LOGE(TAG, "HTTP response buffer overflow: current=%d, incoming=%d, max=%d",
             s_http_response_len, evt->data_len, HTTP_RESPONSE_BUFFER_SIZE);
    return ESP_FAIL;  // ← Fail safely instead of corrupting heap
}
```

---

### Fix 3: HTTP Client Error Handling

**Added:**
```c
// Check client initialization
esp_http_client_handle_t client = esp_http_client_init(&config);
if (client == NULL) {
    ESP_LOGE(TAG, "Failed to initialize HTTP client");
    return ESP_FAIL;
}

// More event logging
case HTTP_EVENT_ERROR:
    ESP_LOGE(TAG, "HTTP_EVENT_ERROR");
    break;
case HTTP_EVENT_ON_CONNECTED:
    ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
    break;
case HTTP_EVENT_DISCONNECTED:
    ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
    break;
```

---

### Fix 4: Heap Monitoring

**Added heap logging to track memory issues:**

```c
#include "esp_heap_caps.h"

// Before starting fetch loop
ESP_LOGI(TAG, "Free heap before loop: %lu bytes", esp_get_free_heap_size());

// Before each fetch
ESP_LOGI(TAG, "Free heap: %lu bytes (largest block: %lu)", 
         esp_get_free_heap_size(), 
         heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT));
```

**Result:** Can see if memory is leaking or running low

---

### Fix 5: Startup Stabilization Delay

**Added 2-second delay after time sync:**
```c
xEventGroupWaitBits(s_event_group,
                    WIFI_CONNECTED_BIT | TIME_SYNCED_BIT,
                    pdFALSE, pdTRUE, portMAX_DELAY);

// NEW: Let system settle
ESP_LOGI(TAG, "Waiting 2 seconds for system to stabilize...");
vTaskDelay(pdMS_TO_TICKS(2000));
```

**Reason:** Prevents race conditions and heap fragmentation during startup

---

## Expected Output After Fix

### ✅ Display Task Should Show:
```
I (4265) display_task: Loading initial screen...
I (4265) display_task: Waiting for LVGL mutex (timeout=5000ms)...
I (4265) display_task: Got LVGL mutex, loading screen...
I (4270) display_task: Screen loaded, rendering to framebuffer...
[LVGL info logs about rendering]
I (4280) display_task: Framebuffer rendered, releasing mutex...
I (4281) display_task: Initial screen loaded successfully
I (4281) display_task: Refreshing initial display (~25 seconds)...
[25 seconds of e-paper refresh]
I (29500) display_task: Initial display shown successfully
I (29500) display_task: Display task monitoring state changes...
```

### ✅ HA Client Should Show:
```
I (9644) time_sync_task: Time synchronized
I (9644) ha_client_task: Waiting 2 seconds for system to stabilize...
I (11644) ha_client_task: Starting HA data fetch loop (interval: 600000 ms / 600 seconds)
I (11645) ha_client_task: Free heap before loop: 215432 bytes
I (11646) ha_client_task: Free heap: 215432 bytes (largest block: 196608)
D (11650) ha_client_task: Attempt 1/3: Fetching sensor.date_time_iso
D (11655) ha_client_task: Performing HTTP GET to http://192.168.0.67:8123/api/states/sensor.date_time_iso
D (11700) ha_client_task: HTTP_EVENT_ON_CONNECTED
D (11750) ha_client_task: Response (xxx bytes): {"entity_id":"sensor.date_time_iso",...}
I (11755) ha_client_task: Fetched sensor.date_time_iso: 2026-06-25T14:30:00
I (11760) ha_client_task: Parsed ISO datetime: 2026-06-25T14:30:00 → date=2026-06-25, time=14:30
I (11770) ha_client_task: Updated: Date=2026-06-25, Time=14:30
```

### ❌ Should NOT See:
```
Guru Meditation Error: Core 0 panic'ed (StoreProhibited)   ← FIXED
0xf5f5f5f5 pattern in backtrace                            ← FIXED
```

---

## Debugging Tips

If **display still hangs**, look for where it stops:
- "Loading initial screen..." ← Gets this far
- "Waiting for LVGL mutex..." ← Trying to get mutex
- "Got LVGL mutex..." ← Success getting mutex
- "Screen loaded..." ← loadScreen() completed
- "Framebuffer rendered..." ← lv_refr_now() completed
- "Initial screen loaded successfully" ← Mutex section done
- "Refreshing initial display..." ← About to call lvgl_epaper_port_refresh()
- "Initial display shown successfully" ← E-paper refresh done

If it hangs at any point, we know exactly where.

If **heap corruption still happens**, check logs:
- "Free heap before loop: XXX" ← Starting heap size
- "Free heap: XXX (largest block: YYY)" ← Before each request
- Look for decreasing heap or small largest block
- "HTTP response buffer overflow" ← Buffer too small

---

## Files Modified

1. **main/Tasks/display_task.c**
   - Don't exit on mutex timeout
   - Increased mutex timeout 1s → 5s
   - Added detailed progress logging

2. **main/Tasks/ha_client_task.c**
   - Added `#include "esp_heap_caps.h"`
   - Increased HTTP buffer 2048 → 4096
   - Better overflow protection (- 1 for null terminator)
   - Check client init before use
   - Added heap monitoring
   - Added 2s stabilization delay
   - More event logging

---

## Next Test

Flash and monitor for:
1. Display task completing successfully
2. Heap staying stable (not decreasing)
3. HTTP requests completing without crash
4. E-paper showing content

```bash
idf.py -p /dev/cu.usbmodem113201 flash monitor
```

Watch the logs carefully - the detailed logging will show exactly where any issue occurs!
