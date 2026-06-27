#ifndef EEZ_LVGL_UI_SCREENS_H
#define EEZ_LVGL_UI_SCREENS_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

// Screens

enum ScreensEnum {
    _SCREEN_ID_FIRST = 1,
    SCREEN_ID_MAIN = 1,
    _SCREEN_ID_LAST = 1
};

typedef struct _objects_t {
    lv_obj_t *main;
    lv_obj_t *ha_lbl_time;
    lv_obj_t *ha_lbl_date;
    lv_obj_t *img_wifi_state;
    lv_obj_t *img_ha_state;
    lv_obj_t *ha_lbl_home_condition_today;
    lv_obj_t *ha_lbl_home_realfeel_temperature;
    lv_obj_t *img_weather_current;
    lv_obj_t *lbl_forecast_day1_name;
    lv_obj_t *img_forecast_day1;
    lv_obj_t *lbl_forecast_day1_temp;
    lv_obj_t *lbl_forecast_day2_name;
    lv_obj_t *img_forecast_day2;
    lv_obj_t *lbl_forecast_day2_temp;
    lv_obj_t *lbl_forecast_day3_name;
    lv_obj_t *lbl_forecast_day3_temp;
    lv_obj_t *img_forecast_day3;
    lv_obj_t *lbl_forecast_day4_name;
    lv_obj_t *lbl_forecast_day4_temp;
    lv_obj_t *img_forecast_day4;
    lv_obj_t *lbl_forecast_day5_name;
    lv_obj_t *lbl_forecast_day1_temp5;
    lv_obj_t *img_forecast_day5;
} objects_t;

extern objects_t objects;

void create_screen_main();
void tick_screen_main();

void tick_screen_by_id(enum ScreensEnum screenId);
void tick_screen(int screen_index);

void create_screens();

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_SCREENS_H*/