#ifndef EPAPER_DIAGNOSTICS_H
#define EPAPER_DIAGNOSTICS_H

/**
 * @brief Run comprehensive e-paper display diagnostics
 * 
 * This function runs a series of visual tests to help diagnose
 * display issues such as ghosting, stuck pixels, or hardware defects.
 * 
 * Each test runs for 30 seconds to allow visual inspection.
 * Total runtime: ~4.5 minutes
 * 
 * Tests performed:
 * 1. Full white screen (baseline)
 * 2. Full black screen (inverse test)
 * 3. Horizontal color bars (line detection)
 * 4. Vertical color bars (orientation test)
 * 5. Checkerboard pattern (pixel accuracy)
 * 6. Ghosting test (refresh quality)
 * 7. Pixel-by-pixel write (driver test)
 * 8. Framebuffer state check
 * 9. Final clear
 * 
 * @note Call this from main.c INSTEAD of normal display_task
 *       for diagnostic session
 */
void epaper_run_diagnostics(void);

#endif // EPAPER_DIAGNOSTICS_H
