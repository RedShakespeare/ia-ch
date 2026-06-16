To add fonts, place them in this directory.

For a font to be registered by the game, it must start with
<width>x<height>, and end with .png (e.g. "7x13_uushi.png").

The helper script `tools/generate-bitmap-font.py` can generate a compatible
PNG atlas from TrueType/OpenType fonts and all text found in the game data and
source string literals. It keeps printable ASCII in the current atlas positions
and appends extra UTF-8 characters after them, writing a JSON map beside the
PNG.

Example:

    tools/generate-bitmap-font.py \
        --font "Noto Sans CJK SC" \
        --font /usr/share/fonts/truetype/unifont/unifont.ttf \
        --font-size 22 \
        --cell 12x24 \
        --output installed_files/gfx/fonts/12x24_cjk.png

The script requires Pillow (`python3 -m pip install Pillow`).
