#include "epaper_diagnostics.h"
#include "gdem102_driver.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "epaper_diag";

void epaper_run_diagnostics(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "E-PAPER DISPLAY DIAGNOSTICS");
    ESP_LOGI(TAG, "========================================");
    
    // Test 1: Full White Screen
    ESP_LOGI(TAG, "Test 1: Full white screen (clearing test)");
    gdem102_clear(GDEM102_COLOR_WHITE);
    ESP_LOGI(TAG, "  Framebuffer cleared to white");
    gdem102_flush();
    ESP_LOGI(TAG, "  Physical display refreshed");
    ESP_LOGI(TAG, "  CHECK DISPLAY: Should be completely white");
    ESP_LOGI(TAG, "  - Are there any black lines visible? (Y/N)");
    ESP_LOGI(TAG, "  - If YES: Note their position (top/middle/bottom)");
    vTaskDelay(pdMS_TO_TICKS(30000));  // Wait 30s to observe
    
    // Test 2: Full Black Screen
    ESP_LOGI(TAG, "Test 2: Full black screen");
    gdem102_clear(GDEM102_COLOR_BLACK);
    ESP_LOGI(TAG, "  Framebuffer cleared to black");
    gdem102_flush();
    ESP_LOGI(TAG, "  Physical display refreshed");
    ESP_LOGI(TAG, "  CHECK DISPLAY: Should be completely black");
    ESP_LOGI(TAG, "  - Are there any white/gray lines visible? (Y/N)");
    vTaskDelay(pdMS_TO_TICKS(30000));
    
    // Test 3: Horizontal Bars (Test for line artifacts)
    ESP_LOGI(TAG, "Test 3: Horizontal color bars");
    gdem102_clear(GDEM102_COLOR_WHITE);
    
    // Draw 4 horizontal bars
    int bar_height = 640 / 4;
    gdem102_fill_rect(0, 0, 960, bar_height, GDEM102_COLOR_BLACK);
    gdem102_fill_rect(0, bar_height, 960, bar_height, GDEM102_COLOR_WHITE);
    gdem102_fill_rect(0, bar_height * 2, 960, bar_height, GDEM102_COLOR_YELLOW);
    gdem102_fill_rect(0, bar_height * 3, 960, bar_height, GDEM102_COLOR_RED);
    
    ESP_LOGI(TAG, "  Drawing horizontal bars:");
    ESP_LOGI(TAG, "    Top (0-159):    BLACK");
    ESP_LOGI(TAG, "    Upper (160-319): WHITE");
    ESP_LOGI(TAG, "    Lower (320-479): YELLOW");
    ESP_LOGI(TAG, "    Bottom (480-639): RED");
    gdem102_flush();
    ESP_LOGI(TAG, "  CHECK DISPLAY: Note position of any artifacts");
    ESP_LOGI(TAG, "  - Do lines appear in specific color regions?");
    ESP_LOGI(TAG, "  - Do lines cross bar boundaries?");
    vTaskDelay(pdMS_TO_TICKS(30000));
    
    // Test 4: Vertical Bars
    ESP_LOGI(TAG, "Test 4: Vertical color bars");
    gdem102_clear(GDEM102_COLOR_WHITE);
    
    int bar_width = 960 / 4;
    gdem102_fill_rect(0, 0, bar_width, 640, GDEM102_COLOR_BLACK);
    gdem102_fill_rect(bar_width, 0, bar_width, 640, GDEM102_COLOR_WHITE);
    gdem102_fill_rect(bar_width * 2, 0, bar_width, 640, GDEM102_COLOR_YELLOW);
    gdem102_fill_rect(bar_width * 3, 0, bar_width, 640, GDEM102_COLOR_RED);
    
    ESP_LOGI(TAG, "  Drawing vertical bars:");
    ESP_LOGI(TAG, "    Left (0-239):   BLACK");
    ESP_LOGI(TAG, "    Mid-L (240-479): WHITE");
    ESP_LOGI(TAG, "    Mid-R (480-719): YELLOW");
    ESP_LOGI(TAG, "    Right (720-959): RED");
    gdem102_flush();
    ESP_LOGI(TAG, "  CHECK DISPLAY: Are lines horizontal or vertical?");
    ESP_LOGI(TAG, "  - Horizontal lines suggest scan line issue");
    ESP_LOGI(TAG, "  - Vertical lines suggest column driver issue");
    vTaskDelay(pdMS_TO_TICKS(30000));
    
    // Test 5: Checkerboard Pattern (8x8 pixel blocks)
    ESP_LOGI(TAG, "Test 5: Checkerboard pattern (8x8 blocks)");
    gdem102_clear(GDEM102_COLOR_WHITE);
    
    for (int y = 0; y < 640; y += 8) {
        for (int x = 0; x < 960; x += 8) {
            gdem102_color_t color = ((x / 8) + (y / 8)) % 2 ? GDEM102_COLOR_BLACK : GDEM102_COLOR_WHITE;
            gdem102_fill_rect(x, y, 8, 8, color);
        }
    }
    
    ESP_LOGI(TAG, "  Drawing checkerboard (8x8 blocks)");
    gdem102_flush();
    ESP_LOGI(TAG, "  CHECK DISPLAY: Checkerboard pattern");
    ESP_LOGI(TAG, "  - Do lines appear in a regular pattern?");
    ESP_LOGI(TAG, "  - Are lines always at same pixel rows?");
    vTaskDelay(pdMS_TO_TICKS(30000));
    
    // Test 6: Double Clear Test (Ghosting check)
    ESP_LOGI(TAG, "Test 6: Double clear test (ghosting check)");
    
    // First: Draw complex pattern
    gdem102_clear(GDEM102_COLOR_BLACK);
    for (int i = 0; i < 640; i += 20) {
        gdem102_draw_line(0, i, 959, i, GDEM102_COLOR_WHITE);
    }
    gdem102_flush();
    ESP_LOGI(TAG, "  Step 1: Drew white horizontal lines on black");
    vTaskDelay(pdMS_TO_TICKS(30000));
    
    // Second: Clear once
    gdem102_clear(GDEM102_COLOR_WHITE);
    gdem102_flush();
    ESP_LOGI(TAG, "  Step 2: Cleared to white (single clear)");
    ESP_LOGI(TAG, "  CHECK DISPLAY: Can you see ghost of previous pattern?");
    ESP_LOGI(TAG, "  - If YES: This is ghosting/refresh issue");
    ESP_LOGI(TAG, "  - If NO: Lines are not ghosting-related");
    vTaskDelay(pdMS_TO_TICKS(30000));
    
    // Test 7: Pixel-by-Pixel Write Test (Top 100 rows)
    ESP_LOGI(TAG, "Test 7: Pixel-by-pixel write test (top 100 rows)");
    gdem102_clear(GDEM102_COLOR_WHITE);
    
    ESP_LOGI(TAG, "  Writing individual pixels to top 100 rows...");
    for (int y = 0; y < 100; y++) {
        for (int x = 0; x < 960; x++) {
            // Alternate pattern: every other pixel
            if ((x + y) % 2 == 0) {
                gdem102_draw_pixel(x, y, GDEM102_COLOR_BLACK);
            }
        }
    }
    
    gdem102_flush();
    ESP_LOGI(TAG, "  CHECK DISPLAY: Top 100 rows should have dot pattern");
    ESP_LOGI(TAG, "  - Rest should be white");
    ESP_LOGI(TAG, "  - Do lines appear in the patterned area or white area?");
    vTaskDelay(pdMS_TO_TICKS(30000));
    
    // Test 8: Framebuffer Dump (First 100 bytes)
    ESP_LOGI(TAG, "Test 8: Framebuffer state check");
    gdem102_clear(GDEM102_COLOR_WHITE);
    
    // Try to read back framebuffer state (if driver exposes it)
    ESP_LOGI(TAG, "  Framebuffer cleared to WHITE");
    ESP_LOGI(TAG, "  Expected: All pixels should be 0x01 (white)");
    ESP_LOGI(TAG, "  If you see 0x00 (black) in logs, driver has issue");
    
    gdem102_flush();
    vTaskDelay(pdMS_TO_TICKS(30000));
    
    // Final Test: Return to normal white screen
    ESP_LOGI(TAG, "Test 9: Final clear to white");
    gdem102_clear(GDEM102_COLOR_WHITE);
    gdem102_flush();
    ESP_LOGI(TAG, "  Returned to white screen");
    
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "DIAGNOSTICS COMPLETE");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "ANALYSIS GUIDE:");
    ESP_LOGI(TAG, "1. Lines appear only after LVGL rendering:");
    ESP_LOGI(TAG, "   → Software issue (LVGL/flush callback)");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "2. Lines appear on all tests (even Test 1):");
    ESP_LOGI(TAG, "   → Hardware defect (display panel or controller)");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "3. Lines appear only on certain colors:");
    ESP_LOGI(TAG, "   → Color mapping issue in driver");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "4. Lines disappear after double clear:");
    ESP_LOGI(TAG, "   → Ghosting/refresh timing issue");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "5. Lines always at same pixel rows:");
    ESP_LOGI(TAG, "   → Hardware defect (stuck row drivers)");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "6. Lines position varies:");
    ESP_LOGI(TAG, "   → Software issue (buffer corruption)");
    ESP_LOGI(TAG, "========================================");
}
