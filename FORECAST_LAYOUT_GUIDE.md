# 5-Day Weather Forecast Layout Guide

This guide walks you through creating a professional weather dashboard with current conditions and 5-day forecast.

## 🎨 Proposed Layout

```
┌─────────────────────────────────────────────────────────────────────┐
│  Saturday Jun 27, 2026                           [WiFi] [HA]        │
├─────────────────────────────────────────────────────────────────────┤
│                                                                      │
│   ┌──────────┐                                                      │
│   │          │   Partly cloudy                                      │
│   │  128x128 │   15.7 °C                                            │
│   │   Icon   │                                                      │
│   │          │                                                      │
│   └──────────┘                                                      │
│                                                                      │
├─────────────────────────────────────────────────────────────────────┤
│  5-Day Forecast:                                                    │
│                                                                      │
│   Sun    Mon    Tue    Wed    Thu                                   │
│   [64]   [64]   [64]   [64]   [64]   ← Icons (64x64)               │
│   22°C   24°C   21°C   23°C   25°C   ← Temps                        │
│                                                                      │
└─────────────────────────────────────────────────────────────────────┘
```

**Screen Size:** 960x640 pixels

**Layout Breakdown:**
- **Top Bar:** Date, WiFi/HA status icons
- **Current Weather:** Large 128x128 icon + condition text + temperature
- **Forecast Row:** 5 columns × (day name + icon + temperature)

---

## 📊 Required Variables in EEZ Studio

You'll need to add these variables in EEZ Studio:

### **Current Weather (Already Exists):**
- `ha_time` (String)
- `ha_date` (String)
- `ha_home_realfeel_temperature` (String)
- `ha_home_condition_today` (String)

### **5-Day Forecast (New - Add These):**

| Variable Name | Type | Description |
|---------------|------|-------------|
| `ha_forecast_day1_temp` | String | "22 °C" |
| `ha_forecast_day1_condition` | String | "Sunny" |
| `ha_forecast_day1_name` | String | "Sun" |
| `ha_forecast_day2_temp` | String | "24 °C" |
| `ha_forecast_day2_condition` | String | "Cloudy" |
| `ha_forecast_day2_name` | String | "Mon" |
| `ha_forecast_day3_temp` | String | "21 °C" |
| `ha_forecast_day3_condition` | String | "Rainy" |
| `ha_forecast_day3_name` | String | "Tue" |
| `ha_forecast_day4_temp` | String | "23 °C" |
| `ha_forecast_day4_condition` | String | "Partly cloudy" |
| `ha_forecast_day4_name` | String | "Wed" |
| `ha_forecast_day5_temp` | String | "25 °C" |
| `ha_forecast_day5_condition` | String | "Sunny" |
| `ha_forecast_day5_name` | String | "Thu" |

**Total:** 15 new variables (5 days × 3 variables)

---

## 🎨 EEZ Studio Widget Setup

### **Step 1: Add Variables**

1. Open EEZ Studio → **Variables** section
2. Add each forecast variable (click **Add Variable**):
   - Name: `ha_forecast_day1_temp`
   - Type: `string`
   - Default: `"--"`
3. Repeat for all 15 variables

### **Step 2: Create Current Weather Section**

**Large Weather Icon:**
- Widget: **Image**
- Name: `img_weather_current`
- Position: `(50, 80)` (left side)
- Size: `128x128`
- Default: `weather_partly_cloudy`

**Condition Label:**
- Widget: **Label**
- Name: `ha_lbl_home_condition_today` (already exists)
- Position: `(200, 100)`
- Font: `roboto_med_24`
- Text: Bound to `ha_home_condition_today`

**Temperature Label:**
- Widget: **Label**
- Name: `ha_lbl_home_realfeel_temperature` (already exists)
- Position: `(200, 140)`
- Font: `roboto_med_48`
- Text: Bound to `ha_home_realfeel_temperature`

### **Step 3: Create Forecast Row**

For each day (1-5), create a vertical group:

**Day 1 (x=100):**
- **Day Name Label:**
  - Name: `lbl_forecast_day1_name`
  - Position: `(100, 280)`
  - Font: `roboto_med_18`
  - Text: Bound to `ha_forecast_day1_name`
  - Align: Center

- **Weather Icon:**
  - Name: `img_forecast_day1`
  - Position: `(84, 310)` (centered under label)
  - Size: `64x64`
  - Default: `weather_sunny`

- **Temperature Label:**
  - Name: `lbl_forecast_day1_temp`
  - Position: `(100, 385)`
  - Font: `roboto_med_20`
  - Text: Bound to `ha_forecast_day1_temp`
  - Align: Center

**Day 2 (x=250):**
- Repeat above with `day2` names, x position = 250

**Day 3 (x=400):**
- Repeat above with `day3` names, x position = 400

**Day 4 (x=550):**
- Repeat above with `day4` names, x position = 550

**Day 5 (x=700):**
- Repeat above with `day5` names, x position = 700

**Spacing:** 150 pixels between each forecast column

---

## 🖼️ Icon Requirements

### **Current Weather Icon (Large):**
- Size: **128x128 pixels**
- Format: **Indexed** (smallest)
- Import in EEZ Studio as `weather_*_large` (e.g., `weather_sunny_large`)

### **Forecast Icons (Smaller):**
- Size: **64x64 pixels**
- Format: **Indexed** (smallest)
- Import in EEZ Studio as `weather_*` (e.g., `weather_sunny`)

### **Download & Convert:**

```bash
# 1. Download weather SVGs (same as before from Pictogrammers MDI)

# 2. Convert to 128x128 for current weather:
cd eez-project/images/weather
for svg in *.svg; do
    ../convert-for-epaper.sh "$svg" 128 128
done

# 3. Convert to 64x64 for forecast:
for svg in *.svg; do
    ../convert-for-epaper.sh "$svg" 64 64
done

# Now you have both sizes for each icon!
```

---

## 💾 Memory Calculation

### **Current Weather:**
- 1 large icon (128x128 indexed): ~2 KB
- Total for 14 weather conditions: ~28 KB

### **Forecast:**
- 1 small icon (64x64 indexed): ~500 bytes
- Total for 14 weather conditions: ~7 KB

### **Grand Total:**
- Icons: ~35 KB
- Variables/Code: ~5 KB
- **Total Added: ~40 KB** (0.7% of 5.75 MB free)

**Result:** Negligible impact! ✅

---

## 🔧 Implementation Phases

### **Phase 1: Home Assistant Setup** (Do First)
1. Add template sensors from `HA_FORECAST_SENSORS.yaml`
2. Restart Home Assistant
3. Verify sensors exist in Developer Tools

### **Phase 2: EEZ Studio Setup**
1. Add 15 forecast variables
2. Create widgets for current weather
3. Create widgets for 5-day forecast
4. Download and import weather icons (both sizes)
5. Build project

### **Phase 3: ESP32 Code** (I'll provide)
1. Add forecast variables to `eez_vars.c`
2. Add fetch logic to `ha_client_task.c`
3. Add icon mapping for forecast icons
4. Build and flash

### **Phase 4: Testing**
1. Flash firmware
2. Verify all sensors are fetched
3. Check icons change dynamically
4. Adjust positions/sizes in EEZ Studio if needed

---

## 📐 Precise Layout Coordinates

Based on 960x640 screen:

```
┌─────────────────────────────────────────────────────────────────┐
│ Date (centered)          WiFi: (860, 10)  HA: (900, 10)         │  ← Y: 0-50
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  [128x128]  Partly cloudy                                       │  ← Y: 60-220
│  @ (50,80)  15.7 °C                                             │
│                                                                  │
├─────────────────────────────────────────────────────────────────┤
│                        5-Day Forecast                            │  ← Y: 240-260
│                                                                  │
│   Sun      Mon      Tue      Wed      Thu                       │  ← Y: 280
│   [64]     [64]     [64]     [64]     [64]                      │  ← Y: 310
│   22°C     24°C     21°C     23°C     25°C                      │  ← Y: 385
│                                                                  │
│   X:100    X:250    X:400    X:550    X:700                     │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

**Horizontal Spacing:** 150px between forecast columns  
**Column Centers:** 100, 250, 400, 550, 700

---

## 🎯 Widget Naming Convention

### **Current Weather:**
- `img_weather_current` - Current weather icon (128x128)
- `ha_lbl_home_condition_today` - Condition text (exists)
- `ha_lbl_home_realfeel_temperature` - Temperature (exists)

### **Forecast Day N (N = 1-5):**
- `lbl_forecast_dayN_name` - Day name ("Sun", "Mon", etc.)
- `img_forecast_dayN` - Weather icon (64x64)
- `lbl_forecast_dayN_temp` - Temperature ("22 °C")

**Total Widgets:** 3 (current) + 15 (forecast) = **18 widgets**

---

## 📝 Next Steps

1. **Add template sensors to Home Assistant** (use `HA_FORECAST_SENSORS.yaml`)
2. **Let me know when sensors are created** - I'll update the ESP32 code
3. **Create the EEZ Studio layout** (variables + widgets)
4. **Download and import weather icons** (128x128 and 64x64)
5. **Build EEZ project** and regenerate code
6. **I'll integrate** the forecast fetching and icon updates

---

## 💡 Tips

- **Start Simple:** Get current weather working with large icon first
- **Then Add Forecast:** Add one day at a time to test
- **Icon Fallback:** If an icon is missing, code will log a warning but won't crash
- **Layout Tweaks:** Easy to adjust positions in EEZ Studio - just rebuild and reflash!

---

**This will be a beautiful, professional weather dashboard!** 🌤️📊
