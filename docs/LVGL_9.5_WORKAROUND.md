# LVGL 9.5.0 Missing Header Workaround

## Issue

LVGL 9.5.0 has a bug where the file `src/misc/cache/class/lv_cache_class.h` is missing but referenced in `src/misc/cache/lv_cache.h`.

This causes compilation to fail with:
```
fatal error: class/lv_cache_class.h: No such file or directory
```

## Workaround

After the component manager downloads LVGL, create the missing file:

```bash
cat > managed_components/lvgl__lvgl/src/misc/cache/class/lv_cache_class.h << 'EOF'
/**
 * @file lv_cache_class.h
 * Cache class header - aggregates cache implementations
 * Missing file in LVGL 9.5.0 - added manually
 */

#ifndef LV_CACHE_CLASS_H
#define LV_CACHE_CLASS_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "lv_cache_lru_rb.h"
#include "lv_cache_lru_ll.h"
#include "lv_cache_sc_da.h"

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_CACHE_CLASS_H*/
EOF
```

## Steps

1. **Clean and download fresh components:**
   ```bash
   rm -rf build managed_components
   idf.py reconfigure
   ```

2. **Create the missing header:**
   ```bash
   # Run the cat command above
   ```

3. **Build:**
   ```bash
   idf.py build
   ```

## Permanent Fix

This workaround needs to be reapplied after:
- Running `idf.py fullclean`
- Deleting `managed_components`
- Component manager re-downloads LVGL

## Alternative: Downgrade to LVGL 9.2.x

Edit `main/idf_component.yml`:
```yaml
dependencies:
  lvgl/lvgl: "~9.2.0"  # Locks to 9.2.x
```

However, this may cause compatibility issues with `espressif/esp_lvgl_port` which expects newer LVGL versions.

## Status

- ✅ LVGL 9.5.0 + manual header: **Working**
- ⚠️ LVGL 9.2.x + esp_lvgl_port 2.3-2.4: **Incompatible with ESP-IDF 6.0**
- ✅ LVGL 9.5.0 + esp_lvgl_port 2.8.0: **Working with workaround**

## Upstream Bug Report

This should be reported to LVGL repository:
https://github.com/lvgl/lvgl/issues

The file simply aggregates the three cache implementation headers that already exist in the `class/` directory.
