# EEZ Studio Widget Integration Guide

Your EEZ Studio project currently builds successfully with a basic layout. To complete the Home Assistant dashboard functionality, you need to add the following widgets:

## Current Status

✅ **Working:**
- Screen: `main` (SCREEN_ID_MAIN)
- Widget: `lblDate` (objects.lbl_date) - displays date from HA

⚠️ **Missing (currently just logged, not displayed):**
- Time display widget
- WiFi status indicator
- Home Assistant status indicator
- Error screen

## Widgets to Add in EEZ Studio

### 1. Time Label

**Widget:** Label  
**Name in EEZ Studio:** `lblTime`  
**Access in code:** `objects.lbl_time`

**Properties:**
- Position: Below or next to `lblDate`
- Initial text: "00:00:00"
- Font: Same as date or slightly smaller

**Code integration (already prepared in `eez_vars.c`):**
```c
// Uncomment when widget is added:
if (objects.lbl_time && time) {
    lv_label_set_text(objects.lbl_time, time);
}
```

### 2. WiFi Status Label/Icon

**Widget:** Label  
**Name in EEZ Studio:** `lblWifiStatus`  
**Access in code:** `objects.lbl_wifi_status`

**Properties:**
- Position: Bottom-left corner
- Initial text: "WiFi: ?"
- Font: Smaller font (e.g., Montserrat 12)
- Color changes based on status (green=OK, red=error)

**Code integration (already prepared in `eez_vars.c`):**
```c
// Uncomment when widget is added:
if (objects.lbl_wifi_status) {
    lv_label_set_text(objects.lbl_wifi_status, connected ? "WiFi: OK" : "WiFi: X");
    lv_obj_set_style_text_color(objects.lbl_wifi_status,
                                 connected ? lv_color_make(0, 255, 0) : lv_color_make(255, 0, 0),
                                 0);
}
```

### 3. Home Assistant Status Label/Icon

**Widget:** Label  
**Name in EEZ Studio:** `lblHaStatus`  
**Access in code:** `objects.lbl_ha_status`

**Properties:**
- Position: Bottom-right corner
- Initial text: "HA: ?"
- Font: Smaller font (e.g., Montserrat 12)
- Color changes based on status (green=OK, red=error)

**Code integration (already prepared in `eez_vars.c`):**
```c
// Uncomment when widget is added:
if (objects.lbl_ha_status) {
    lv_label_set_text(objects.lbl_ha_status, connected ? "HA: OK" : "HA: X");
    lv_obj_set_style_text_color(objects.lbl_ha_status,
                                 connected ? lv_color_make(0, 255, 0) : lv_color_make(255, 0, 0),
                                 0);
}
```

### 4. Error Screen (Optional but Recommended)

**Screen:** New screen  
**Name in EEZ Studio:** `error`  
**Screen ID:** `SCREEN_ID_ERROR`

**Widgets on error screen:**
- `lblErrorTitle` - "Home Assistant Unreachable"
- `lblErrorWifiStatus` - Shows WiFi connection state
- `lblErrorHaStatus` - Shows HA connection state  
- `lblErrorLastUpdate` - Shows last successful update time

**Code integration:**
When you add the error screen, update `eez_vars.c`:
```c
void eez_show_error_screen(...) {
    // Update error screen widgets
    if (objects.lbl_error_title) {
        lv_label_set_text(objects.lbl_error_title, "HA Error");
    }
    // ... set other labels
    
    // Switch to error screen
    loadScreen(SCREEN_ID_ERROR);
}
```

## Steps to Add Widgets

1. **Open your EEZ Studio project**
2. **Select the `main` screen**
3. **Add widgets from the palette:**
   - Drag a Label widget onto the screen
   - In the properties panel, set the **Identifier** field
   - Set position, size, font, etc.
4. **Regenerate code:**
   - File → Build (or Ctrl+B)
   - This regenerates `screens.c`, `screens.h`, etc.
5. **Copy updated files to your ESP32 project:**
   ```bash
   cp -r [eez-project]/ui/* main/ui/
   ```
6. **Rebuild ESP32 project:**
   ```bash
   /tmp/build-with-v6.sh
   ```

## Widget Naming Convention

When you add a widget in EEZ Studio:
- **Identifier** (in properties): `lblTime`, `lblWifiStatus`, etc.
- **Access in C code**: `objects.lbl_time`, `objects.lbl_wifi_status`

EEZ Studio automatically converts:
- `lblTime` → `objects.lbl_time`
- `spinnerDemo` → `objects.spinner_demo`
- `btnRefresh` → `objects.btn_refresh`

(CamelCase → snake_case)

## Current Widget Structure

From your `screens.h`:
```c
typedef struct _objects_t {
    lv_obj_t *main;           // Main screen
    lv_obj_t *spinner_demo;   // Spinner widget
    lv_obj_t *lbl_date;       // Date label ✅ WORKING
    // Add more widgets here as you create them
} objects_t;

extern objects_t objects;
```

## Testing Individual Widgets

After adding each widget:

1. **Test in EEZ Studio simulator** (if available)
2. **Rebuild and flash to ESP32:**
   ```bash
   /tmp/build-with-v6.sh
   idf.py -p /dev/ttyUSB0 flash monitor
   ```
3. **Check logs** - the widget updates are logged:
   ```
   I (1234) eez_vars: Updated date: 2026-06-25
   I (1235) eez_vars: Time update: 10:30:00 (no UI widget yet)
   ```

## Recommended Layout

```
┌─────────────────────────────────────┐
│     Home Assistant Dashboard        │
│                                     │
│          2026-06-25                │ ← lblDate
│           10:30:45                 │ ← lblTime
│                                     │
│         (spinner while loading)     │
│                                     │
│  WiFi: OK              HA: OK      │ ← status indicators
└─────────────────────────────────────┘
```

## 4-Color E-Paper Display

Your display supports:
- Black
- White  
- Red
- Yellow

Consider using:
- **Green text** (via dithering) or **White** for OK status
- **Red** for errors
- **Yellow** for warnings
- **Black** for main text

## Next Steps

1. Add the missing widgets in EEZ Studio
2. Set their Identifiers correctly
3. Regenerate the code
4. Copy to `main/ui/`
5. Uncomment the relevant sections in `eez_vars.c`
6. Rebuild and test!

The integration code is already written - you just need to create the widgets in EEZ Studio! 🎨
