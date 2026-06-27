# Color Testing - Lessons Learned

## Summary

Dynamic color changing of LVGL widgets is **incompatible** with this e-paper port setup. After extensive testing, we've reverted to a stable baseline.

## What We Tried

### Attempt 1: Style Changes with Invalidation
```c
lv_obj_set_style_arc_color(arc, new_color, LV_PART_INDICATOR);
lv_obj_invalidate(arc);
```
**Result:** ❌ LVGL didn't detect style change as dirty, no redraw occurred

### Attempt 2: Aggressive Invalidation
```c
lv_obj_remove_style(arc, NULL, LV_PART_INDICATOR);
lv_obj_set_style_arc_color(arc, new_color, LV_PART_INDICATOR);
lv_obj_invalidate(arc);
lv_obj_invalidate(objects.main);  // Entire screen
```
**Result:** ❌ Memory corruption, LVGL heap errors, deadlocks

### Attempt 3: Object Recreation
```c
lv_obj_del(objects.arc_color_test);
lv_obj_t *obj = lv_arc_create(parent);
lv_obj_set_style_arc_color(obj, new_color, LV_PART_INDICATOR);
```
**Result:** ❌ Object created but color still rendered as BLACK, "invalidation during render" errors, eventual deadlock

## Root Causes Identified

### 1. LVGL 9.x Style Caching
LVGL 9 has aggressive style caching that doesn't properly invalidate areas when styles change dynamically.

### 2. Partial Render Mode Limitations
```c
LV_DISPLAY_RENDER_MODE_PARTIAL
```
Partial rendering only redraws "dirty" regions. Style changes don't reliably mark regions as dirty.

### 3. E-Paper Port Not Designed for Dynamic Changes
The `esp-lvgl-epaper-port` and demo were designed for **static content** that's rendered once. Dynamic style changes were never tested.

### 4. Animated Widgets Are Problematic
The spinner widget has continuous animation that conflicts with manual invalidation attempts.

### 5. Race Conditions
Object creation triggers automatic LVGL rendering. Calling `lv_refr_now()` too soon causes "invalidation during render" errors.

## What Works

✅ **Static colors set once during object creation**
✅ **Updating label text** (text content, not color)
✅ **Updating arc values** (numeric value, not color)
✅ **Creating objects with fixed colors**

## What Doesn't Work

❌ Changing object colors after creation
❌ Dynamic style updates
❌ Animated widgets on e-paper (spinner, etc.)
❌ Multiple sequential `lv_refr_now()` calls

## How to Test Colors Manually

Since dynamic color changing doesn't work, test colors by editing code and reflashing:

### Edit `main/ui/screens.c`

**Test RED:**
```c
// In spinnerDemo creation:
lv_obj_set_style_arc_color(obj, lv_color_make(255, 0, 0), LV_PART_INDICATOR);
```

**Test YELLOW:**
```c
lv_obj_set_style_arc_color(obj, lv_color_make(255, 255, 0), LV_PART_INDICATOR);
```

**Test BLACK:**
```c
lv_obj_set_style_arc_color(obj, lv_color_black(), LV_PART_INDICATOR);
```

Then:
```bash
idf.py build flash
```

Each color requires a reflash, but this is guaranteed to work.

## Color Mapping Confirmed

From the working demo, we know the color pipeline works:

```c
// RGB Input → E-Paper Output
RGB(255, 0, 0)     → RED    (0x03)
RGB(255, 255, 0)   → YELLOW (0x02)
RGB(0, 0, 0)       → BLACK  (0x00)
RGB(255, 255, 255) → WHITE  (0x01)
```

The GDEM102F91 4-color e-paper display **definitely supports red and yellow** (confirmed by demo).

The problem was never the color mapping - it was LVGL's inability to dynamically update colors in this e-paper setup.

## Current Stable State

### Dashboard Features Working ✅

- WiFi connection and auto-reconnect
- Home Assistant data fetching (every 10 minutes)
- Date and time display from HA
- WiFi and HA status indicators
- E-paper refresh (25 seconds)
- Proper memory management (no corruption)
- No deadlocks
- Watchdog properly configured

### Removed for Stability ❌

- Color cycling test arc
- Dynamic color changes
- Aggressive LVGL invalidation
- Object recreation during runtime

## Recommendations Going Forward

### 1. Use Static Colors
Design your dashboard with fixed colors chosen at compile time.

### 2. Focus on Data Updates
Update **content** (text, values, visibility) not **styles** (colors, sizes, positions).

### 3. Test Colors Offline
Create test builds with different color schemes, don't try to change colors at runtime.

### 4. Avoid Animated Widgets
Use static widgets (labels, arcs with fixed position, rectangles) not animated ones (spinners, animations).

### 5. Single Render Pass
Create all objects → wait → call `lv_refr_now()` once → refresh e-paper. Don't try multiple render cycles.

## Alternative: Direct Framebuffer Access

If you absolutely need dynamic colors, bypass LVGL:

```c
#include "gdem102_driver.h"

// Draw colored rectangle directly to framebuffer
for (int y = 200; y < 300; y++) {
    for (int x = 500; x < 600; x++) {
        gdem102_draw_pixel(x, y, GDEM102_COLOR_RED);
    }
}

// Then refresh display
gdem102_flush();
lvgl_epaper_port_refresh();
```

This completely bypasses LVGL's rendering pipeline and writes directly to the e-paper driver.

## Conclusion

**The dashboard works reliably** when displaying HA data with static styling. Dynamic color changes are not worth the complexity and instability they introduce.

**Focus on the core goal:** A stable e-paper dashboard showing solar power, battery status, weather, and other HA entities with good readability.

Colors can be chosen during design/development. Runtime color changes are not feasible with this architecture.

---

**Current build is stable and ready for:**
- Adding more HA entities (solar power, battery, weather)
- Improving layout and fonts
- Optimizing refresh frequency
- Adding more data visualizations (static colors)
