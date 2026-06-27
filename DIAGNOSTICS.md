# E-Paper Display Diagnostics Guide

## Purpose

This diagnostics mode helps isolate whether display issues (ghosting, lines, missing content) are caused by:
- Hardware defects
- Software/driver bugs
- LVGL rendering issues  
- Refresh timing problems
- Buffer corruption

## How to Enable Diagnostics Mode

### Step 1: Edit `main/main.c`

Find this section near the top of the file:

```c
// ========================================
// DIAGNOSTICS MODE
// Uncomment the line below to run display diagnostics instead of normal operation
// This will run a series of test patterns to diagnose display issues
// ========================================
// #define ENABLE_EPAPER_DIAGNOSTICS
```

Uncomment the last line:

```c
#define ENABLE_EPAPER_DIAGNOSTICS
```

### Step 2: Build and Flash

```bash
idf.py build flash monitor
```

### Step 3: Observe Tests

The system will run 9 diagnostic tests, each for 30 seconds.

**Total runtime:** ~4.5 minutes

---

## Diagnostic Tests

### Test 1: Full White Screen
**Purpose:** Baseline test  
**Expected:** Completely white display  
**Look for:** Any black lines or artifacts

🔍 **If lines appear:** Note their exact position (top/middle/bottom)

---

### Test 2: Full Black Screen
**Purpose:** Inverse baseline  
**Expected:** Completely black display  
**Look for:** White or gray lines

---

### Test 3: Horizontal Color Bars
**Display:**
- Top (0-159px): BLACK
- Upper (160-319px): WHITE
- Lower (320-479px): YELLOW
- Bottom (480-639px): RED

🔍 **Check:**
- Do lines appear only in certain color regions?
- Do lines cross bar boundaries?

---

### Test 4: Vertical Color Bars
**Display:**
- Left (0-239px): BLACK
- Mid-Left (240-479px): WHITE  
- Mid-Right (480-719px): YELLOW
- Right (720-959px): RED

🔍 **Check:**
- Are artifacts horizontal or vertical?
  - **Horizontal lines:** Scan line / row driver issue
  - **Vertical lines:** Column driver issue

---

### Test 5: Checkerboard Pattern
**Display:** 8x8 pixel alternating black/white blocks

🔍 **Check:**
- Do lines appear in a regular pattern?
- Are lines always at the same pixel rows?

---

### Test 6: Ghosting Test
**Two steps:**
1. Draw white horizontal lines on black background
2. Clear to solid white

🔍 **Check:**
- Can you see "ghost" of previous pattern after clear?
  - **YES:** Ghosting/refresh issue
  - **NO:** Lines are not ghosting-related

---

### Test 7: Pixel-by-Pixel Write Test
**Display:** Dot pattern in top 100 rows, rest white

🔍 **Check:**
- Do lines appear in patterned area or white area?
- Does pixel-level drawing show issues?

---

### Test 8: Framebuffer State Check
**Purpose:** Verify framebuffer cleared correctly  
**Monitor logs** for framebuffer state

---

### Test 9: Final Clear
Returns display to clean white state

---

## Interpretation Guide

### Lines appear ONLY after LVGL rendering (not in these tests)
**Diagnosis:** Software issue (LVGL/flush callback)  
**Action:** Check `lvgl_flush_cb()` in `esp-lvgl-epaper-port`

### Lines appear on ALL tests (even Test 1)
**Diagnosis:** Hardware defect (display panel or controller)  
**Action:** Contact display manufacturer / RMA

### Lines appear only on certain colors
**Diagnosis:** Color mapping issue in driver  
**Action:** Check `rgb_to_epaper_color()` function

### Lines disappear after double clear (Test 6)
**Diagnosis:** Ghosting/refresh timing issue  
**Action:** Add multiple clear cycles or longer delays

### Lines always at same pixel rows
**Diagnosis:** Hardware defect (stuck row drivers)  
**Action:** Display hardware issue - RMA likely needed

### Lines position varies between tests
**Diagnosis:** Software issue (buffer corruption/timing)  
**Action:** Check buffer allocation, cache coherency, SPI timing

---

## Disabling Diagnostics Mode

### Step 1: Comment out the define

```c
// #define ENABLE_EPAPER_DIAGNOSTICS
```

### Step 2: Rebuild

```bash
idf.py build flash
```

System returns to normal dashboard operation.

---

## Log Output

During diagnostics, monitor logs will show:

```
I (xxx) epaper_diag: ========================================
I (xxx) epaper_diag: E-PAPER DISPLAY DIAGNOSTICS
I (xxx) epaper_diag: ========================================
I (xxx) epaper_diag: Test 1: Full white screen (clearing test)
I (xxx) epaper_diag:   Framebuffer cleared to white
I (xxx) epaper_diag:   Physical display refreshed
I (xxx) epaper_diag:   CHECK DISPLAY: Should be completely white
I (xxx) epaper_diag:   - Are there any black lines visible? (Y/N)
I (xxx) epaper_diag:   - If YES: Note their position (top/middle/bottom)
```

---

## Common Issues and Solutions

### Issue: Lines only in top 1/3 of screen

**Possible Causes:**
1. Buffer size mismatch (LVGL buffer too small)
2. Partial flush not covering full display
3. Row driver issue in display hardware

**Tests to confirm:**
- Test 3 (Horizontal bars) - Do lines appear only in top bars?
- Test 7 (Pixel test) - Are top 100 rows affected?

### Issue: Lines appear after first refresh, disappear after second

**Diagnosis:** Inadequate clear/refresh cycle  
**Solution:** Add double-clear before rendering:

```c
gdem102_clear(GDEM102_COLOR_WHITE);
vTaskDelay(pdMS_TO_TICKS(100));
gdem102_clear(GDEM102_COLOR_WHITE);
```

### Issue: "Last updated:" label not showing

**Tests to confirm:**
- Test 1 - Does plain white screen work?
- If YES: Issue is with LVGL text rendering, not display
- Check font file is included in CMakeLists.txt
- Check label position isn't off-screen

---

## Files Modified

- `main/Tasks/epaper_diagnostics.c` - Diagnostic test suite
- `main/Tasks/epaper_diagnostics.h` - Header file
- `main/main.c` - Added diagnostics mode flag
- `main/CMakeLists.txt` - Added diagnostics.c to build

---

## Support

After running diagnostics, report findings:
1. Which tests showed lines/artifacts?
2. Screenshot or photo of each test
3. Position of lines (pixel coordinates if possible)
4. Whether lines are consistent or vary

This information will help determine if the issue is:
- ✅ Fixable in software
- ❌ Hardware defect requiring RMA
