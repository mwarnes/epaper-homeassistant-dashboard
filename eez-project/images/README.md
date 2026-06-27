# E-Paper Image Conversion

This folder contains images for the EEZ Studio UI project.

## Quick Start: Converting Images for E-Paper

E-paper displays require images with **white backgrounds** (not transparency).

### Convert a Single Image

```bash
./convert-for-epaper.sh my-icon.png
```

### Convert Multiple Images

```bash
./convert-for-epaper.sh icon1.png icon2.png icon3.png
```

### Convert All PNG Images

```bash
./convert-for-epaper.sh *.png
```

## What the Script Does

1. ✅ **Backs up original** to `filename-original.png` (keeps your originals safe)
2. ✅ **Removes transparency** from PNG alpha channel
3. ✅ **Replaces with white background** (required for e-paper)
4. ✅ **Shows file sizes** before/after conversion

## After Conversion

1. **Open EEZ Studio**
2. **Delete old images** from Images tab
3. **Add converted images** (use format: **RGB565**)
4. **Build project** in EEZ Studio
5. **Build firmware:** `idf.py build && idf.py flash`

## E-Paper Color Palette

Your display supports **4 colors only**:
- **Black:** `#000000` - Text, icons, borders
- **White:** `#FFFFFF` - Background, highlights  
- **Yellow:** `#FFFF00` - Warnings, accents
- **Red:** `#FF0000` - Alerts, errors

## Image Guidelines

### ✅ DO:
- Use simple, bold graphics
- Keep to 4 colors (black/white/yellow/red)
- Use solid colors (no gradients)
- Create icons without transparency (white background)
- Test on actual e-paper (colors look different than LCD)

### ❌ DON'T:
- Use photographs or complex images
- Use gradients or anti-aliasing
- Rely on transparency (converts to black)
- Use more than 4 colors

## Recommended Image Sizes

- **Small icons:** 24x24 or 32x32
- **Medium icons:** 64x64
- **Large graphics:** 128x128 or 200x200
- **Full screen:** 960x640 (use sparingly)

## Memory Budget

Current app partition: **7MB** (5.75MB free)

| Icon Size | Format | Size/Icon | ~How Many |
|-----------|--------|-----------|-----------|
| 24x24 | RGB565 | ~1.2 KB | ~4,700 icons |
| 64x64 | RGB565 | ~8 KB | ~700 icons |
| 128x128 | RGB565 | ~32 KB | ~180 icons |

**Tip:** Use **Indexed Color** format in EEZ Studio to reduce size by 50-90%!

## Troubleshooting

**Black backgrounds instead of white?**
- Run this script on your images
- Re-import into EEZ Studio
- Rebuild in EEZ Studio
- Clean build: `idf.py fullclean && idf.py build`

**Script doesn't work?**
```bash
# Check if ImageMagick is installed:
magick --version

# Install if needed:
brew install imagemagick
```
