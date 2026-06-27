# Color Debug Test - Finding the Red/Yellow Issue

## The Problem

- ✅ **BLACK** renders correctly on e-paper
- ❌ **RED** appears invisible (white?)
- ❓ **YELLOW** status unknown

## Color Mapping in GDEM102 Driver

The e-paper uses 2-bit color values:
```c
GDEM102_COLOR_BLACK  = 0x00  // Pure black pigment
GDEM102_COLOR_WHITE  = 0x01  // No pigment (paper color)
GDEM102_COLOR_YELLOW = 0x02  // Yellow pigment
GDEM102_COLOR_RED    = 0x03  // Red pigment
```

## Color Detection Logic (in lvgl_epaper_port)

```c
// Yellow: High R+G, low B
if (r > 150 && g > 150 && b < 80) → YELLOW

// Red: High R, low G+B  
if (r > 150 && g < 80 && b < 80) → RED

// Black: Dark overall
if (brightness < 80) → BLACK

// Default: Everything else
→ WHITE
```

## What We're Testing

**Code sends:**
```c
lv_color_make(255, 0, 0)  // Pure red: RGB(255, 0, 0)
```

**Should match RED detection:**
- r=255 > 150 ✓
- g=0 < 80 ✓
- b=0 < 80 ✓
- **Result: Should return GDEM102_COLOR_RED (0x03)**

## Debug Logging Added

I've added logging to `rgb_to_epaper_color()` function to show the first 10 non-white color conversions:

```c
ESP_LOGI: Color conversion: RGB(r,g,b) brightness=X -> COLOR_NAME
```

## Test Instructions

```bash
idf.py -p /dev/cu.usbmodem113201 flash monitor
```

## What to Look For in Logs

### 1. Initial Display (BLACK arc)
```
I (xxxx) lvgl_epaper: Color conversion: RGB(0,0,0) brightness=0 -> BLACK
```

### 2. First Refresh (RED arc)
**Expected:**
```
I (xxxx) eez_vars: Arc color changed to RED (cycle 1)
I (xxxx) lvgl_epaper: Color conversion: RGB(255,0,0) brightness=85 -> RED
```

**If we see instead:**
```
I (xxxx) lvgl_epaper: Color conversion: RGB(255,255,255) brightness=255 -> WHITE
```
→ Problem: LVGL is converting our color to white before flush

**If we see:**
```
I (xxxx) lvgl_epaper: Color conversion: RGB(255,0,0) brightness=85 -> RED
```
But arc still invisible on display → Problem: Driver not rendering red pigment

**If we see no log at all:**
→ Problem: Arc pixels might not be dirty/updating

### 3. Second Refresh (YELLOW arc)
**Expected:**
```
I (xxxx) eez_vars: Arc color changed to YELLOW (cycle 2)
I (xxxx) lvgl_epaper: Color conversion: RGB(255,255,0) brightness=170 -> YELLOW
```

## Possible Issues

### Theory 1: LVGL Style Override
LVGL might be applying default styles that override our color:
- Arc background might be white
- Indicator might have opacity issues
- Theme colors might interfere

### Theory 2: Color Format Conversion
RGB888 → e-paper conversion might have bugs in certain color ranges

### Theory 3: Anti-aliasing
LVGL might be blending colors for smooth edges, creating intermediate RGB values that don't match our detection thresholds

### Theory 4: Display Driver Issue
The GDEM102 driver might not be properly activating red/yellow pigment layers

## Next Steps Based on Results

**If RGB values are correct but color is wrong:**
→ Adjust detection thresholds in `rgb_to_epaper_color()`

**If RGB values are white/wrong:**
→ Check LVGL style system and color inheritance

**If no conversion logs appear:**
→ Arc might not be in the dirty region / not triggering refresh

**If conversion shows RED but display shows white:**
→ Check GDEM102 driver's color layer activation

---

## Quick Fix If Thresholds Are Wrong

If we see something like `RGB(248, 0, 0)` not matching, we can adjust:

```c
// More lenient red detection
if (r > 128 && g < 100 && b < 100) {
    return GDEM102_COLOR_RED;
}
```

Flash and monitor - let's see what the logs reveal! 🔍
