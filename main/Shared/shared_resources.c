#include "shared_resources.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "shared_resources";

// Global instances
dashboard_state_t dashboard_state;
SemaphoreHandle_t dashboard_state_mutex = NULL;
EventGroupHandle_t s_event_group = NULL;

void shared_resources_init(void)
{
    ESP_LOGI(TAG, "Initializing shared resources...");
    
    // Create mutexes
    dashboard_state_mutex = xSemaphoreCreateMutex();
    s_event_group = xEventGroupCreate();
    
    if (!dashboard_state_mutex || !s_event_group) {
        ESP_LOGE(TAG, "Failed to create synchronization primitives");
        abort();
    }
    
    // Initialize state
    memset(&dashboard_state, 0, sizeof(dashboard_state));
    dashboard_state.wifi_connected = false;
    dashboard_state.ha_connected = false;
    dashboard_state.last_successful_update = 0;
    dashboard_state.failed_update_count = 0;
    
    ESP_LOGI(TAG, "Shared resources initialized successfully");
}
