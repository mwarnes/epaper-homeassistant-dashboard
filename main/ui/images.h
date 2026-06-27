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

#ifndef EXT_IMG_DESC_T
#define EXT_IMG_DESC_T
typedef struct _ext_img_desc_t {
    const char *name;
    const lv_img_dsc_t *img_dsc;
} ext_img_desc_t;
#endif

extern const ext_img_desc_t images[32];

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_IMAGES_H*/