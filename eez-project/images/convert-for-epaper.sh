#!/bin/bash
#
# convert-for-epaper.sh
# Converts PNG images for e-paper display by removing transparency
# and replacing it with white background.
#
# Usage:
#   ./convert-for-epaper.sh image.png
#   ./convert-for-epaper.sh image1.png image2.png image3.png
#   ./convert-for-epaper.sh *.png
#

set -e  # Exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Check if ImageMagick is installed
if ! command -v magick &> /dev/null; then
    echo -e "${RED}Error: ImageMagick is not installed!${NC}"
    echo "Install it with: brew install imagemagick"
    exit 1
fi

# Check if any arguments provided
if [ $# -eq 0 ]; then
    echo "Usage: $0 <image.png> [image2.png ...]"
    echo ""
    echo "Example:"
    echo "  $0 wifi-on.png"
    echo "  $0 *.png"
    echo ""
    echo "This script removes transparency from PNG images and replaces"
    echo "it with a white background (required for e-paper displays)."
    exit 1
fi

echo -e "${GREEN}E-Paper Image Converter${NC}"
echo "Converting images to white background (no transparency)..."
echo ""

converted=0
skipped=0
errors=0

for image in "$@"; do
    # Skip backup files
    if [[ "$image" == *"-original.png" ]]; then
        echo -e "${YELLOW}⏭  Skipping backup file: $image${NC}"
        ((skipped++))
        continue
    fi
    
    # Check if file exists
    if [ ! -f "$image" ]; then
        echo -e "${RED}✗ File not found: $image${NC}"
        ((errors++))
        continue
    fi
    
    # Check if it's a PNG
    if [[ ! "$image" =~ \.png$ ]]; then
        echo -e "${YELLOW}⏭  Skipping non-PNG file: $image${NC}"
        ((skipped++))
        continue
    fi
    
    # Get filename without extension
    filename="${image%.png}"
    backup="${filename}-original.png"
    
    # Create backup if it doesn't exist
    if [ ! -f "$backup" ]; then
        cp "$image" "$backup"
        echo -e "📦 Backed up: ${YELLOW}$backup${NC}"
    else
        echo -e "📦 Backup exists: ${YELLOW}$backup${NC}"
    fi
    
    # Get original file info
    size_before=$(stat -f%z "$image" 2>/dev/null || stat -c%s "$image" 2>/dev/null)
    
    # Convert: remove alpha channel, replace with white
    if magick "$image" -background white -alpha remove -alpha off "$image" 2>/dev/null; then
        size_after=$(stat -f%z "$image" 2>/dev/null || stat -c%s "$image" 2>/dev/null)
        
        # Get image info
        info=$(magick identify "$image" 2>/dev/null | awk '{print $3, $6}')
        
        echo -e "${GREEN}✓ Converted: $image${NC}"
        echo -e "  Size: $size_before → $size_after bytes"
        echo -e "  Info: $info"
        echo ""
        
        ((converted++))
    else
        echo -e "${RED}✗ Failed to convert: $image${NC}"
        echo ""
        ((errors++))
    fi
done

# Summary
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo -e "${GREEN}✓ Converted: $converted${NC}"
if [ $skipped -gt 0 ]; then
    echo -e "${YELLOW}⏭ Skipped: $skipped${NC}"
fi
if [ $errors -gt 0 ]; then
    echo -e "${RED}✗ Errors: $errors${NC}"
fi
echo ""

if [ $converted -gt 0 ]; then
    echo -e "${GREEN}Next steps:${NC}"
    echo "1. Open EEZ Studio"
    echo "2. Re-import the converted images (delete old ones first)"
    echo "3. Use format: RGB565 (or Indexed Color for smaller size)"
    echo "4. Build in EEZ Studio"
    echo "5. Run: idf.py build && idf.py flash"
fi

exit 0
