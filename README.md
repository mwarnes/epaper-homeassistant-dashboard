# E-Paper Home Assistant Dashboard

ESP32-S3 dashboard displaying Home Assistant data on a 4-color e-paper display (GDEM102F91, 960×640).

## Quick Start

### 1. Configure Credentials

```bash
idf.py menuconfig
```

Navigate to: **Component config → Home Assistant Dashboard Configuration**

Set your:
- WiFi SSID and password
- Home Assistant URL (e.g., `http://192.168.1.100:8123`)
- Home Assistant long-lived access token

### 2. Build and Flash

```bash
/tmp/build-with-v6.sh  # Or: idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

### 3. Monitor Logs

Watch the serial output to verify:
- WiFi connection
- Time synchronization
- Home Assistant data fetch
- Display updates

## Features

### Phase 1 (Current)
- ✅ WiFi connectivity with auto-reconnect
- ✅ SNTP time synchronization
- ✅ Home Assistant REST API client
- ✅ Date/time display from HA sensors (`sensor.date`, `sensor.time`)
- ✅ 10-minute update cycle (configurable)
- ✅ Error screen after 30 minutes of HA failures (configurable)
- ✅ 4-color e-paper display support
- ✅ LVGL v9.2 UI framework
- ✅ FreeRTOS dual-core architecture (Core 0: network, Core 1: UI)
- ✅ NVS configuration storage
- ✅ Kconfig build-time configuration
- ✅ Timezone support

### Planned Phase 2+
- [ ] Additional HA sensors (temperature, weather, etc.)
- [ ] Battery power support with deep sleep
- [ ] Touch interface for manual refresh
- [ ] QR code display for quick access
- [ ] Over-the-air (OTA) updates

## Architecture

### Task Structure

**Core 0 (Network Layer)**:
- `wifi_task` - WiFi connection management
- `time_sync_task` - SNTP time synchronization  
- `ha_client_task` - Fetch data from Home Assistant
- `power_mgmt_task` - Power management (placeholder)

**Core 1 (UI Layer)**:
- `lvgl_task` - LVGL timer handler (5ms tick)
- `display_task` - E-paper update coordination

### Shared Resources
- Event group: `WIFI_CONNECTED_BIT`, `TIME_SYNCED_BIT`, `HA_ERROR_BIT`
- Dashboard state: date, time, status flags, timestamps
- Mutexes: `dashboard_state_mutex`, `lvgl_mutex`

## Configuration

See [docs/CONFIGURATION.md](docs/CONFIGURATION.md) for detailed configuration options.

### Build-Time (Kconfig)

All settings available via `idf.py menuconfig`:
- WiFi credentials
- Home Assistant URL and token
- Update interval and error grace period
- Timezone
- Power mode

### Runtime (NVS)

Runtime configuration overrides Kconfig defaults. See CONFIGURATION.md for NVS tools.

## Hardware

- **MCU**: ESP32-S3 (dual-core, 240 MHz)
- **Display**: GDEM102F91 (Good Display)
  - Resolution: 960×640 pixels
  - Colors: 4 (black, white, red, yellow)
  - Interface: SPI
  - Refresh time: ~20 seconds
- **Power**: USB (always-on mode in Phase 1)

## Dependencies

- ESP-IDF v6.0+
- LVGL v9.2 (managed component)
- `esp-lvgl-port` (managed component)
- `gdem102f91-epaper-driver` (local component)
- `esp-lvgl-epaper-port` (local component)

## Project Structure

```
epaper-homeassistant-dashboard/
├── main/
│   ├── main.c                    # Application entry point
│   ├── Shared/                   # Shared resources
│   │   ├── shared_resources.c/h  # Event groups, mutexes
│   │   └── config_manager.c/h    # NVS configuration
│   ├── Tasks/                    # FreeRTOS tasks
│   │   ├── wifi_task.c/h
│   │   ├── time_sync_task.c/h
│   │   ├── ha_client_task.c/h
│   │   ├── power_mgmt_task.c/h
│   │   ├── lvgl_task.c/h
│   │   └── display_task.c/h
│   ├── ui/                       # EEZ Studio UI files
│   │   ├── screens.c/h           # Screen definitions
│   │   ├── vars.h                # UI variables
│   │   └── ui.c/h                # UI initialization
│   ├── eez_vars.c/h              # EEZ variable implementation
│   └── Kconfig.projbuild         # Configuration menu
├── docs/                         # Documentation
│   ├── CONFIGURATION.md
│   └── superpowers/              # Design specs & plans
├── partitions.csv                # Partition table (2MB app)
├── sdkconfig                     # ESP-IDF configuration
└── CMakeLists.txt

Local components (../):
├── gdem102f91-epaper-driver/     # E-paper low-level driver
└── esp-lvgl-epaper-port/         # LVGL integration for e-paper
```

## Binary Size

- **Current**: 1.27 MB (39% free in 2MB partition)
- **Bootloader**: 21 KB
- **Partition table**: Default

## Development

### Build Script

Uses ESP-IDF v6.0 environment:

```bash
/tmp/build-with-v6.sh
```

Contents:
```bash
#!/bin/bash
source ~/.espressif/v6.0/esp-idf/export.sh
idf.py build
```

### Monitor

```bash
idf.py -p /dev/ttyUSB0 monitor
```

Exit monitor: `Ctrl+]`

### Clean Build

```bash
idf.py fullclean
idf.py build
```

## Troubleshooting

### WiFi Not Connecting
- Check SSID and password in menuconfig
- Verify 2.4GHz network (ESP32 doesn't support 5GHz)
- Check router logs

### Home Assistant Connection Failed
- Verify HA URL is reachable from ESP32
- Check long-lived token is valid
- Ensure HA is running
- Check firewall rules

### Time Not Syncing
- Verify WiFi is connected
- Check NTP servers are reachable
- Review timezone configuration

### Display Not Updating
- Check logs for LVGL errors
- Verify e-paper SPI connections
- Check display task status in logs

## License

[Add your license here]

## Credits

- ESP-IDF by Espressif Systems
- LVGL by LVGL LLC
- E-paper driver for GDEM102F91
