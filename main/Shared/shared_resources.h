#ifndef SHARED_RESOURCES_H
#define SHARED_RESOURCES_H

#include <stdbool.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"

// Event group bits
#define WIFI_CONNECTED_BIT  BIT0
#define TIME_SYNCED_BIT     BIT1
#define HA_ERROR_BIT        BIT2

// Shared state structure
typedef struct {
    bool wifi_connected;
    bool ha_connected;
    time_t last_successful_update;
    uint32_t failed_update_count;
    
    // HA data (Phase 1: date/time)
    char date_str[32];      // e.g., "2026-06-24"
    char time_str[32];      // e.g., "14:35"
    char timezone[64];      // e.g., "Africa/Johannesburg"
} dashboard_state_t;

// Global shared resources
extern dashboard_state_t dashboard_state;
extern SemaphoreHandle_t dashboard_state_mutex;
extern EventGroupHandle_t s_event_group;

// Initialization
void shared_resources_init(void);

#endif // SHARED_RESOURCES_H
