#ifndef EEZ_VARS_H
#define EEZ_VARS_H

#include <stdbool.h>

// Update functions for UI widgets
void eez_set_date(const char *date);
void eez_set_time(const char *time);
void eez_set_wifi_status(bool connected);
void eez_set_ha_status(bool connected);

// Screen management (placeholder - extend when you add error screen in EEZ Studio)
void eez_show_error_screen(const char *wifi_status, const char *ha_status, const char *last_update);
void eez_show_dashboard_screen(void);

#endif // EEZ_VARS_H
