// NUCLEAR OPTION: Recreate arc instead of changing style
// Replace eez_cycle_spinner_color() in main/eez_vars.c with this if invalidation doesn't work

void eez_cycle_spinner_color(void)
{
    static uint8_t color_index = 0;
    
    lv_color_t color;
    const char *color_name;
    
    switch (color_index) {
        case 0:
            color = lv_color_black();
            color_name = "BLACK";
            break;
        case 1:
            color = lv_color_make(255, 0, 0);  // Red
            color_name = "RED";
            break;
        case 2:
            color = lv_color_make(255, 255, 0);  // Yellow
            color_name = "YELLOW";
            break;
        default:
            color = lv_color_black();
            color_name = "BLACK";
            color_index = 0;
            break;
    }
    
    // Update spinner (disabled - animation causes issues)
    if (objects.spinner_demo) {
        lv_obj_set_style_arc_color(objects.spinner_demo, color, LV_PART_INDICATOR);
        lv_obj_invalidate(objects.spinner_demo);
    }
    
    // Update arc by DESTROYING and RECREATING it
    if (objects.arc_color_test) {
        // Get parent before deleting
        lv_obj_t *parent = lv_obj_get_parent(objects.arc_color_test);
        
        // Destroy old arc (this WILL mark area as dirty)
        lv_obj_del(objects.arc_color_test);
        objects.arc_color_test = NULL;
        
        // Create new arc with NEW color
        lv_obj_t *obj = lv_arc_create(parent);
        objects.arc_color_test = obj;
        
        // Position and size
        lv_obj_set_pos(obj, 500, 200);
        lv_obj_set_size(obj, 100, 100);
        
        // Arc settings
        lv_arc_set_range(obj, 0, 100);
        lv_arc_set_value(obj, 75);  // 75% arc
        lv_arc_set_bg_angles(obj, 0, 360);  // Full circle background
        
        // Set NEW color
        lv_obj_set_style_arc_color(obj, color, LV_PART_INDICATOR);
        lv_obj_set_style_arc_width(obj, 10, LV_PART_INDICATOR);
        
        // Hide background arc (only show indicator)
        lv_obj_set_style_arc_width(obj, 0, LV_PART_MAIN);
        
        // Remove knob
        lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, LV_PART_KNOB);
        
        ESP_LOGI(TAG, "Arc RECREATED with %s color (cycle %d) - LVGL color: R=%d G=%d B=%d", 
                 color_name, color_index, color.red, color.green, color.blue);
    } else {
        ESP_LOGW(TAG, "Arc object not available");
    }
    
    // Cycle to next color
    color_index = (color_index + 1) % 3;
}
