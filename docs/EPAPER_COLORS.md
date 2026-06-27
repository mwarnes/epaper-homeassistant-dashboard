# E-Paper Display Color Guide

## Display Specifications

**Model:** GDEM102F91 (Good Display)  
**Resolution:** 960×640 pixels  
**Colors:** 4-color (Black, White, Red, Yellow)  
**Refresh Time:** ~20 seconds

## Available Colors

### In LVGL Code

```c
lv_color_black()           // Black - Main text
lv_color_white()           // White - Background
lv_color_make(255, 0, 0)   // Red - Errors/warnings
lv_color_make(255, 255, 0) // Yellow - Alerts/info
```

### Color Usage Guidelines

#### ✅ Use Black For:
- Normal status text
- Primary content (date, time)
- "OK" status indicators
- Body text
- Headers

#### ✅ Use Red For:
- Error messages
- Disconnected states
- Critical alerts
- "ERROR" status indicators

#### ✅ Use Yellow For:
- Warnings (not critical)
- Informational highlights
- Loading states
- Temporary messages

#### ✅ Use White For:
- Background
- Inverted text areas
- Highlight backgrounds

## Current Implementation

### Status Indicators

**WiFi Status:**
- Connected: `"WiFi: OK"` in **Black**
- Disconnected: `"WiFi: ERROR"` in **Red**

**Home Assistant Status:**
- Connected: `"HA: OK"` in **Black**
- Disconnected: `"HA: ERROR"` in **Red**

**Time/Date:**
- Normal: **Black** text
- Error state: **Red** text with error message

## Color Limitations

### ❌ What Doesn't Work:
- **Green** - Will be dithered to black/white/yellow mix
- **Blue** - Will be dithered to black/white mix
- **Purple, Orange, etc.** - Limited dithering results
- **Gradients** - Not supported, use solid colors only

### Display Behavior:
When you use unsupported colors:
- Colors are dithered/converted to nearest available color
- Green (0, 255, 0) becomes a mix of black/yellow/white
- May not be readable or look as intended

## Best Practices

### 1. Keep It Simple
```c
// Good
lv_color_black()  // Clear, readable

// Bad - will be dithered
lv_color_make(0, 255, 0)  // Green doesn't exist on this display
```

### 2. High Contrast
- Black on White: Excellent readability
- Red on White: Good for errors
- Yellow on White: Use sparingly, lower contrast
- Avoid: Black on Yellow, Red on Black

### 3. Status Colors
```c
// Normal status
lv_obj_set_style_text_color(obj, lv_color_black(), 0);

// Error status
lv_obj_set_style_text_color(obj, lv_color_make(255, 0, 0), 0);

// Warning status
lv_obj_set_style_text_color(obj, lv_color_make(255, 255, 0), 0);
```

### 4. Background Colors
```c
// White background (default)
lv_obj_set_style_bg_color(obj, lv_color_white(), 0);

// Black background (inverted)
lv_obj_set_style_bg_color(obj, lv_color_black(), 0);
lv_obj_set_style_text_color(obj, lv_color_white(), 0);
```

## Testing Colors

When testing on the actual display:
1. **Black** should be very dark/black
2. **White** should be light gray/white background
3. **Red** should be distinct red color
4. **Yellow** should be distinct yellow/orange color

### Common Issues:
- **Ghosting**: Previous image remains faint - normal for e-paper
- **Color mixing**: Multiple colors in one area may show artifacts
- **Refresh artifacts**: Normal during 20-second refresh cycle

## EEZ Studio Configuration

When designing in EEZ Studio:
1. Set text color to Black for normal text
2. Set text color to Red for error states
3. Use Yellow sparingly for highlights
4. Set background to White (default)
5. Preview won't show e-paper limitations - test on device

## Performance Considerations

### Refresh Time
- Full screen refresh: ~20 seconds
- Changing colors requires full refresh
- Plan updates accordingly (current: 10 minutes)

### Power Usage
- Color changes consume more power
- Red/Yellow pixels use different voltages
- Minimize unnecessary color changes

## Code Examples

### Status Indicator with Color Change
```c
void update_status(lv_obj_t *label, bool is_ok) {
    if (is_ok) {
        lv_label_set_text(label, "Status: OK");
        lv_obj_set_style_text_color(label, lv_color_black(), 0);
    } else {
        lv_label_set_text(label, "Status: ERROR");
        lv_obj_set_style_text_color(label, lv_color_make(255, 0, 0), 0);
    }
}
```

### Warning Message
```c
lv_obj_t *warning = lv_label_create(parent);
lv_label_set_text(warning, "Warning: Low Battery");
lv_obj_set_style_text_color(warning, lv_color_make(255, 255, 0), 0);
```

### Error Box
```c
lv_obj_t *error_box = lv_obj_create(parent);
lv_obj_set_style_bg_color(error_box, lv_color_make(255, 0, 0), 0);
lv_obj_set_style_text_color(error_box, lv_color_white(), 0);

lv_obj_t *error_label = lv_label_create(error_box);
lv_label_set_text(error_label, "Connection Failed");
```

## Summary

**Remember:** This is a 4-color e-paper display!
- ✅ Use: Black, White, Red, Yellow
- ❌ Avoid: Green, Blue, Purple, gradients
- 📏 Keep high contrast: Black on White, Red on White
- ⏱️ Account for 20-second refresh time
