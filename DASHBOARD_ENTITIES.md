# Dashboard Entity Mapping - E-Paper Home Assistant

## Current Implementation (needs update)

### Date & Time - CHANGE REQUIRED ✅

**Current (broken):**
- `sensor.date` → 404 Not Found
- `sensor.time` → 404 Not Found

**Use instead:**
- `sensor.date_time_iso` → Current value: `2026-06-25T13:01:00`
  - Format: ISO 8601 (can be parsed to extract date and time separately)

## Recommended Dashboard Entities

### 1. **Essential Status** (Currently Implemented)
- ✅ WiFi Status (internal)
- ✅ HA Connection Status (internal)
- ✅ Date & Time (needs entity fix)

### 2. **Weather & Environment** ⭐
```
weather.home                           → "sunny"
sensor.home_apparent_temperature       → 21.1 (with unit)
sensor.home_realfeel_temperature       → 22.6 (RealFeel)
sensor.home_humidity                   → 56%
```

### 3. **Solar Power System** ⚡ (You have a dual inverter setup!)
```
# Battery
sensor.battery_soc                                              → 68%
sensor.deye_sunsynk_sol_ark_x_2_battery_power_2                → -369W (neg=charging)
sensor.deye_sunsynk_sol_ark_x_2_battery_temperature_2         → 20.0°C

# Grid
sensor.deye_sunsynk_sol_ark_x_2_grid_power_2                   → 50W
sensor.deye_sunsynk_sol_ark_x_2_grid_voltage_2                 → 238.3V

# Solar Production ⭐ FOUND!
sensor.deye_sunsynk_sol_ark_x_2_pv_power_2                     → 3505W (TOTAL CURRENT)
sensor.deye_sunsynk_sol_ark_x_2_inverter_1_pv_power_3          → 1255W (Inverter 1)
sensor.deye_sunsynk_sol_ark_x_2_inverter_2_pv_power_3          → 2250W (Inverter 2)
sensor.deye_sunsynk_sol_ark_x_2_pv_energy_2                    → 12.75 kWh (today)
sensor.deye_sunsynk_sol_ark_x_2_inverter_1_device_mode_2       → "Charge below 27%"

# Energy Totals
sensor.deye_sunsynk_sol_ark_x_2_grid_energy_in_2               → 6.85 kWh (today)
sensor.deye_sunsynk_sol_ark_x_2_grid_energy_out_2              → 0.01 kWh (today)
sensor.deye_sunsynk_sol_ark_x_2_battery_energy_in_2            → 6.88 kWh
sensor.deye_sunsynk_sol_ark_x_2_battery_energy_out_2           → 0.11 kWh
```

### 4. **Security & Doors** 🚪
```
binary_sensor.driveway_gate_homekit                            → off (closed)
binary_sensor.main_gate_shelly_driveway_gate                   → on (open)
# Note: Check for garage door sensors (automations suggest you have one)
```

### 5. **Temperature Sensors** 🌡️
```
sensor.66sleigh_temperature                                    → 15.6°C (outdoor?)
sensor.geyserwala_water_temperature                            → 31°C (geyser/hot water)
sensor.deye_sunsynk_sol_ark_x_2_inverter_1_temperature_2      → 47.6°C (inverter health)
sensor.deye_sunsynk_sol_ark_x_2_inverter_2_temperature_2      → 51.6°C (inverter health)
```

### 6. **Batteries** 🔋
```
sensor.iphone_battery_level                                    → 65%
sensor.66sleigh_battery                                        → 88%
sensor.percy_battery                                           → 100% (pool robot?)
```

## Recommended Dashboard Layout for E-Paper

### Layout 1: Solar Energy Dashboard ⚡
```
┌────────────────────────────────────────────────┐
│ Date: 2026-06-25        Time: 13:01           │
│ Weather: Sunny ☀️        Temp: 21.1°C         │
├────────────────────────────────────────────────┤
│                                                │
│  BATTERY SOC: 68% ████████████░░░░░░          │
│  Battery Power: -369W (Charging)              │
│                                                │
│  GRID: 50W ←                                  │
│  Solar Production: 3505W ⚡                    │
│                                                │
│  Energy Today:                                │
│    From Grid: 6.85 kWh                        │
│    To Grid:   0.01 kWh                        │
│    Battery In: 6.88 kWh                       │
│                                                │
├────────────────────────────────────────────────┤
│ WiFi: ✓  HA: ✓  Last Update: 2 min ago       │
└────────────────────────────────────────────────┘
```

### Layout 2: Home Status Dashboard 🏠
```
┌────────────────────────────────────────────────┐
│ 2026-06-25 13:01          Sunny ☀️ 21°C       │
├────────────────────────────────────────────────┤
│ GATES                    │ TEMPERATURES        │
│ • Driveway: Closed       │ • Outside: 15.6°C  │
│ • Main: Open ⚠️          │ • Water: 31°C      │
│                          │ • Home: 21.1°C     │
├──────────────────────────┼─────────────────────┤
│ POWER                    │ BATTERIES           │
│ • Grid: 50W              │ • House: 68%       │
│ • Battery: Charging      │ • iPhone: 65%      │
│ • Solar: 3505W           │ • 66Sleigh: 88%    │
├────────────────────────────────────────────────┤
│ WiFi: ✓  HA: ✓  Updated: 13:01               │
└────────────────────────────────────────────────┘
```

## Next Steps

1. **Update Code** - Change entity IDs:
   - Replace `sensor.date` → `sensor.date_time_iso` (parse for date)
   - Replace `sensor.time` → `sensor.date_time_iso` (parse for time)

2. ✅ **Solar PV entities found:**
   - `sensor.deye_sunsynk_sol_ark_x_2_pv_power_2` = 3505W (use this for dashboard)
   - `sensor.deye_sunsynk_sol_ark_x_2_pv_energy_2` = 12.75 kWh (daily production)

3. **Expand Dashboard** - Choose entities based on preferred layout above

## Entity Groups for Easy Testing

### Minimal (Current):
- `sensor.date_time_iso`
- WiFi status (internal)
- HA status (internal)

### Solar Focus:
- `sensor.date_time_iso`
- `sensor.battery_soc`
- `sensor.deye_sunsynk_sol_ark_x_2_battery_power_2`
- `sensor.deye_sunsynk_sol_ark_x_2_grid_power_2`
- `weather.home`
- `sensor.home_apparent_temperature`

### Home Status:
- `sensor.date_time_iso`
- `weather.home`
- `sensor.home_apparent_temperature`
- `sensor.home_humidity`
- `binary_sensor.driveway_gate_homekit`
- `binary_sensor.main_gate_shelly_driveway_gate`
- `sensor.battery_soc`
- `sensor.iphone_battery_level`
