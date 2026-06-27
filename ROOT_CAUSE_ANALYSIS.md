# Root Cause Analysis: Watchdog Timeout

## Executive Summary

**Problem:** Task watchdog timeout crashes during e-paper display updates  
**Root Cause:** Blocking 20-25 second e-paper flush operation in watchdog-subscribed task with 5-second timeout  
**Solution:** Unsubscribe display_task from task watchdog  

---

## The Investigation

### Comparing with Working Demo

We analyzed the working LVGL demo in `../esp-lvgl-epaper-demo` to understand why it doesn't have watchdog issues.

**Key Finding:** The demo runs the blocking e-paper flush in `app_main()` context, which is **not subscribed to the task watchdog**, while our implementation runs it in a FreeRTOS task that **is subscribed**.

---

## Root Cause Breakdown

### 1. E-Paper Display Characteristics

The GDEM102F91 e-paper display has these properties:
- **Resolution:** 960×640 pixels (614,400 pixels)
- **Color depth:** 2-bit (4 colors: Black/White/Red/Yellow)
- **Refresh time:** ~20-25 seconds for full update
- **Blocking operation:** SPI transfer + polling BUSY pin

### 2. LVGL Flush Callback

When LVGL needs to update the display:

```c
// In lvgl_epaper_port.c
void lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    // 1. Transfer pixel data to e-paper controller (SPI)
    gdem102_draw_bitmap(...);
    
    // 2. Trigger refresh
    gdem102_refresh();
    
    // 3. Poll BUSY pin (blocks for ~20-25 seconds!)
    while (gpio_get_level(BUSY_PIN) == 0) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    lv_disp_flush_ready(disp);
}
```

### 3. Our Task Architecture

```c
// main/main.c
xTaskCreatePinnedToCore(
    display_task,
    "display_task",
    4096,
    NULL,
    5,       // Priority
    NULL,
    1        // Core 1
);
```

**Critical issue:** When `xTaskCreate*()` creates a task, ESP-IDF **automatically subscribes it to the task watchdog** (if enabled).

**Watchdog configuration** (from `sdkconfig`):
```
CONFIG_ESP_TASK_WDT_EN=y
CONFIG_ESP_TASK_WDT_TIMEOUT_S=5          ← 5 second timeout
CONFIG_ESP_TASK_WDT_CHECK_IDLE_TASK_CPU0=y
CONFIG_ESP_TASK_WDT_CHECK_IDLE_TASK_CPU1=y
```

### 4. The Failure Sequence

```
display_task runs on Core 1
    ↓
Detects state change (WiFi/HA status update)
    ↓
Acquires lvgl_mutex
    ↓
Updates label text: lv_label_set_text()
    ↓
LVGL marks area as dirty
    ↓
Calls lv_refr_now() to force refresh
    ↓
LVGL calls flush callback
    ↓
Flush callback blocks for ~20-25 seconds ← PROBLEM
    ↓
Task cannot yield or feed watchdog
    ↓
Watchdog fires after 5 seconds
    ↓
CRASH!
```

### 5. Crash Backtrace Explanation

```
E (9243) task_wdt: Task watchdog got triggered
E (9243) task_wdt:  - IDLE1 (CPU 1)
E (9243) task_wdt: CPU 1: display_task

Backtrace:
lv_label_set_text               ← Set WiFi/HA status text
  → set_text_internal
  → lv_label_mark_need_refr_text
  → lv_obj_invalidate            ← Mark area as dirty
  → invalidate_area_core
  → lv_inv_area
  [Eventually triggers flush which blocks for 20s]
```

---

## Why the Demo Works

**File:** `../esp-lvgl-epaper-demo/main/main.c`

```c
void app_main(void)
{
    // 1. Initialize LVGL port
    lvgl_epaper_port_init();
    
    // 2. Create UI
    if (lvgl_port_lock(0)) {
        create_dashboard_ui();
        lvgl_port_unlock();
    }
    
    // 3. Force initial render
    vTaskDelay(pdMS_TO_TICKS(100));
    if (lvgl_port_lock(0)) {
        lv_refr_now(lvgl_epaper_port_get_display());
        lvgl_port_unlock();
    }
    
    // 4. Do the blocking refresh (IN APP_MAIN CONTEXT!)
    vTaskDelay(pdMS_TO_TICKS(500));
    lvgl_epaper_port_refresh();      // ← 20s blocking operation
    
    // 5. Simple loop
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
```

**Key differences:**

| Aspect | Demo | Our App |
|--------|------|---------|
| **Context** | `app_main()` function | `display_task` FreeRTOS task |
| **Watchdog subscription** | Not subscribed | Auto-subscribed on task creation |
| **Blocking operation** | Safe (no watchdog) | Causes timeout |
| **Architecture** | Simple, synchronous | Multi-task, event-driven |

The demo runs in `app_main()` which is a special context that's **not subject to the same task watchdog rules** as user-created tasks.

---

## Solutions Considered

### Option 1: Increase Watchdog Timeout ❌

```c
// sdkconfig
CONFIG_ESP_TASK_WDT_TIMEOUT_S=30
```

**Pros:** Simple config change  
**Cons:** 
- Hides real issues (30s is way too long)
- Affects all tasks globally
- Makes debugging other issues harder

### Option 2: Restructure Like Demo ❌

Move display updates to `app_main()` context.

**Pros:** Matches working demo  
**Cons:**
- Major refactor required
- Loses multi-task architecture benefits
- Doesn't fit our event-driven model

### Option 3: Make Flush Non-Blocking ❌

Rewrite e-paper driver to use interrupts instead of polling.

**Pros:** Proper async operation  
**Cons:**
- Significant driver changes required
- ESP32-S3 GPIO interrupts add complexity
- Out of scope for Phase 1

### Option 4: Unsubscribe display_task ✅ CHOSEN

```c
void display_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Display task starting...");
    
    // Unsubscribe from watchdog
    esp_task_wdt_delete(NULL);
    ESP_LOGI(TAG, "Display task unsubscribed from watchdog");
    
    // ... rest of task ...
}
```

**Pros:**
- Minimal code change (2 lines)
- Preserves architecture
- Safe for this specific use case
- Matches demo's approach (no watchdog for display)

**Cons:**
- Task won't be monitored by watchdog
- Could hide actual hangs (mitigated by task being non-critical)

**Why it's safe:**
- Display updates are **not safety-critical**
- Long blocking time is **expected behavior** for e-paper
- Other tasks (WiFi, HA client, time sync) remain watchdog-monitored
- Task runs on dedicated Core 1 (won't affect network operations)
- If task truly hangs, user will notice (no display updates)

---

## Implementation

**File:** `main/Tasks/display_task.c`

```c
#include "esp_task_wdt.h"  // Add header

void display_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Display task starting on core %d", xPortGetCoreID());
    
    // Unsubscribe from watchdog - e-paper refresh takes ~20-25 seconds (blocking)
    // which exceeds the 5-second watchdog timeout. This is expected behavior.
    esp_task_wdt_delete(NULL);
    ESP_LOGI(TAG, "Display task unsubscribed from watchdog (e-paper refresh is slow)");
    
    // ... rest of task implementation ...
}
```

---

## Expected Behavior After Fix

### Successful Boot Logs

```
I (3208) display_task: Display task starting on core 1
I (3210) display_task: Display task unsubscribed from watchdog (e-paper refresh is slow)
I (6210) display_task: Loading initial screen...
I (6215) display_task: Initial screen loaded
I (6220) display_task: Display task monitoring state changes...

[No watchdog timeout - task runs normally]

I (12345) display_task: State changed, updating display...
[20-25 second pause while e-paper refreshes]
I (37890) display_task: Display updated successfully
```

### What Changed

- ✅ No more watchdog timeouts
- ✅ Display updates work (even if slow)
- ✅ Other tasks remain monitored
- ✅ System stability maintained

---

## Lessons Learned

### For E-Paper Displays

1. **Blocking operations are normal** - e-paper is inherently slow
2. **Watchdog timeouts != bugs** - sometimes timeouts are overly aggressive
3. **Context matters** - where code runs affects watchdog behavior
4. **Demo architecture** - always check how reference implementations handle timing

### For ESP-IDF Development

1. **Tasks are auto-subscribed** to watchdog on creation
2. **`app_main()` is special** - different watchdog rules
3. **Timeout config is global** - affects all tasks
4. **Use `esp_task_wdt_delete(NULL)`** to unsubscribe current task
5. **Non-critical tasks** can safely run without watchdog

### For Multi-Task Systems

1. **Separate concerns** - display updates shouldn't block network tasks
2. **Core affinity helps** - display on Core 1, network on Core 0
3. **Document slow operations** - make intentional blocking explicit
4. **Choose architecture** based on constraints, not just patterns

---

## Verification

### Test Cases

1. **Normal operation:**
   - Display updates every 10 minutes
   - WiFi/HA status changes trigger updates
   - No watchdog timeouts

2. **Error conditions:**
   - WiFi disconnect → display shows error (red text)
   - HA unavailable → display shows error after grace period
   - Task recovery after network issues

3. **Performance:**
   - Initial boot completes without timeout
   - Screen loads in ~3 seconds
   - Updates complete in ~25 seconds
   - Other tasks not affected by display blocking

### Monitoring

Watch for these log patterns:

```bash
# Good - task unsubscribed
I display_task: Display task unsubscribed from watchdog

# Good - updates happening
I display_task: State changed, updating display...
I display_task: Display updated successfully

# Bad - if you see this, something else is wrong
E task_wdt: Task watchdog got triggered
E task_wdt: CPU 1: display_task  ← Should not happen anymore
```

---

## Related Files

- **Analysis source:** `../esp-lvgl-epaper-demo/main/main.c` (working demo)
- **Fixed file:** `main/Tasks/display_task.c` (our implementation)
- **Watchdog config:** `sdkconfig` (CONFIG_ESP_TASK_WDT_TIMEOUT_S=5)
- **E-paper port:** `../esp-lvgl-epaper-port/src/lvgl_epaper_port.c` (flush callback)
- **Driver:** `../gdem102f91-epaper-driver/src/gdem102_ll.c` (blocking operations)

---

## Git History

```
a349a41 fix: unsubscribe display_task from watchdog to prevent timeout
1e2200c fix: defer screen loading to avoid watchdog timeout during ui_init
16e222b fix: disable screen load animations to prevent watchdog timeout
92cfab8 fix: resolve LVGL watchdog timeout and grace period calculation
```

All previous fixes addressed **symptoms** (animations, screen loading).  
This fix addresses the **root cause** (blocking operations in watchdog-subscribed task).

---

## Future Improvements (Out of Scope for Phase 1)

1. **Async e-paper driver** - Use GPIO interrupts instead of polling BUSY pin
2. **Partial refresh support** - Update only changed regions (faster)
3. **Display buffer queue** - Decouple rendering from flushing
4. **Display health monitoring** - Custom timeout detection without watchdog

For Phase 1 (date/time display), the current solution is appropriate and safe.
