# Power & Solar Monitoring Integration

This document describes the power and solar monitoring features added to the dashboard.

## Overview

The dashboard now displays real-time power usage, solar generation, battery status, and grid connection status with dynamic icons and energy metrics.

## Features

### 1. **House Power** (Static Icon)
- **Icon**: `img_house_power` (always displayed)
- **Current Power**: Shows instantaneous load in W
- **Daily Energy**: Shows total consumption for the day in kWh
- **Entity Source**: 
  - Power: `sensor.deye_sunsynk_sol_ark_x_2_load_power_2`
  - Daily: `sensor.load_energy_daily`

### 2. **Grid Power** (Dynamic Icon)
- **Icons**: 
  - `img_grid_on` - Grid connected
  - `img_grid_off` - Grid disconnected (load shedding/outage)
- **Current Power**: Shows grid import (+) or export (-) in W
- **Daily Energy**: Shows total grid import for the day in kWh
- **Entity Source**:
  - Power: `sensor.deye_sunsynk_sol_ark_x_2_inverter_2_grid_power_2`
  - Daily: `sensor.grid_energy_daily`
  - Status: `input_boolean.grid_outage_active` (inverted logic)
- **Logic**: Icon switches automatically based on grid outage status
  - `input_boolean.grid_outage_active` = `on` → Grid DOWN (show grid_off icon)
  - `input_boolean.grid_outage_active` = `off` → Grid UP (show grid_on icon)

### 3. **Battery Power** (Dynamic Icon - 10 Levels)
- **Icons**: `img_battery_power_10` through `img_battery_power_100`
- **Current Power**: Shows battery charge (-) or discharge (+) in W
- **State of Charge**: Shows current battery charge level as percentage (e.g., "65%")
- **Entity Source**:
  - Power: `sensor.deye_sunsynk_sol_ark_x_2_battery_power_2`
  - SOC: `sensor.deye_sunsynk_sol_ark_x_2_battery_state_of_charge_2`
- **Logic**: Icon selected based on battery state of charge (SOC):
  - 95-100%: `img_battery_power_100`
  - 85-94%: `img_battery_power_90`
  - 75-84%: `img_battery_power_80`
  - 65-74%: `img_battery_power_70`
  - 55-64%: `img_battery_power_60`
  - 45-54%: `img_battery_power_50`
  - 35-44%: `img_battery_power_40`
  - 25-34%: `img_battery_power_30`
  - 15-24%: `img_battery_power_20`
  - 0-14%: `img_battery_power_10`

### 4. **Solar PV Power** (Dynamic Icon)
- **Icons**:
  - `img_solar_power_day` - PV generating power
  - `img_solar_power_night` - No PV generation
- **Current Power**: Shows instantaneous PV generation in W
- **Daily Energy**: Shows total PV generation for the day in kWh
- **Entity Source**:
  - Power: `sensor.deye_sunsynk_sol_ark_x_2_pv_power_2`
  - Daily: `sensor.pv_energy_daily`
- **Logic**: Icon switches based on PV power value:
  - Power > 0W: Day icon (PV is producing)
  - Power = 0W: Night icon (no production)
- **Note**: Uses actual power value, not sun position (handles maintenance scenarios)

## Data Format

### Current Power (W)
All current power values are displayed with "W" unit:
- Format: `"1234 W"` or `"-250 W"` (battery)
- Battery shows sign: `-` = charging, `+` or no sign = discharging
- Updated every 60 seconds

### Daily Energy (kWh)
House, PV, and Grid daily energy displayed with "kWh" unit:
- Format: `"12.5 kWh"`
- Reset at midnight
- Updated every 60 seconds

### Battery State of Charge (%)
Battery "day" position shows current charge level:
- Format: `"65%"`
- Updated every 60 seconds

## Variables

### EEZ Studio Variables
All variables follow the `ha_` prefix naming convention:

**Current Power:**
- `ha_house_power` - House load in W
- `ha_pv_power` - PV generation in W
- `ha_battery_power` - Battery power in W
- `ha_grid_power` - Grid power in W

**Daily Energy:**
- `ha_house_power_day` - House consumption in kWh
- `ha_pv_power_day` - PV generation in kWh
- `ha_battery_power_day` - Battery state of charge in % (e.g., "65%")
- `ha_grid_power_day` - Grid energy in kWh

### Image Objects
Dynamic image objects in EEZ Studio:
- `objects.img_house_power` - Static house icon
- `objects.img_grid_power` - Dynamic grid icon (on/off)
- `objects.img_battery_power` - Dynamic battery icon (10-100%)
- `objects.img_pv_power` - Dynamic PV icon (day/night)

## Functions

### Icon Update Functions (eez_vars.c)

```c
void eez_update_grid_icon(bool grid_connected)
```
Updates grid icon based on connection status.

```c
void eez_update_battery_icon(int soc)
```
Selects appropriate battery icon based on state of charge (0-100%).

```c
void eez_update_pv_icon(float pv_power_w)
```
Switches PV icon between day/night based on power output.

### Data Flow

```
Every 60 seconds:
  ├─ Fetch current power (W)
  │   ├─ House load → ha_house_power
  │   ├─ PV power → ha_pv_power → eez_update_pv_icon()
  │   ├─ Battery power → ha_battery_power
  │   ├─ Grid power → ha_grid_power
  │   ├─ Battery SOC → eez_update_battery_icon()
  │   └─ Grid status → eez_update_grid_icon()
  │
  ├─ Fetch daily energy (kWh) and battery SOC (%)
  │   ├─ House consumption → ha_house_power_day
  │   ├─ PV generation → ha_pv_power_day
  │   ├─ Battery SOC → ha_battery_power_day ("65%")
  │   └─ Grid import → ha_grid_power_day
  │
  └─ ui_tick() updates all labels automatically
```

## Home Assistant Entities Used

### Deye/Sunsynk Inverter Sensors
- `sensor.deye_sunsynk_sol_ark_x_2_pv_power_2` - PV power
- `sensor.deye_sunsynk_sol_ark_x_2_load_power_2` - House load
- `sensor.deye_sunsynk_sol_ark_x_2_battery_power_2` - Battery power
- `sensor.deye_sunsynk_sol_ark_x_2_battery_state_of_charge_2` - Battery SOC
- `sensor.deye_sunsynk_sol_ark_x_2_inverter_2_grid_power_2` - Grid power

### Daily Energy Sensors (Template)
- `sensor.pv_energy_daily` - Daily PV generation
- `sensor.load_energy_daily` - Daily house consumption
- `sensor.grid_energy_daily` - Daily grid import

### Status Sensors
- `input_boolean.grid_outage_active` - Grid outage status (inverted: on = outage, off = connected)

## Implementation Notes

1. **Power values are signed**: Positive = consumption/import, Negative = generation/export
2. **Icons update instantly**: Icon changes happen immediately when conditions change
3. **No sun entity dependency**: PV icon uses actual power measurement, not time of day
4. **Battery icon smooth transitions**: 10% increments prevent excessive icon switching
5. **Energy resets daily**: Daily energy sensors reset at midnight (HA side)

## Binary Size

After adding power monitoring:
- **Binary**: 2.0 MB
- **Free**: 5.0 MB (73%)
- **Status**: ✅ Well within limits

## Testing

Flash and monitor to see power data:

```bash
idf.py flash monitor
```

**Expected logs:**
```
I (xxx) ha_client_task: House power: 1234 W
I (xxx) ha_client_task: PV power: 3500 W
I (xxx) ha_client_task: PV icon: DAY (3500.0W)
I (xxx) ha_client_task: Battery power: -250 W (charging)
I (xxx) ha_client_task: Battery SOC: 85%
I (xxx) eez_vars: Battery icon updated: SOC=85%
I (xxx) eez_vars: HA variable updated: battery_power_day = '85%'
I (xxx) ha_client_task: Grid power: -1500 W
I (xxx) ha_client_task: Grid status: CONNECTED (outage_active=false)
I (xxx) eez_vars: Grid icon: ON (connected)
I (xxx) ha_client_task: House energy today: 12.5 kWh
I (xxx) ha_client_task: PV energy today: 28.3 kWh
I (xxx) ha_client_task: Grid energy today: 2.1 kWh
I (xxx) ha_client_task: Power & energy data updated
```

## Future Enhancements

Potential additions:
- Export energy display (negative grid values)
- Battery charge/discharge direction indicator
- Self-sufficiency percentage
- Real-time power flow animation
- Historical energy graphs
