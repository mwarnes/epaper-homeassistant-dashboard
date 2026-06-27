# Configuration Guide

The E-Paper Home Assistant Dashboard supports configuration through both **build-time** (Kconfig) and **runtime** (NVS) methods.

## Build-Time Configuration (Kconfig)

Configure default values at build time using ESP-IDF's menuconfig system:

```bash
idf.py menuconfig
```

Navigate to: **Component config → Home Assistant Dashboard Configuration**

### Available Settings

#### WiFi Configuration
- **WiFi SSID**: Network name to connect to
- **WiFi Password**: Network password

#### Home Assistant Configuration
- **Home Assistant URL**: Full URL to your HA instance  
  - Example: `http://192.168.1.100:8123`
- **Home Assistant Long-Lived Access Token**: Token from HA profile settings
- **Update interval (seconds)**: How often to fetch data (default: 600 = 10 minutes)
- **Error grace period (seconds)**: Time before showing error screen (default: 1800 = 30 minutes)

#### Time Configuration
- **Timezone**: POSIX timezone string for local time display
  - Examples:
    - UTC: `UTC`
    - CET: `CET-1CEST,M3.5.0,M10.5.0/3`
    - EST: `EST5EDT,M3.2.0/2,M11.1.0`
    - PST: `PST8PDT,M3.2.0,M11.1.0`

#### Power Management
- **Power Mode**: Select operation mode
  - **Always On** (default): Continuous operation
  - **Deep Sleep**: Sleep between updates (not implemented in Phase 1)
  - **Light Sleep**: Idle sleep (not implemented in Phase 1)

### Example: Setting Credentials via Menuconfig

```bash
# Configure build-time defaults
idf.py menuconfig

# Navigate to:
# Component config → Home Assistant Dashboard Configuration → WiFi Configuration
# Set your WiFi SSID and password

# Navigate to:
# Component config → Home Assistant Dashboard Configuration → Home Assistant Configuration
# Set your HA URL and token

# Save and exit, then build
idf.py build
```

## Runtime Configuration (NVS)

Runtime configuration overrides Kconfig defaults and persists across reboots. Use the ESP-IDF NVS tools to set values:

### Using nvs_partition_gen.py

Create a CSV file with your settings:

```csv
key,type,encoding,value
ha_dashboard,namespace,,
wifi_ssid,data,string,YourWiFiSSID
wifi_pass,data,string,YourWiFiPassword
ha_url,data,string,http://192.168.1.100:8123
ha_token,data,string,eyJ0eXAiOiJKV1QiLCJhbGc...
update_interval,data,u32,600000
error_grace_period,data,u32,1800000
power_mode,data,u8,0
timezone,data,string,CET-1CEST,M3.5.0,M10.5.0/3
```

Generate and flash the NVS partition:

```bash
python $IDF_PATH/components/nvs_flash/nvs_partition_generator/nvs_partition_gen.py \
    generate config.csv nvs.bin 0x6000

esptool.py -p /dev/ttyUSB0 write_flash 0x9000 nvs.bin
```

### Using Python nvs_set Tool

```python
from esp_idf_nvs import NVS

nvs = NVS('/dev/ttyUSB0')
nvs.open('ha_dashboard')

# Set WiFi credentials
nvs.set('wifi_ssid', 'YourWiFiSSID', 'string')
nvs.set('wifi_pass', 'YourWiFiPassword', 'string')

# Set Home Assistant credentials
nvs.set('ha_url', 'http://192.168.1.100:8123', 'string')
nvs.set('ha_token', 'eyJ0eXAiOiJKV1QiLCJhbGc...', 'string')

# Set update intervals (in milliseconds)
nvs.set('update_interval', 600000, 'u32')  # 10 minutes
nvs.set('error_grace_period', 1800000, 'u32')  # 30 minutes

# Set power mode (0=always-on, 1=deep-sleep, 2=light-sleep)
nvs.set('power_mode', 0, 'u8')

# Set timezone
nvs.set('timezone', 'CET-1CEST,M3.5.0,M10.5.0/3', 'string')

nvs.commit()
nvs.close()
```

## Configuration Priority

1. **NVS** (highest priority): Runtime configuration persists across reboots
2. **Kconfig** (fallback): Build-time defaults used when NVS is empty

## Viewing Active Configuration

The device logs active configuration on startup:

```
I (1234) config_manager: Configuration loaded:
I (1235) config_manager:   WiFi SSID: MyNetwork
I (1236) config_manager:   HA URL: http://192.168.1.100:8123
I (1237) config_manager:   Update interval: 600000 ms (600 sec)
I (1238) config_manager:   Error grace period: 1800000 ms (1800 sec)
I (1239) config_manager:   Power mode: 0
I (1240) config_manager:   Timezone: CET-1CEST,M3.5.0,M10.5.0/3
```

## Quick Start for Development

For quick testing during development, set credentials in menuconfig:

```bash
# Set your credentials
idf.py menuconfig

# Build and flash
idf.py build flash monitor
```

The device will use these defaults immediately without requiring NVS configuration.

## Security Notes

- **Never commit credentials** to version control
- Kconfig values are compiled into the binary - consider them "development defaults"
- For production, use NVS or secure boot with encrypted NVS
- The `sdkconfig` file contains Kconfig values and should be in `.gitignore`
