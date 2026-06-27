# Quick Start: Using menuconfig

## Set Your Credentials

```bash
idf.py menuconfig
```

Navigate using arrow keys and Enter:

```
Component config →
  Home Assistant Dashboard Configuration →
    WiFi Configuration →
      WiFi SSID: [Enter your WiFi name]
      WiFi Password: [Enter your WiFi password]
    
    Home Assistant Configuration →
      Home Assistant URL: http://[your-ha-ip]:8123
      Home Assistant Long-Lived Access Token: [paste your token]
      Update interval: 600  (10 minutes, in seconds)
      Error grace period: 1800  (30 minutes, in seconds)
    
    Time Configuration →
      Timezone: [Your POSIX timezone string]
        Examples:
          - UTC (default)
          - CET-1CEST,M3.5.0,M10.5.0/3  (Central European Time)
          - EST5EDT,M3.2.0/2,M11.1.0     (Eastern Time)
          - PST8PDT,M3.2.0,M11.1.0       (Pacific Time)
```

Press `S` to save, then `Q` to quit.

## Generate Home Assistant Token

1. Log into Home Assistant
2. Click your profile (bottom left)
3. Scroll down to "Long-Lived Access Tokens"
4. Click "Create Token"
5. Give it a name (e.g., "ESP32 Dashboard")
6. Copy the token and paste into menuconfig

## Build and Flash

```bash
/tmp/build-with-v6.sh  # Uses ESP-IDF v6.0
idf.py -p /dev/ttyUSB0 flash monitor
```

Or with ESP-IDF activated:

```bash
idf.py build flash monitor
```

## Verify Configuration

Watch the logs for:

```
I (1234) config_manager: Configuration loaded:
I (1235) config_manager:   WiFi SSID: YourNetwork
I (1236) config_manager:   HA URL: http://192.168.1.100:8123
I (1237) config_manager:   Update interval: 600000 ms (600 sec)
I (1238) config_manager:   Error grace period: 1800000 ms (1800 sec)
I (1239) config_manager:   Power mode: 0
I (1240) config_manager:   Timezone: CET-1CEST,M3.5.0,M10.5.0/3

I (2000) wifi_task: Connected to WiFi
I (3000) time_sync_task: Time synchronized
I (5000) ha_client_task: HA data fetched successfully
```

## Quick Tips

- **Default values**: Already set for quick testing (see `main/Kconfig.projbuild`)
- **Override at runtime**: Use NVS tools (see docs/CONFIGURATION.md)
- **Security**: Don't commit `sdkconfig` with real credentials
- **Testing**: Values compile into binary - great for development!

## Common Timezone Strings

| Location | Timezone String |
|----------|----------------|
| UTC | `UTC` |
| London | `GMT0BST,M3.5.0/1,M10.5.0` |
| Paris/Berlin | `CET-1CEST,M3.5.0,M10.5.0/3` |
| New York | `EST5EDT,M3.2.0/2,M11.1.0` |
| Los Angeles | `PST8PDT,M3.2.0,M11.1.0` |
| Tokyo | `JST-9` |
| Sydney | `AEST-10AEDT,M10.1.0,M4.1.0/3` |

## Home Assistant Sensors Required

The dashboard fetches these sensors by default:
- `sensor.date` - Date entity
- `sensor.time` - Time entity

These are usually available by default in Home Assistant. Verify in:
Developer Tools → States → Search for "sensor.date" and "sensor.time"
