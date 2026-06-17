To add fonts, place them in this directory.

For a font to be registered by the game, it must start with
<width>x<height>, and end with .png (e.g. "7x13_uushi.png").

The helper script `tools/generate-bitmap-font.py` can generate a compatible
PNG atlas from TrueType/OpenType fonts and all text found in the game data and
source string literals. It keeps printable ASCII in the current atlas positions
and appends extra UTF-8 characters after them, writing a JSON map beside the
PNG. By default the generated atlas is packed into multiple rows, which keeps
large CJK glyph sets at practical image sizes. Pass `--single-row` to force the
legacy single-row layout. Glyphs that are wider or taller than the target cell
can be scaled down to fit the fixed game cell size. The script rejects fonts
that only render a missing-glyph box for required characters.

Example:

    tools/generate-bitmap-font.py \
        --font /usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf \
        --ascii-font /usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf \
        --cjk-font /usr/share/fonts/truetype/adobe/dengkuanheiti.ttf \
        --font-size 18 \
        --cjk-font-size 16 \
        --cell 12x24 \
        --atlas-cell 12x32 \
        --ascii-cell 12x24 \
        --ascii-atlas-cell 12x32 \
        --cjk-cell 16x24 \
        --cjk-atlas-cell 16x24 \
        --output installed_files/gfx/fonts/12x24_cjk.png

The script requires Pillow (`python3 -m pip install Pillow`).
