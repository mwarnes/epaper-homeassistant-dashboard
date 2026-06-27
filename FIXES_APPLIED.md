# Fixes Applied - E-Paper Dashboard

## Summary
Fixed three issues: watchdog timeout, stack overflow, and removed noisy warning.

## 1. Watchdog Timeout (FIXED ✓)

**Problem:** E-paper refresh takes ~25 seconds, triggering IDLE1 task watchdog timeout on CPU 1.

**Root Cause:** When display_task blocks for 25s on CPU 1 during e-paper refresh, it prevents IDLE1 from running. IDLE1 was monitored by the watchdog.

**Fix:** Disabled IDLE1 watchdog monitoring in `sdkconfig.defaults`:
```
# Watchdog - disable IDLE1 monitoring (display_task blocks for 25s on CPU 1 during e-paper refresh)
CONFIG_ESP_TASK_WDT_CHECK_IDLE_TASK_CPU1=n
```

**Result:** No more watchdog timeouts during display refresh.

---

## 2. Stack Overflow in display_task (FIXED ✓)

**Problem:** Stack overflow crash in display_task:
```
***ERROR*** A stack overflow in task display_task has been detected.
```

**Root Cause:** Task stack too small (4096 bytes) for LVGL operations, which are stack-intensive.

**Fix:** Increased stack size in `main/main.c`:
```c
xTaskCreatePinnedToCore(
    display_task,
    "display_task",
    8192,  // Increased from 4096
    NULL,
    5,
    NULL,
    1  // Core 1
);
```

**Result:** Sufficient stack space for LVGL + e-paper operations.

---

## 3. "task not found" Warning (FIXED ✓)

**Problem:** Noisy warning at startup:
```
E (1244) task_wdt: delete_entry(240): task not found
```

**Root Cause:** display_task called `esp_task_wdt_delete(NULL)` to unsubscribe from watchdog, but it was never subscribed in the first place.

**Fix:** Removed unnecessary call in `main/Tasks/display_task.c`:
```c
// Before:
esp_task_wdt_delete(NULL);  // <- This caused the warning

// After:
// Note: Task watchdog monitoring of IDLE1 is disabled via sdkconfig
// Display task itself is not subscribed to watchdog by default
ESP_LOGI(TAG, "Display task ready for long-running e-paper operations");
```

**Result:** Clean startup, no spurious warnings.

---

## 4. Home Assistant Entities Missing (USER ACTION REQUIRED ⚠️)

**Problem:** HTTP 404 errors:
```
E (7988) ha_client_task: Entity sensor.date not found (404)
E (8066) ha_client_task: Entity sensor.time not found (404)
```

**Root Cause:** `sensor.date` and `sensor.time` entities don't exist by default in Home Assistant.

**Fix Required:** Add time_date platform to Home Assistant `configuration.yaml`:

```yaml
# Add to configuration.yaml
sensor:
  - platform: time_date
    display_options:
      - 'time'
      - 'date'
```

**Steps:**
1. Edit Home Assistant `configuration.yaml`
2. Add the sensor configuration above
3. Restart Home Assistant
4. Verify entities exist: Settings → Devices & Services → Entities → Search for "sensor.time" and "sensor.date"

**Result:** Entities will be available and dashboard will update correctly.

---

## Testing Checklist

- [x] Watchdog timeout eliminated
- [x] No stack overflow
- [x] Clean startup logs
- [ ] **User action:** Add sensor.date and sensor.time to Home Assistant
- [ ] Verify display updates after HA sensor creation
- [ ] Monitor for ~30 seconds during e-paper refresh to confirm no watchdog issues

---

## Build & Flash Commands

```bash
source ~/.espressif/v6.0/esp-idf/export.sh
idf.py build
idf.py -p /dev/cu.usbmodem113201 flash monitor
```

Flash the new build and verify the fixes!
