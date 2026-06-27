# How to Add More Entities to the Dashboard

Quick guide for expanding your e-paper dashboard with solar, weather, and other entities.

## Step 1: Add to dashboard_state (Shared/shared_resources.h)

**Current state:**
```c
typedef struct {
    char date_str[32];
    char time_str[32];
    bool wifi_connected;
    bool ha_connected;
    // ... etc
} dashboard_state_t;
```

**Add new fields:**
```c
typedef struct {
    char date_str[32];
    char time_str[32];
    bool wifi_connected;
    bool ha_connected;
    
    // Solar/Energy
    float solar_power_w;           // sensor.deye_sunsynk_sol_ark_x_2_pv_power_2
    float battery_soc;              // sensor.battery_soc
    float battery_power_w;          // sensor.deye_sunsynk_sol_ark_x_2_battery_power_2
    float grid_power_w;             // sensor.deye_sunsynk_sol_ark_x_2_grid_power_2
    
    // Weather
    char weather_state[32];         // weather.home (sunny/cloudy/etc)
    float temperature_c;            // sensor.home_apparent_temperature
    float humidity_pct;             // sensor.home_humidity
    
    // ... existing fields
} dashboard_state_t;
```

## Step 2: Fetch Entities (main/Tasks/ha_client_task.c)

**Add to fetch loop:**
```c
while (1) {
    // Existing: Fetch datetime
    char datetime_iso[32] = {0};
    // ... existing code ...
    
    // NEW: Fetch solar/energy data
    char solar_power_str[32] = {0};
    char battery_soc_str[32] = {0};
    char battery_power_str[32] = {0};
    char grid_power_str[32] = {0};
    
    esp_err_t solar_err = ha_fetch_entity(ha_url, ha_token, 
                                           "sensor.deye_sunsynk_sol_ark_x_2_pv_power_2", 
                                           solar_power_str, sizeof(solar_power_str));
    
    esp_err_t battery_soc_err = ha_fetch_entity(ha_url, ha_token,
                                                 "sensor.battery_soc",
                                                 battery_soc_str, sizeof(battery_soc_str));
    
    esp_err_t battery_power_err = ha_fetch_entity(ha_url, ha_token,
                                                   "sensor.deye_sunsynk_sol_ark_x_2_battery_power_2",
                                                   battery_power_str, sizeof(battery_power_str));
    
    esp_err_t grid_power_err = ha_fetch_entity(ha_url, ha_token,
                                                "sensor.deye_sunsynk_sol_ark_x_2_grid_power_2",
                                                grid_power_str, sizeof(grid_power_str));
    
    // NEW: Fetch weather data
    char weather_state[32] = {0};
    char temperature_str[32] = {0};
    char humidity_str[32] = {0};
    
    esp_err_t weather_err = ha_fetch_entity(ha_url, ha_token,
                                            "weather.home",
                                            weather_state, sizeof(weather_state));
    
    esp_err_t temp_err = ha_fetch_entity(ha_url, ha_token,
                                         "sensor.home_apparent_temperature",
                                         temperature_str, sizeof(temperature_str));
    
    esp_err_t humidity_err = ha_fetch_entity(ha_url, ha_token,
                                             "sensor.home_humidity",
                                             humidity_str, sizeof(humidity_str));
    
    // Update shared state
    if (xSemaphoreTake(dashboard_state_mutex, portMAX_DELAY) == pdTRUE) {
        // Existing datetime update
        if (datetime_err == ESP_OK && strlen(date_str) > 0 && strlen(time_str) > 0) {
            strncpy(dashboard_state.date_str, date_str, sizeof(dashboard_state.date_str) - 1);
            strncpy(dashboard_state.time_str, time_str, sizeof(dashboard_state.time_str) - 1);
        }
        
        // NEW: Update solar/energy
        if (solar_err == ESP_OK) {
            dashboard_state.solar_power_w = atof(solar_power_str);
        }
        if (battery_soc_err == ESP_OK) {
            dashboard_state.battery_soc = atof(battery_soc_str);
        }
        if (battery_power_err == ESP_OK) {
            dashboard_state.battery_power_w = atof(battery_power_str);
        }
        if (grid_power_err == ESP_OK) {
            dashboard_state.grid_power_w = atof(grid_power_str);
        }
        
        // NEW: Update weather
        if (weather_err == ESP_OK) {
            strncpy(dashboard_state.weather_state, weather_state, sizeof(dashboard_state.weather_state) - 1);
        }
        if (temp_err == ESP_OK) {
            dashboard_state.temperature_c = atof(temperature_str);
        }
        if (humidity_err == ESP_OK) {
            dashboard_state.humidity_pct = atof(humidity_str);
        }
        
        time(&dashboard_state.last_successful_update);
        dashboard_state.ha_connected = true;
        
        xSemaphoreGive(dashboard_state_mutex);
    }
    
    // Sleep until next update
    vTaskDelay(pdMS_TO_TICKS(update_interval));
}
```

## Step 3: Update EEZ Variables (main/eez_vars.c/h)

**Add new setter functions:**
```c
// In eez_vars.h
void eez_set_solar_power(float watts);
void eez_set_battery_soc(float percent);
void eez_set_battery_power(float watts);
void eez_set_grid_power(float watts);
void eez_set_weather(const char* state);
void eez_set_temperature(float celsius);
void eez_set_humidity(float percent);

// In eez_vars.c
void eez_set_solar_power(float watts) {
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%.0f W", watts);
    lv_label_set_text(objects.solar_power_label, buffer);
}

void eez_set_battery_soc(float percent) {
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%.0f%%", percent);
    lv_label_set_text(objects.battery_soc_label, buffer);
    
    // Also update progress bar if you have one
    lv_bar_set_value(objects.battery_bar, (int)percent, LV_ANIM_OFF);
}

void eez_set_battery_power(float watts) {
    char buffer[32];
    if (watts < 0) {
        snprintf(buffer, sizeof(buffer), "Charging: %.0f W", -watts);
    } else {
        snprintf(buffer, sizeof(buffer), "Discharging: %.0f W", watts);
    }
    lv_label_set_text(objects.battery_power_label, buffer);
}

void eez_set_grid_power(float watts) {
    char buffer[32];
    if (watts > 0) {
        snprintf(buffer, sizeof(buffer), "Importing: %.0f W", watts);
    } else {
        snprintf(buffer, sizeof(buffer), "Exporting: %.0f W", -watts);
    }
    lv_label_set_text(objects.grid_power_label, buffer);
}

void eez_set_weather(const char* state) {
    // Map weather state to icon/text
    if (strcmp(state, "sunny") == 0) {
        lv_label_set_text(objects.weather_label, "☀️ Sunny");
    } else if (strcmp(state, "cloudy") == 0) {
        lv_label_set_text(objects.weather_label, "☁️ Cloudy");
    } else if (strcmp(state, "rainy") == 0) {
        lv_label_set_text(objects.weather_label, "🌧️ Rainy");
    } else {
        lv_label_set_text(objects.weather_label, state);
    }
}

void eez_set_temperature(float celsius) {
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%.1f°C", celsius);
    lv_label_set_text(objects.temperature_label, buffer);
}

void eez_set_humidity(float percent) {
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%.0f%%", percent);
    lv_label_set_text(objects.humidity_label, buffer);
}
```

## Step 4: Update Display Task (main/Tasks/display_task.c)

**Add to state change detection:**
```c
// Read current state
char current_date[32] = {0};
char current_time[32] = {0};
bool wifi_connected = false;
bool ha_connected = false;

// NEW: Solar/Energy state
float solar_power_w = 0;
float battery_soc = 0;
float battery_power_w = 0;
float grid_power_w = 0;

// NEW: Weather state
char weather_state[32] = {0};
float temperature_c = 0;
float humidity_pct = 0;

if (xSemaphoreTake(dashboard_state_mutex, portMAX_DELAY) == pdTRUE) {
    strncpy(current_date, dashboard_state.date_str, sizeof(current_date) - 1);
    strncpy(current_time, dashboard_state.time_str, sizeof(current_time) - 1);
    wifi_connected = dashboard_state.wifi_connected;
    ha_connected = dashboard_state.ha_connected;
    
    // NEW: Copy solar/energy
    solar_power_w = dashboard_state.solar_power_w;
    battery_soc = dashboard_state.battery_soc;
    battery_power_w = dashboard_state.battery_power_w;
    grid_power_w = dashboard_state.grid_power_w;
    
    // NEW: Copy weather
    strncpy(weather_state, dashboard_state.weather_state, sizeof(weather_state) - 1);
    temperature_c = dashboard_state.temperature_c;
    humidity_pct = dashboard_state.humidity_pct;
    
    xSemaphoreGive(dashboard_state_mutex);
}

// Update display
if (xSemaphoreTake(lvgl_mutex, portMAX_DELAY) == pdTRUE) {
    // Existing updates
    if (strlen(current_date) > 0) {
        eez_set_date(current_date);
    }
    if (strlen(current_time) > 0) {
        eez_set_time(current_time);
    }
    
    // NEW: Update solar/energy
    eez_set_solar_power(solar_power_w);
    eez_set_battery_soc(battery_soc);
    eez_set_battery_power(battery_power_w);
    eez_set_grid_power(grid_power_w);
    
    // NEW: Update weather
    eez_set_weather(weather_state);
    eez_set_temperature(temperature_c);
    eez_set_humidity(humidity_pct);
    
    lv_refr_now(NULL);
    xSemaphoreGive(lvgl_mutex);
}
```

## Step 5: Design UI in EEZ Studio

1. Open your EEZ Studio project
2. Add new labels/widgets for:
   - `solar_power_label` - Solar power in watts
   - `battery_soc_label` - Battery percentage
   - `battery_bar` - Visual battery level
   - `battery_power_label` - Charging/discharging status
   - `grid_power_label` - Grid import/export
   - `weather_label` - Weather icon/text
   - `temperature_label` - Temperature
   - `humidity_label` - Humidity

3. Export and build

## Quick Test

**To test a single entity first:**

1. Add just one field (e.g., solar_power_w)
2. Fetch it in ha_client_task
3. Create eez_set_solar_power()
4. Add label to EEZ Studio
5. Call from display_task
6. Flash and verify

Then expand to more entities!

## Your Top Priority Entities (IMO)

Based on your setup, I'd add in this order:

1. **Solar power** - sensor.deye_sunsynk_sol_ark_x_2_pv_power_2 (3505W)
2. **Battery SOC** - sensor.battery_soc (68%)
3. **Grid power** - sensor.deye_sunsynk_sol_ark_x_2_grid_power_2 (50W)
4. **Temperature** - sensor.home_apparent_temperature (21.1°C)
5. **Weather** - weather.home (sunny)

This gives you a comprehensive solar energy dashboard! 🌞⚡🔋
