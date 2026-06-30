#include <string.h>

#include "screens.h"
#include "images.h"
#include "fonts.h"
#include "actions.h"
#include "vars.h"
#include "styles.h"
#include "ui.h"

#include <string.h>

objects_t objects;

//
// Event handlers
//

lv_obj_t *tick_value_change_obj;

//
// Screens
//

void create_screen_main() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.main = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 960, 640);
    {
        lv_obj_t *parent_obj = obj;
        {
            // HaLblTime
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.ha_lbl_time = obj;
            lv_obj_set_pos(obj, 915, 613);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &ui_font_roboto_med_14, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "");
        }
        {
            // HaLblDate
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.ha_lbl_date = obj;
            lv_obj_set_pos(obj, 401, 16);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &ui_font_roboto_med_30, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "");
        }
        {
            // imgWifiState
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.img_wifi_state = obj;
            lv_obj_set_pos(obj, 755, 606);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_wifi_off);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // imgHaState
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.img_ha_state = obj;
            lv_obj_set_pos(obj, 788, 606);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_ha_off);
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            lv_obj_set_pos(obj, 822, 613);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &ui_font_roboto_med_14, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "Last updated: ");
        }
        {
            lv_obj_t *obj = lv_image_create(parent_obj);
            lv_obj_set_pos(obj, 45, 48);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_weather_partly_cloudy_large);
        }
        {
            // HaLblHomeConditionToday
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.ha_lbl_home_condition_today = obj;
            lv_obj_set_pos(obj, 156, 48);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &ui_font_roboto_med_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "");
        }
        {
            // HaLblHomeRealfeelTemperature
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.ha_lbl_home_realfeel_temperature = obj;
            lv_obj_set_pos(obj, 156, 69);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &ui_font_roboto_med_30, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "");
        }
        {
            // img_weather_current
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.img_weather_current = obj;
            lv_obj_set_pos(obj, 28, 48);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_weather_partly_cloudy_large);
        }
        {
            // lbl_forecast_day1_name
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.lbl_forecast_day1_name = obj;
            lv_obj_set_pos(obj, 31, 193);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &ui_font_roboto_med_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "");
        }
        {
            // img_forecast_day1
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.img_forecast_day1 = obj;
            lv_obj_set_pos(obj, 26, 214);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_weather_sunny_small);
        }
        {
            // lbl_forecast_day1_temp
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.lbl_forecast_day1_temp = obj;
            lv_obj_set_pos(obj, 32, 262);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &ui_font_roboto_med_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "");
        }
        {
            // lbl_forecast_day2_name
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.lbl_forecast_day2_name = obj;
            lv_obj_set_pos(obj, 88, 193);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &ui_font_roboto_med_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "");
        }
        {
            // img_forecast_day2
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.img_forecast_day2 = obj;
            lv_obj_set_pos(obj, 82, 214);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_weather_sunny_small);
        }
        {
            // lbl_forecast_day2_temp
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.lbl_forecast_day2_temp = obj;
            lv_obj_set_pos(obj, 88, 262);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &ui_font_roboto_med_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "");
        }
        {
            // lbl_forecast_day3_name
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.lbl_forecast_day3_name = obj;
            lv_obj_set_pos(obj, 145, 193);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &ui_font_roboto_med_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "");
        }
        {
            // lbl_forecast_day3_temp
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.lbl_forecast_day3_temp = obj;
            lv_obj_set_pos(obj, 144, 262);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &ui_font_roboto_med_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "");
        }
        {
            // img_forecast_day3
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.img_forecast_day3 = obj;
            lv_obj_set_pos(obj, 139, 214);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_weather_sunny_small);
        }
        {
            // lbl_forecast_day4_name
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.lbl_forecast_day4_name = obj;
            lv_obj_set_pos(obj, 201, 193);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &ui_font_roboto_med_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "");
        }
        {
            // lbl_forecast_day4_temp
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.lbl_forecast_day4_temp = obj;
            lv_obj_set_pos(obj, 200, 262);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &ui_font_roboto_med_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "");
        }
        {
            // img_forecast_day4
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.img_forecast_day4 = obj;
            lv_obj_set_pos(obj, 195, 214);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_weather_sunny_small);
        }
        {
            // lbl_forecast_day5_name
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.lbl_forecast_day5_name = obj;
            lv_obj_set_pos(obj, 256, 193);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &ui_font_roboto_med_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "");
        }
        {
            // lbl_forecast_day1_temp5
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.lbl_forecast_day1_temp5 = obj;
            lv_obj_set_pos(obj, 256, 262);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &ui_font_roboto_med_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "");
        }
        {
            // img_forecast_day5
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.img_forecast_day5 = obj;
            lv_obj_set_pos(obj, 250, 214);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_weather_sunny_small);
        }
        {
            lv_obj_t *obj = lv_image_create(parent_obj);
            lv_obj_set_pos(obj, 156, 152);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_weather_air_quality);
        }
        {
            lv_obj_t *obj = lv_image_create(parent_obj);
            lv_obj_set_pos(obj, 156, 128);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_weather_humidity);
        }
        {
            lv_obj_t *obj = lv_image_create(parent_obj);
            lv_obj_set_pos(obj, 156, 104);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_weather_wind);
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.obj1 = obj;
            lv_obj_set_pos(obj, 180, 107);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &ui_font_roboto_med_16, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "");
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.obj2 = obj;
            lv_obj_set_pos(obj, 180, 131);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &ui_font_roboto_med_16, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "");
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.obj3 = obj;
            lv_obj_set_pos(obj, 183, 155);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &ui_font_roboto_med_16, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "");
        }
        {
            // img_grid_power
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.img_grid_power = obj;
            lv_obj_set_pos(obj, 601, 93);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_grid_on);
        }
        {
            // img_house_power
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.img_house_power = obj;
            lv_obj_set_pos(obj, 362, 96);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_house_power);
        }
        {
            // img_pv_power
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.img_pv_power = obj;
            lv_obj_set_pos(obj, 440, 93);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_solar_power_day);
        }
        {
            // img_battery_power
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.img_battery_power = obj;
            lv_obj_set_pos(obj, 521, 93);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_battery_power_30);
        }
        {
            // ha_lbl_house_power_day
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.ha_lbl_house_power_day = obj;
            lv_obj_set_pos(obj, 367, 164);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &ui_font_roboto_med_16, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "");
        }
        {
            // ha_lbl_pv_power_day
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.ha_lbl_pv_power_day = obj;
            lv_obj_set_pos(obj, 446, 164);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &ui_font_roboto_med_16, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "");
        }
        {
            // ha_lbl_battery_power_day
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.ha_lbl_battery_power_day = obj;
            lv_obj_set_pos(obj, 527, 164);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &ui_font_roboto_med_16, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "");
        }
        {
            // ha_lbl_grid_power_day
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.ha_lbl_grid_power_day = obj;
            lv_obj_set_pos(obj, 607, 164);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &ui_font_roboto_med_16, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "");
        }
        {
            // ha_lbl_house_power
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.ha_lbl_house_power = obj;
            lv_obj_set_pos(obj, 367, 187);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &ui_font_roboto_med_14, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "");
        }
        {
            // ha_lbl_pv_power
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.ha_lbl_pv_power = obj;
            lv_obj_set_pos(obj, 446, 187);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &ui_font_roboto_med_14, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "");
        }
        {
            // ha_lbl_battery_power
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.ha_lbl_battery_power = obj;
            lv_obj_set_pos(obj, 527, 187);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &ui_font_roboto_med_14, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "");
        }
        {
            // ha_lbl_grid_power
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.ha_lbl_grid_power = obj;
            lv_obj_set_pos(obj, 607, 187);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &ui_font_roboto_med_14, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "");
        }
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.obj0 = obj;
            lv_obj_set_pos(obj, 360, 320);
            lv_obj_set_size(obj, 80, 128);
            lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_image_create(parent_obj);
                    lv_obj_set_pos(obj, 3, 3);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_image_set_src(obj, &img_house_power);
                }
                {
                    // ha_lbl_house_power_1
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.ha_lbl_house_power_1 = obj;
                    lv_obj_set_pos(obj, 13, 94);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_font(obj, &ui_font_roboto_med_14, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "");
                }
            }
        }
        {
            // ha_lbl_house_power_day_1
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.ha_lbl_house_power_day_1 = obj;
            lv_obj_set_pos(obj, 374, 393);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &ui_font_roboto_med_16, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_layout(obj, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "");
        }
    }
    
    tick_screen_main();
}

void tick_screen_main() {
    {
        const char *new_val = get_var_ha_time();
        const char *cur_val = lv_label_get_text(objects.ha_lbl_time);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.ha_lbl_time;
            lv_label_set_text(objects.ha_lbl_time, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = get_var_ha_date();
        const char *cur_val = lv_label_get_text(objects.ha_lbl_date);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.ha_lbl_date;
            lv_label_set_text(objects.ha_lbl_date, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = get_var_ha_home_condition_today();
        const char *cur_val = lv_label_get_text(objects.ha_lbl_home_condition_today);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.ha_lbl_home_condition_today;
            lv_label_set_text(objects.ha_lbl_home_condition_today, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = get_var_ha_home_realfeel_temperature();
        const char *cur_val = lv_label_get_text(objects.ha_lbl_home_realfeel_temperature);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.ha_lbl_home_realfeel_temperature;
            lv_label_set_text(objects.ha_lbl_home_realfeel_temperature, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = get_var_ha_forecast_day1_name();
        const char *cur_val = lv_label_get_text(objects.lbl_forecast_day1_name);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.lbl_forecast_day1_name;
            lv_label_set_text(objects.lbl_forecast_day1_name, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = get_var_ha_forecast_day1_temp();
        const char *cur_val = lv_label_get_text(objects.lbl_forecast_day1_temp);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.lbl_forecast_day1_temp;
            lv_label_set_text(objects.lbl_forecast_day1_temp, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = get_var_ha_forecast_day2_name();
        const char *cur_val = lv_label_get_text(objects.lbl_forecast_day2_name);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.lbl_forecast_day2_name;
            lv_label_set_text(objects.lbl_forecast_day2_name, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = get_var_ha_forecast_day2_temp();
        const char *cur_val = lv_label_get_text(objects.lbl_forecast_day2_temp);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.lbl_forecast_day2_temp;
            lv_label_set_text(objects.lbl_forecast_day2_temp, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = get_var_ha_forecast_day3_name();
        const char *cur_val = lv_label_get_text(objects.lbl_forecast_day3_name);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.lbl_forecast_day3_name;
            lv_label_set_text(objects.lbl_forecast_day3_name, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = get_var_ha_forecast_day3_temp();
        const char *cur_val = lv_label_get_text(objects.lbl_forecast_day3_temp);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.lbl_forecast_day3_temp;
            lv_label_set_text(objects.lbl_forecast_day3_temp, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = get_var_ha_forecast_day4_name();
        const char *cur_val = lv_label_get_text(objects.lbl_forecast_day4_name);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.lbl_forecast_day4_name;
            lv_label_set_text(objects.lbl_forecast_day4_name, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = get_var_ha_forecast_day4_temp();
        const char *cur_val = lv_label_get_text(objects.lbl_forecast_day4_temp);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.lbl_forecast_day4_temp;
            lv_label_set_text(objects.lbl_forecast_day4_temp, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = get_var_ha_forecast_day5_name();
        const char *cur_val = lv_label_get_text(objects.lbl_forecast_day5_name);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.lbl_forecast_day5_name;
            lv_label_set_text(objects.lbl_forecast_day5_name, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = get_var_ha_forecast_day5_temp();
        const char *cur_val = lv_label_get_text(objects.lbl_forecast_day1_temp5);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.lbl_forecast_day1_temp5;
            lv_label_set_text(objects.lbl_forecast_day1_temp5, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = get_var_ha_wind();
        const char *cur_val = lv_label_get_text(objects.obj1);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.obj1;
            lv_label_set_text(objects.obj1, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = get_var_ha_humidity();
        const char *cur_val = lv_label_get_text(objects.obj2);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.obj2;
            lv_label_set_text(objects.obj2, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = get_var_ha_air_quality_pm25();
        const char *cur_val = lv_label_get_text(objects.obj3);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.obj3;
            lv_label_set_text(objects.obj3, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = get_var_ha_house_power_day();
        const char *cur_val = lv_label_get_text(objects.ha_lbl_house_power_day);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.ha_lbl_house_power_day;
            lv_label_set_text(objects.ha_lbl_house_power_day, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = get_var_ha_pv_power_day();
        const char *cur_val = lv_label_get_text(objects.ha_lbl_pv_power_day);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.ha_lbl_pv_power_day;
            lv_label_set_text(objects.ha_lbl_pv_power_day, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = get_var_ha_battery_power_day();
        const char *cur_val = lv_label_get_text(objects.ha_lbl_battery_power_day);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.ha_lbl_battery_power_day;
            lv_label_set_text(objects.ha_lbl_battery_power_day, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = get_var_ha_grid_power_day();
        const char *cur_val = lv_label_get_text(objects.ha_lbl_grid_power_day);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.ha_lbl_grid_power_day;
            lv_label_set_text(objects.ha_lbl_grid_power_day, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = get_var_ha_house_power();
        const char *cur_val = lv_label_get_text(objects.ha_lbl_house_power);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.ha_lbl_house_power;
            lv_label_set_text(objects.ha_lbl_house_power, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = get_var_ha_pv_power();
        const char *cur_val = lv_label_get_text(objects.ha_lbl_pv_power);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.ha_lbl_pv_power;
            lv_label_set_text(objects.ha_lbl_pv_power, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = get_var_ha_battery_power();
        const char *cur_val = lv_label_get_text(objects.ha_lbl_battery_power);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.ha_lbl_battery_power;
            lv_label_set_text(objects.ha_lbl_battery_power, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = get_var_ha_grid_power();
        const char *cur_val = lv_label_get_text(objects.ha_lbl_grid_power);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.ha_lbl_grid_power;
            lv_label_set_text(objects.ha_lbl_grid_power, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = get_var_ha_house_power();
        const char *cur_val = lv_label_get_text(objects.ha_lbl_house_power_1);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.ha_lbl_house_power_1;
            lv_label_set_text(objects.ha_lbl_house_power_1, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = get_var_ha_house_power_day();
        const char *cur_val = lv_label_get_text(objects.ha_lbl_house_power_day_1);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.ha_lbl_house_power_day_1;
            lv_label_set_text(objects.ha_lbl_house_power_day_1, new_val);
            tick_value_change_obj = NULL;
        }
    }
}

typedef void (*tick_screen_func_t)();
tick_screen_func_t tick_screen_funcs[] = {
    tick_screen_main,
};
void tick_screen(int screen_index) {
    if (screen_index >= 0 && screen_index < 1) {
        tick_screen_funcs[screen_index]();
    }
}
void tick_screen_by_id(enum ScreensEnum screenId) {
    tick_screen(screenId - 1);
}

//
// Fonts
//

ext_font_desc_t fonts[] = {
    { "ROBOTO_MED_14", &ui_font_roboto_med_14 },
    { "ROBOTO_14", &ui_font_roboto_14 },
    { "ROBOTO_16", &ui_font_roboto_16 },
    { "ROBOTO_MED_16", &ui_font_roboto_med_16 },
    { "ROBOTO_MED_18", &ui_font_roboto_med_18 },
    { "ROBOTO_MED_30", &ui_font_roboto_med_30 },
#if LV_FONT_MONTSERRAT_8
    { "MONTSERRAT_8", &lv_font_montserrat_8 },
#endif
#if LV_FONT_MONTSERRAT_10
    { "MONTSERRAT_10", &lv_font_montserrat_10 },
#endif
#if LV_FONT_MONTSERRAT_12
    { "MONTSERRAT_12", &lv_font_montserrat_12 },
#endif
#if LV_FONT_MONTSERRAT_14
    { "MONTSERRAT_14", &lv_font_montserrat_14 },
#endif
#if LV_FONT_MONTSERRAT_16
    { "MONTSERRAT_16", &lv_font_montserrat_16 },
#endif
#if LV_FONT_MONTSERRAT_18
    { "MONTSERRAT_18", &lv_font_montserrat_18 },
#endif
#if LV_FONT_MONTSERRAT_20
    { "MONTSERRAT_20", &lv_font_montserrat_20 },
#endif
#if LV_FONT_MONTSERRAT_22
    { "MONTSERRAT_22", &lv_font_montserrat_22 },
#endif
#if LV_FONT_MONTSERRAT_24
    { "MONTSERRAT_24", &lv_font_montserrat_24 },
#endif
#if LV_FONT_MONTSERRAT_26
    { "MONTSERRAT_26", &lv_font_montserrat_26 },
#endif
#if LV_FONT_MONTSERRAT_28
    { "MONTSERRAT_28", &lv_font_montserrat_28 },
#endif
#if LV_FONT_MONTSERRAT_30
    { "MONTSERRAT_30", &lv_font_montserrat_30 },
#endif
#if LV_FONT_MONTSERRAT_32
    { "MONTSERRAT_32", &lv_font_montserrat_32 },
#endif
#if LV_FONT_MONTSERRAT_34
    { "MONTSERRAT_34", &lv_font_montserrat_34 },
#endif
#if LV_FONT_MONTSERRAT_36
    { "MONTSERRAT_36", &lv_font_montserrat_36 },
#endif
#if LV_FONT_MONTSERRAT_38
    { "MONTSERRAT_38", &lv_font_montserrat_38 },
#endif
#if LV_FONT_MONTSERRAT_40
    { "MONTSERRAT_40", &lv_font_montserrat_40 },
#endif
#if LV_FONT_MONTSERRAT_42
    { "MONTSERRAT_42", &lv_font_montserrat_42 },
#endif
#if LV_FONT_MONTSERRAT_44
    { "MONTSERRAT_44", &lv_font_montserrat_44 },
#endif
#if LV_FONT_MONTSERRAT_46
    { "MONTSERRAT_46", &lv_font_montserrat_46 },
#endif
#if LV_FONT_MONTSERRAT_48
    { "MONTSERRAT_48", &lv_font_montserrat_48 },
#endif
};

//
// Color themes
//

uint32_t active_theme_index = 0;

//
//
//

void create_screens() {

// Set default LVGL theme
    lv_display_t *dispp = lv_display_get_default();
    lv_theme_t *theme = lv_theme_default_init(dispp, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED), false, LV_FONT_DEFAULT);
    lv_display_set_theme(dispp, theme);
    
    // Initialize screens
    // Create screens
    create_screen_main();
}