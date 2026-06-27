# 🚨 IMPORTANT: Latest Fix Applied

## Critical Change

**Screen loading has been moved out of ui_init()** to prevent watchdog timeout.

### What Changed

**Before:** Screen loaded during `ui_init()` → caused 5+ second style operations → watchdog timeout

**After:** Screen created but not loaded during `ui_init()` → loaded by display_task after 3s delay

### Files Modified

- `main/ui/ui.c` - Removed `loadScreen()` call from `ui_init()`
- `main/Tasks/display_task.c` - Added initial screen load after delay

### Flash Command

```bash
idf.py -p /dev/ttyUSB0 flash monitor
```

### What You Should See

```
I (1527) lvgl_task: Initializing EEZ UI...
I (1530) lvgl_task: EEZ UI initialized          ← Should complete in <1s now!
I (3208) display_task: Display task starting
I (6208) display_task: Loading initial screen...
I (6210) display_task: Initial screen loaded    ← 3s later, no timeout!
```

### Why This Works

The LVGL screen load process triggers:
1. `lv_scr_load()` 
2. → `lv_obj_set_pos()` for all widgets
3. → `lv_obj_refresh_style()` for each widget
4. → `lv_obj_invalidate()` for each widget
5. → Style calculations and area invalidations

On e-paper displays, these operations are slow because:
- Each invalidation queries display properties
- E-paper port has slower rendering pipeline
- Total time >5 seconds → watchdog timeout!

**Solution:** Do the expensive screen load in a separate task (display_task) which:
- Starts after LVGL is fully initialized
- Holds the LVGL mutex properly
- Runs on Core 1 (won't block Core 0 WiFi/HA tasks)
- Has time to complete before watchdog triggers

### Commit

```
1e2200c fix: defer screen loading to avoid watchdog timeout during ui_init
```

### Build Status

- ✅ Compiles successfully
- Binary: ~1.27 MB
- **FLASH THIS VERSION NOW**

### If It Still Crashes

If you still see watchdog timeout, check:

1. **Which task?** Look for `CPU 1: lvgl_task` or `CPU 1: display_task`
2. **Where in backtrace?** Send the full backtrace
3. **Timing:** Does it happen during ui_init or later?

The fix should work because:
- `ui_init()` now only creates screens (fast)
- Screen load happens 3s later when system is stable
- Mutex is properly held
- Task has watchdog configured

🎯 **This should finally fix the watchdog timeout!**
