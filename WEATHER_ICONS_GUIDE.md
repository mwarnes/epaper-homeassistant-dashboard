# Weather Icons Setup Guide

This guide shows you how to download, convert, and import weather icons for your e-paper dashboard.

## 📋 Required Icons

Based on Home Assistant weather states, you need these MDI icons:

| Weather State | MDI Icon Name | Download From |
|--------------|---------------|---------------|
| Sunny | `mdi:weather-sunny` | https://pictogrammers.com/library/mdi/icon/weather-sunny/ |
| Clear night | `mdi:weather-night` | https://pictogrammers.com/library/mdi/icon/weather-night/ |
| Partly cloudy | `mdi:weather-partly-cloudy` | ✅ Already imported! |
| Cloudy | `mdi:weather-cloudy` | https://pictogrammers.com/library/mdi/icon/weather-cloudy/ |
| Foggy | `mdi:weather-fog` | https://pictogrammers.com/library/mdi/icon/weather-fog/ |
| Rainy | `mdi:weather-rainy` | https://pictogrammers.com/library/mdi/icon/weather-rainy/ |
| Pouring | `mdi:weather-pouring` | https://pictogrammers.com/library/mdi/icon/weather-pouring/ |
| Snowy | `mdi:weather-snowy` | https://pictogrammers.com/library/mdi/icon/weather-snowy/ |
| Snow & Rain | `mdi:weather-snowy-rainy` | https://pictogrammers.com/library/mdi/icon/weather-snowy-rainy/ |
| Hail | `mdi:weather-hail` | https://pictogrammers.com/library/mdi/icon/weather-hail/ |
| Lightning | `mdi:weather-lightning` | https://pictogrammers.com/library/mdi/icon/weather-lightning/ |
| Lightning & Rain | `mdi:weather-lightning-rainy` | https://pictogrammers.com/library/mdi/icon/weather-lightning-rainy/ |
| Windy | `mdi:weather-windy` | https://pictogrammers.com/library/mdi/icon/weather-windy/ |
| Exceptional | `mdi:alert-circle` | https://pictogrammers.com/library/mdi/icon/alert-circle/ |

## 🔧 Step-by-Step Process

### Step 1: Download SVG Icons

For each icon:

1. Visit the Pictogrammers MDI library link above
2. Click **Download SVG** button
3. Save to `eez-project/images/weather/` directory (create if needed)
4. Rename file to match pattern: `weather-sunny.svg`, `weather-cloudy.svg`, etc.

**Quick tip:** Create the directory first:
```bash
mkdir -p eez-project/images/weather
```

### Step 2: Convert SVGs to PNG

Use your existing conversion script to convert all icons at once:

```bash
cd eez-project/images/weather

# Convert all SVG files to PNG (64x64 recommended for e-paper)
for svg in *.svg; do
    ../convert-for-epaper.sh "$svg" 64 64
done
```

This will:
- Convert SVG → PNG at 64x64 pixels
- Set white background (transparent → white)
- Save as `${filename}-64x64.png`

**Recommended sizes:**
- **64x64**: Good balance for e-paper
- **48x48**: Smaller, saves memory
- **96x96**: Larger, more detail (uses more memory)

### Step 3: Import to EEZ Studio

1. **Open** your EEZ Studio project
2. **Navigate to:** Images section
3. **For each PNG file:**
   - Click **"Add Image"**
   - Select the converted PNG file (e.g., `weather-sunny-64x64.png`)
   - **Name it:** Use pattern `weather_sunny`, `weather_cloudy`, etc.
   - **Format:** Choose **Indexed** (smallest) or **RGB565** (better quality)
   - Click **OK**

4. **Build** the EEZ project to generate C code

### Step 4: Update CMakeLists.txt

Add the new image files to your build:

```cmake
# In main/CMakeLists.txt, find the SRCS section and add:
"ui/ui_image_weather_sunny.c"
"ui/ui_image_weather_clear_night.c"
"ui/ui_image_weather_cloudy.c"
"ui/ui_image_weather_fog.c"
"ui/ui_image_weather_rainy.c"
"ui/ui_image_weather_pouring.c"
"ui/ui_image_weather_snowy.c"
"ui/ui_image_weather_snowy_rainy.c"
"ui/ui_image_weather_hail.c"
"ui/ui_image_weather_lightning.c"
"ui/ui_image_weather_lightning_rainy.c"
"ui/ui_image_weather_windy.c"
"ui/ui_image_weather_exceptional.c"
```

### Step 5: Add Image Widget in EEZ Studio

1. **Open** your main screen in EEZ Studio
2. **Add** an **Image** widget
3. **Name it:** `img_weather`
4. **Position:** Where you want the weather icon (e.g., next to condition label)
5. **Size:** 64x64 (or whatever size you chose)
6. **Default Image:** `weather_sunny` (or leave blank)
7. **Build** the project again

### Step 6: Enable Icon Updates in Code

In `main/eez_vars.c`, uncomment the icon mapping:

Find each commented line like:
```c
// icon = &img_weather_sunny;
```

And uncomment/update it:
```c
icon = &img_weather_sunny;
```

Also, at the bottom of `eez_update_weather_icon()`, change:
```c
#if 0  // Enable when img_weather widget exists
```

To:
```c
#if 1  // Enable when img_weather widget exists
```

### Step 7: Rebuild and Flash

```bash
idf.py build flash monitor
```

## 📊 Expected Result

After setup, your dashboard will show:

```
┌────────────────────────────────────────┐
│   Saturday Jun 27, 2026                │
│                                        │
│   Partly cloudy   [☁️]  ← Icon here!  │
│                                        │
│        15.7 °C                         │
│                                        │
│   [WiFi]  [HA]  Last updated: 16:45   │
└────────────────────────────────────────┘
```

The icon will automatically change based on the weather condition from Home Assistant!

## 🎨 Icon Naming Convention

| EEZ Studio Name | Generated Code | Maps to Condition |
|-----------------|----------------|-------------------|
| `weather_sunny` | `img_weather_sunny` | "Sunny" |
| `weather_clear_night` | `img_weather_clear_night` | "Clear night" |
| `weather_partly_cloudy` | `img_partly_cloudy` | "Partly cloudy" (already exists!) |
| `weather_cloudy` | `img_weather_cloudy` | "Cloudy" |
| `weather_fog` | `img_weather_fog` | "Foggy" |
| `weather_rainy` | `img_weather_rainy` | "Rainy" |
| `weather_pouring` | `img_weather_pouring` | "Pouring" |
| `weather_snowy` | `img_weather_snowy` | "Snowy" |
| `weather_snowy_rainy` | `img_weather_snowy_rainy` | "Snow & Rain" |
| `weather_hail` | `img_weather_hail` | "Hail" |
| `weather_lightning` | `img_weather_lightning` | "Lightning" |
| `weather_lightning_rainy` | `img_weather_lightning_rainy` | "Lightning & Rain" |
| `weather_windy` | `img_weather_windy` | "Windy" |
| `weather_exceptional` | `img_weather_exceptional` | "Exceptional" |

## 💾 Memory Considerations

**Icon sizes and memory usage (approximate):**

| Size | Indexed (2 colors) | RGB565 | ARGB8888 |
|------|-------------------|--------|----------|
| 48x48 | ~300 bytes | ~4.5 KB | ~9 KB |
| 64x64 | ~500 bytes | ~8 KB | ~16 KB |
| 96x96 | ~1.1 KB | ~18 KB | ~36 KB |

**For e-paper (black/white):** Use **Indexed** format - it's the smallest and looks great!

**Recommendation:** Start with 64x64 Indexed format. You have 81% free space (5.75 MB), so 14 icons × 500 bytes = ~7 KB total (negligible).

## 🔍 Troubleshooting

### Icons don't appear:
- Check `img_weather` widget exists in EEZ Studio
- Verify `#if 1` is enabled in `eez_vars.c`
- Check logs for "Weather icon: ..." messages

### Build errors:
- Verify all `ui_image_weather_*.c` files are in `CMakeLists.txt`
- Check image names match exactly (underscores, not hyphens)

### Icons look wrong:
- Make sure you ran `convert-for-epaper.sh` (white background)
- Try Indexed format instead of RGB565
- Check icon size matches widget size in EEZ Studio

## 📝 Next Steps

Once icons are working:
1. Adjust icon size/position in EEZ Studio for best layout
2. Consider adding colored icons (yellow sun, blue rain) for GDEM102F91's 4-color support
3. Add icon to error states (e.g., question mark for unknown conditions)

---

**Note:** You only need to do this setup once. After that, icons will automatically update based on weather conditions from Home Assistant!
