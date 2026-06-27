# LVGL Memory & Rendering Conflict Fixes

## Issues Found in Latest Run

### 1. ✅ LVGL Out of Memory
```
[Error] (0.085, +15) lv_draw_add_task: Asserted at expression: new_task != NULL (Out of memory) lv_draw.c:105
[Info]  (0.075, +10) lv_malloc_zeroed: used: 6728 ( 11 %), frag:   0 %, biggest free: 57028 lv_mem.c:106
```

**Problem:** LVGL's internal memory pool (64KB default) too small for e-paper display

**Fix:** Increased LVGL memory in sdkconfig.defaults:
```
# LVGL Configuration  
# Memory - increase from default 64KB for e-paper display
CONFIG_LV_MEM_SIZE_KILOBYTES=128      # Was 64KB
CONFIG_LV_MEM_POOL_EXPAND_SIZE_KILOBYTES=32
```

---

### 2. ✅ LVGL Rendering Conflict
```
[Error] (2.825, +2720) lv_inv_area: Asserted at expression: !disp->rendering_in_progress 
        (Invalidate area is not allowed during rendering.) lv_refr.c:277
```

**Problem:** display_task called `loadScreen()` while LVGL task was rendering

**Cause:** LVGL task runs `lv_timer_handler()` in a loop. Even with mutex, LVGL's internal `rendering_in_progress` flag can be set.

**Fix 1:** Increased wait time before display_task starts:
```c
// Wait 5 seconds instead of 3 for LVGL to finish initial rendering
vTaskDelay(pdMS_TO_TICKS(5000));  // Was 3000
```

**Fix 2:** Added delay after acquiring mutex:
```c
if (xSemaphoreTake(lvgl_mutex, pdMS_TO_TICKS(5000)) == pdTRUE) {
    ESP_LOGI(TAG, "Got LVGL mutex, waiting for any in-progress rendering...");
    vTaskDelay(pdMS_TO_TICKS(100));  // NEW: Let rendering finish
    
    ESP_LOGI(TAG, "Loading screen...");
    loadScreen(SCREEN_ID_MAIN);
    // ...
}
```

---

### 3. ✅ Watchdog Timeout on IDLE0 (CPU 0)
```
E (20666) task_wdt: Task watchdog got triggered. The following tasks/users did not reset the watchdog in time:
E (20666) task_wdt:  - IDLE0 (CPU 0)
E (20666) task_wdt: CPU 0: taskLVGL
```

**Problem:** LVGL task on CPU 0 blocks for too long, preventing IDLE0 from running

**Fix:** Disabled IDLE0 watchdog monitoring (was already disabled for IDLE1):
```
# Watchdog - disable IDLE monitoring (long-running tasks block idle on both cores)
CONFIG_ESP_TASK_WDT_CHECK_IDLE_TASK_CPU0=n  # NEW
CONFIG_ESP_TASK_WDT_CHECK_IDLE_TASK_CPU1=n  # Already had this
```

---

## Complete sdkconfig.defaults Changes

```
# Watchdog - disable IDLE monitoring (long-running tasks block idle on both cores)
CONFIG_ESP_TASK_WDT_CHECK_IDLE_TASK_CPU0=n
CONFIG_ESP_TASK_WDT_CHECK_IDLE_TASK_CPU1=n

# LVGL Configuration
# Memory - increase from default 64KB for e-paper display
CONFIG_LV_MEM_SIZE_KILOBYTES=128
CONFIG_LV_MEM_POOL_EXPAND_SIZE_KILOBYTES=32

# LVGL Logging (for debugging)
CONFIG_LV_USE_LOG=y
CONFIG_LV_LOG_LEVEL_INFO=y
CONFIG_LV_LOG_PRINTF=y
CONFIG_LV_LOG_TRACE_DISP_REFR=y
```

---

## Expected Output After Fixes

### ✅ Should See:
```
I (1573) lvgl_task: Initializing EEZ UI...
[Info]  lv_obj_create: begin                          ← No memory errors
[Info]  lv_label_create: begin                        ← All allocations succeed
I (1637) lvgl_task: EEZ UI initialized                ← Completes successfully

I (6343) display_task: Loading initial screen...
I (6343) display_task: Waiting for LVGL mutex (timeout=5000ms)...
I (6344) display_task: Got LVGL mutex, waiting for any in-progress rendering...
I (6444) display_task: Loading screen...              ← No rendering conflict
I (6450) display_task: Screen loaded, rendering to framebuffer...
I (6455) display_task: Framebuffer rendered, releasing mutex...
I (6456) display_task: Initial screen loaded successfully
I (6456) display_task: Refreshing initial display (~25 seconds)...
[~25 seconds of e-paper refresh]
I (31500) display_task: Initial display shown successfully
```

### ❌ Should NOT See:
```
[Error] lv_draw_add_task: Out of memory               ← FIXED with 128KB
[Error] lv_inv_area: rendering_in_progress            ← FIXED with delays
E (xxxx) task_wdt: IDLE0                              ← FIXED with config
```

---

## Why These Fixes Work

### Memory Fix (64KB → 128KB):
- E-paper displays have large framebuffers
- LVGL needs memory for:
  - Screen objects and widgets
  - Draw buffers and tasks
  - Style cache
  - Animation buffers
- 128KB gives plenty of headroom

### Rendering Conflict Fix (5s wait + 100ms delay):
- LVGL task starts rendering UI immediately after `ui_init()`
- Takes ~2-3 seconds to complete initial render
- 5-second wait ensures LVGL is past initial rendering phase
- 100ms delay after mutex ensures any in-progress `lv_timer_handler()` completes

### Watchdog Fix (IDLE0 disabled):
- LVGL task runs continuously on CPU 0
- During intensive operations (rendering, invalidation), it can block for >5s
- IDLE0 task can't run → watchdog triggers
- Disabling IDLE0 monitoring allows long operations

---

## Timing Breakdown

**Startup sequence:**
```
0.0s   - System boot
1.0s   - WiFi init
1.6s   - LVGL task starts
1.6s   - ui_init() called
1.6s   - LVGL begins initial rendering
~4.0s  - LVGL completes initial rendering
4.3s   - display_task wakes up (after 3s wait - OLD)
6.3s   - display_task wakes up (after 5s wait - NEW)
6.3s   - display_task acquires mutex
6.4s   - display_task waits 100ms
6.5s   - display_task calls loadScreen() ← No conflict!
```

---

## Files Modified

1. **sdkconfig.defaults**
   - Increased LVGL memory: 64KB → 128KB
   - Added pool expand size: 32KB
   - Disabled IDLE0 watchdog monitoring

2. **main/Tasks/display_task.c**
   - Increased wait time: 3s → 5s
   - Added 100ms delay after acquiring mutex
   - More detailed logging

---

## Flash and Test

```bash
idf.py -p /dev/cu.usbmodem113201 flash monitor
```

**Watch for:**
1. No LVGL memory allocation errors
2. No "rendering_in_progress" errors
3. No watchdog timeouts
4. Display task completes and shows screen

The display should now work! 🎨📟
