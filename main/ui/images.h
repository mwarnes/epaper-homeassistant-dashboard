#ifndef EEZ_LVGL_UI_IMAGES_H
#define EEZ_LVGL_UI_IMAGES_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const lv_img_dsc_t img_wifi_off;
extern const lv_img_dsc_t img_wifi_on;
extern const lv_img_dsc_t img_ha_on;
extern const lv_img_dsc_t img_ha_off;
extern const lv_img_dsc_t img_weather_partly_cloudy_large;
extern const lv_img_dsc_t img_weather_partly_cloudy_small;
extern const lv_img_dsc_t img_weather_alert_circle_large;
extern const lv_img_dsc_t img_weather_alert_circle_small;
extern const lv_img_dsc_t img_weather_cloudy_large;
extern const lv_img_dsc_t img_weather_cloudy_small;
extern const lv_img_dsc_t img_weather_fog_large;
extern const lv_img_dsc_t img_weather_fog_small;
extern const lv_img_dsc_t img_weather_hail_large;
extern const lv_img_dsc_t img_weather_hail_small;
extern const lv_img_dsc_t img_weather_lightning_large;
extern const lv_img_dsc_t img_weather_lightning_small;
extern const lv_img_dsc_t img_weather_lightning_rainy_large;
extern const lv_img_dsc_t img_weather_lightning_rainy_small;
extern const lv_img_dsc_t img_weather_night_large;
extern const lv_img_dsc_t img_weather_night_small;
extern const lv_img_dsc_t img_weather_pouring_large;
extern const lv_img_dsc_t img_weather_pouring_small;
extern const lv_img_dsc_t img_weather_rainy_large;
extern const lv_img_dsc_t img_weather_rainy_small;
extern const lv_img_dsc_t img_weather_snowy_large;
extern const lv_img_dsc_t img_weather_snowy_small;
extern const lv_img_dsc_t img_weather_snowy_rainy_large;
extern const lv_img_dsc_t img_weather_snowy_rainy_small;
extern const lv_img_dsc_t img_weather_sunny_large;
extern const lv_img_dsc_t img_weather_sunny_small;
extern const lv_img_dsc_t img_weather_windy_large;
extern const lv_img_dsc_t img_weather_windy_small;
extern const lv_img_dsc_t img_weather_air_quality;
extern const lv_img_dsc_t img_weather_humidity;
extern const lv_img_dsc_t img_weather_wind;
extern const lv_img_dsc_t img_grid_on;
extern const lv_img_dsc_t img_grid_off;
extern const lv_img_dsc_t img_house_power;
extern const lv_img_dsc_t img_solar_power_day;
extern const lv_img_dsc_t img_battery_power_10;
extern const lv_img_dsc_t img_battery_power_20;
extern const lv_img_dsc_t img_battery_power_30;
extern const lv_img_dsc_t img_battery_power_40;
extern const lv_img_dsc_t img_battery_power_50;
extern const lv_img_dsc_t img_battery_power_60;
extern const lv_img_dsc_t img_battery_power_70;
extern const lv_img_dsc_t img_battery_power_80;
extern const lv_img_dsc_t img_battery_power_90;
extern const lv_img_dsc_t img_battery_power_100;
extern const lv_img_dsc_t img_solar_power_night;

#ifndef EXT_IMG_DESC_T
#define EXT_IMG_DESC_T
typedef struct _ext_img_desc_t {
    const char *name;
    const lv_img_dsc_t *img_dsc;
} ext_img_desc_t;
#endif

extern const ext_img_desc_t images[50];

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_IMAGES_H*/