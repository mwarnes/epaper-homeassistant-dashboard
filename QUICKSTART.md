# Quick Start Guide

## Current Status: ✅ Ready to Flash

All watchdog timeout issues have been fixed. The firmware is ready for testing.

## Prerequisites

1. **ESP32-S3** with GDEM102F91 e-paper display
2. **USB connection** to your computer
3. **Home Assistant** running on your network
4. **WiFi credentials** and HA URL ready

## Step 1: Configure Your Settings

```bash
idf.py menuconfig
```

Navigate to: **Component config → Home Assistant Dashboard Configuration**

### Required Settings:

#### WiFi Configuration
- **WiFi SSID**: Your network name
- **WiFi Password**: Your network password

#### Home Assistant Configuration  
- **Home Assistant URL**: `http://192.168.1.XXX:8123` ⚠️ **Use IP address, not .local**
- **Home Assistant Token**: Your long-lived access token

#### Optional Settings
- **Update interval**: 600 seconds (10 minutes) - default is good
- **Error grace period**: 1800 seconds (30 minutes) - default is good
- **Timezone**: `SAST-2` for Cape Town (or your timezone)

**Important:** Use your Home Assistant **IP address** instead of `homeassistant.local` to avoid mDNS issues.

### How to Get Your HA Token

1. Open Home Assistant in your browser
2. Click your profile icon (bottom left)
3. Scroll down to "Long-Lived Access Tokens"
4. Click "Create Token"
5. Give it a name (e.g., "ESP32 Dashboard")
6. Copy the token
7. Paste into menuconfig

## Step 2: Build the Firmware

```bash
/tmp/build-with-v6.sh
```

Or if ESP-IDF is already activated:
```bash
idf.py build
```

Expected output:
```
Project build complete. To flash, run:
 idf.py flash
epaper-homeassistant-dashboard.bin binary size 0x137xxx bytes
```

## Step 3: Flash to ESP32

```bash
idf.py -p /dev/ttyUSB0 flash monitor
```

Replace `/dev/ttyUSB0` with your actual serial port:
- **Linux**: `/dev/ttyUSB0`, `/dev/ttyACM0`
- **macOS**: `/dev/cu.usbserial-XXXX`
- **Windows**: `COM3`, `COM4`, etc.

## Step 4: Monitor the Output

Watch for these log messages:

### ✅ Successful Startup

```
I (xxx) wifi_task: Connected to WiFi successfully
I (xxx) wifi_task: Got IP: 192.168.1.XXX
I (xxx) time_sync_task: Time synchronized
I (xxx) lvgl_task: LVGL task starting on core 1
I (xxx) lvgl_task: Initializing EEZ UI...
I (xxx) lvgl_task: EEZ UI initialized
I (xxx) ha_client_task: Starting HA data fetch loop
I (xxx) ha_client_task: Updated: Date=2026-06-25, Time=11:30:00
I (xxx) display_task: State changed, updating display...
```

### ❌ Common Issues

**WiFi Connection Failed:**
```
E (xxx) wifi_task: Failed to connect after 5 attempts
```
→ Check SSID and password in menuconfig

**HA Connection Failed:**
```
E (xxx) esp-tls: couldn't get hostname for :homeassistant.local
E (xxx) ha_client_task: HTTP request failed: ESP_ERR_HTTP_CONNECT
```
→ Use IP address instead of `.local` hostname

**Watchdog Timeout:**
```
E (xxx) task_wdt: Task watchdog got triggered
E (xxx) task_wdt:  - IDLE1 (CPU 1)
```
→ Should be fixed in current version. If still happening, report the backtrace.

## Step 5: Verify Display Updates

The e-paper display should:
1. **Initialize** with a brief flash
2. **Show** "Hello, world!" and spinner
3. **Update** with time from Home Assistant
4. **Display** WiFi and HA status (black text = OK, red = error)

**Note:** E-paper refreshes take ~20 seconds. Be patient!

## Exit Monitor

Press `Ctrl + ]` to exit the serial monitor.

## Troubleshooting

### Build Errors

**Missing LVGL header:**
```
fatal error: class/lv_cache_class.h: No such file or directory
```

**Solution:** Create the missing file:
```bash
cat > managed_components/lvgl__lvgl/src/misc/cache/class/lv_cache_class.h << 'EOF'
#ifndef LV_CACHE_CLASS_H
#define LV_CACHE_CLASS_H
#ifdef __cplusplus
extern "C" {
#endif
#include "lv_cache_lru_rb.h"
#include "lv_cache_lru_ll.h"
#include "lv_cache_sc_da.h"
#ifdef __cplusplus
}
#endif
#endif
EOF
```

Then rebuild.

### Runtime Errors

**Watchdog Timeouts:**
- All animation-related timeouts have been fixed
- Theme initialization disabled
- Screen load animations disabled
- If still happening, check logs for what's blocking

**HA Not Reachable:**
1. Verify HA is running
2. Check HA URL is correct (use IP!)
3. Verify token is valid
4. Check ESP32 can ping HA: `ping YOUR_HA_IP`

**Display Not Updating:**
1. Check BUSY pin connection (GPIO5)
2. Verify SPI connections
3. Look for errors in logs

## What to Expect

### First Boot
- WiFi connects (takes 2-5 seconds)
- Time syncs via NTP (takes 2-10 seconds)
- LVGL initializes (~2 seconds)
- First HA fetch (immediate after time sync)
- Display updates (~20 seconds for e-paper refresh)

### Normal Operation
- **Every 10 minutes**: Fetch new date/time from HA
- **Display updates**: When data changes
- **Status indicators**: Update with each fetch
- **Error screen**: After 30 minutes of HA failures

### Performance
- **CPU usage**: Low (~5-10% on Core 1 for LVGL)
- **Memory**: ~200KB RAM, 8MB PSRAM available
- **Network**: Minimal (two HTTP requests every 10 min)
- **Power**: ~80-100mA @ 3.3V (WiFi on, no sleep yet)

## Next Steps

1. **Monitor for 10+ minutes** to verify updates work
2. **Check HA sensor data** is being displayed correctly
3. **Replace EEZ UI placeholder** with your actual design
4. **Add more sensors** as needed (Phase 2)

## Files Modified vs Original Plan

These files differ from placeholders due to crash fixes:

| File | Change | Reason |
|------|--------|--------|
| `main/ui/ui.c` | Disabled animations | Prevent watchdog timeout |
| `main/ui/screens.c` | Removed theme init | Too slow for e-paper |
| `main/Tasks/lvgl_task.c` | Added mutex protection | Thread safety |
| `main/Tasks/ha_client_task.c` | Fixed time units | Grace period calc |

## Support

If issues persist:
1. Check `docs/CRASH_FIXES.md` for detailed troubleshooting
2. Review logs carefully - errors are usually clear
3. Verify hardware connections (especially BUSY pin)
4. Ensure you're using the latest commit

## Current Version

```bash
git log --oneline -1
# Should show: 34affcd fix: correct logging of update interval
```

Binary size: ~1.27 MB (39% free in 2MB partition)

🎉 **Your dashboard is ready to run!**
