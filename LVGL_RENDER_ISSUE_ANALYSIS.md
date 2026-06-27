# LVGL Render Issue - Root Cause Analysis

## The Problem

**Symptom:** Arc color changes from BLACK to RED, but display stays BLACK.

**Evidence from logs:**

### Initial Render (WORKS) ✅
```
I (6267) eez_vars: Arc color changed to BLACK - R=0 G=0 B=0
I (6349) lvgl_epaper: Color conversion: RGB(33,33,33) -> BLACK [×10 logs]
I (7669) display_task: Refreshing e-paper display (~25 seconds)...
```
- lv_refr_now() triggers flush callback
- Arc pixels are converted and rendered
- Display updates

### Color Change (FAILS) ❌
```
I (33980) eez_vars: Arc color changed to RED - R=255 G=0 B=0
I (34082) display_task: Calling lv_refr_now()...
I (34092) display_task: lv_refr_now() completed  ← Only 10ms!
I (34593) display_task: Refreshing e-paper display (~25 seconds)...
[NO COLOR CONVERSION LOGS]
```
- lv_refr_now() completes in 10ms (should take longer if rendering)
- Flush callback NEVER CALLED
- No pixels converted = arc not redrawn
- Display shows old BLACK color

## Root Cause

**LVGL's dirty tracking is not detecting the style change as requiring a redraw.**

When we call:
```c
lv_obj_set_style_arc_color(arc, red, LV_PART_INDICATOR);
lv_obj_invalidate(arc);
lv_obj_invalidate(objects.main);  // Even invalidating whole screen!
```

LV GL should mark those areas dirty, but `lv_refr_now()` still decides there's nothing to render.

## Demo vs Our App

| Demo (Works) | Our App (Broken) |
|--------------|------------------|
| Creates UI once | Creates UI once ✓ |
| Sets colors once | Sets colors once ✓ |
| Calls lv_refr_now() once | Calls lv_refr_now() once ✓ |
| Never changes objects | **Changes styles dynamically** ✗ |
| - | Second lv_refr_now() does nothing ✗ |

**The demo NEVER changes object styles after initial render.** Our app is the first to try dynamic style changes with this e-paper port.

## Theories

### Theory 1: LVGL 9.x Style Caching
LVGL 9 has aggressive style caching. Changing a style might update the cache but not mark the rendered output as dirty.

### Theory 2: Partial Render Mode Issue
```c
LV_DISPLAY_RENDER_MODE_PARTIAL
```
In partial mode, LVGL only renders "dirty" regions. Style changes might not properly mark regions dirty.

### Theory 3: E-Paper Optimization
The lvgl_epaper_port might have optimizations that prevent re-rendering of already-rendered areas.

### Theory 4: Render Completion State
After first render, LVGL might think the display is "complete" and ignore subsequent invalidations until a "real" change (like widget creation/destruction).

## Attempted Fixes (All Failed)

1. ✗ `lv_obj_invalidate(arc)` - Should mark dirty, doesn't work
2. ✗ `lv_obj_remove_style()` then re-add - Should force change detection, doesn't work
3. ✗ Hide/show arc - Should force redraw, doesn't work
4. ✗ `lv_obj_invalidate(main)` - Even whole screen invalidation doesn't work!

## The Nuclear Option That SHOULD Work

Instead of changing styles, **destroy and recreate the arc**:

```c
void eez_cycle_spinner_color(void) {
    // ... color selection logic ...
    
    if (objects.arc_color_test) {
        // Get parent and position
        lv_obj_t *parent = lv_obj_get_parent(objects.arc_color_test);
        
        // Destroy old arc
        lv_obj_del(objects.arc_color_test);
        
        // Create new arc with new color
        lv_obj_t *obj = lv_arc_create(parent);
        objects.arc_color_test = obj;
        lv_obj_set_pos(obj, 500, 200);
        lv_obj_set_size(obj, 100, 100);
        lv_arc_set_range(obj, 0, 100);
        lv_arc_set_value(obj, 75);
        lv_arc_set_bg_angles(obj, 0, 360);
        
        // Set NEW color
        lv_obj_set_style_arc_color(obj, color, LV_PART_INDICATOR);
        lv_obj_set_style_arc_width(obj, 10, LV_PART_INDICATOR);
        lv_obj_set_style_arc_width(obj, 0, LV_PART_MAIN);
        lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, LV_PART_KNOB);
    }
}
```

**Why this should work:**
- Object creation ALWAYS marks area as dirty
- No style cache to bypass
- Forces complete redraw of that region

## Alternative: Use Direct Pixel Writing

Skip LVGL entirely for the color test:

```c
// In display_task, before refresh:
if (lvgl_port_lock(0)) {
    // Get display buffer
    lv_display_t *disp = lvgl_epaper_port_get_display();
    lv_draw_buf_t *buf = lv_display_get_buf_active(disp);
    
    // Draw colored rectangle directly to buffer
    for (int y = 200; y < 300; y++) {
        for (int x = 500; x < 600; x++) {
            // Write red pixel directly
            // (buffer format specific code)
        }
    }
    
    lvgl_port_unlock();
}
```

## Next Step: Simplest Possible Test

Let's test if **object recreation** works:

```bash
idf.py -p /dev/cu.usbmodem113201 flash monitor
```

The current build has `lv_obj_invalidate(objects.main)` which should invalidate EVERYTHING. If this doesn't produce "Color conversion" logs, then LVGL's invalidation system is completely broken for our use case, and we need the nuclear option (recreate objects).

**Expected with current build:**
```
I (xxxx) eez_vars: Arc color changed to RED (cycle 1) - R=255 G=0 B=0
I (xxxx) eez_vars: Invalidated entire screen for redraw
I (xxxx) display_task: Calling lv_refr_now()...
```

**If we see "Color conversion" logs** → Whole-screen invalidation works, arc redraw issue
**If NO "Color conversion" logs** → LVGL invalidation broken, need object recreation

---

**Flash and test, then I'll implement object recreation if needed.**
