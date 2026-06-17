#!/usr/bin/env python3
"""
Unit tests for the bitmap font generator.
"""

from __future__ import annotations

import importlib.util
import json
import sys
import tempfile
import unittest
from pathlib import Path

from PIL import Image


REPO_ROOT = Path(__file__).resolve().parents[1]
GENERATOR_PATH = REPO_ROOT / "tools" / "generate-bitmap-font.py"
DEJAVU_MONO = Path("/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf")
UNIFONT = Path("/usr/share/fonts/truetype/unifont/unifont.ttf")


def load_generator():
    spec = importlib.util.spec_from_file_location("generate_bitmap_font", GENERATOR_PATH)
    module = importlib.util.module_from_spec(spec)
    assert spec.loader is not None
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


class BitmapFontGeneratorTest(unittest.TestCase):
    def setUp(self):
        if not DEJAVU_MONO.exists() or not UNIFONT.exists():
            self.skipTest("required system test fonts are not installed")

        self.generator = load_generator()

    def test_mixed_atlas_keeps_atlas_slots_separate_from_logical_size(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            output = Path(temp_dir) / "mixed.png"
            map_output = Path(temp_dir) / "mixed.json"
            default_layout = self.generator.GlyphLayout(12, 24, 12, 32)
            ascii_layout = self.generator.GlyphLayout(12, 24, 12, 32)
            cjk_layout = self.generator.GlyphLayout(16, 24, 16, 24)

            fonts_used, glyphs, columns = self.generator.render_font_png(
                chars=["A", "新"],
                fonts=[str(DEJAVU_MONO)],
                font_size=18,
                default_layout=default_layout,
                ascii_fonts=[str(DEJAVU_MONO)],
                ascii_font_size=18,
                ascii_layout=ascii_layout,
                cjk_fonts=[str(UNIFONT)],
                cjk_font_size=16,
                cjk_layout=cjk_layout,
                columns=0,
                single_row=True,
                output=output,
                ink=255,
                allow_missing=False,
                fit_glyphs=False,
            )

            self.generator.write_map(
                chars=["A", "新"],
                cjk_words=["新"],
                columns=columns,
                default_layout=default_layout,
                glyphs=glyphs,
                output=map_output,
                fonts_used=fonts_used,
            )

            with Image.open(output) as image:
                self.assertEqual(image.size, (29, 32))

            raw_map = map_output.read_text(encoding="utf-8")
            self.assertIn('"\\u65b0": {', raw_map)

            data = json.loads(raw_map)
            self.assertEqual(data["cell"], {"width": 12, "height": 24})
            self.assertEqual(data["atlas_cell"], {"width": 16, "height": 32})
            self.assertEqual(data["glyphs"]["A"]["width"], 12)
            self.assertEqual(data["glyphs"]["A"]["height"], 32)
            self.assertEqual(data["glyphs"]["A"]["logical_width"], 12)
            self.assertEqual(data["glyphs"]["A"]["logical_height"], 24)
            self.assertEqual(data["glyphs"]["A"]["advance"], 12)
            self.assertEqual(data["glyphs"]["A"]["render_offset_y"], -4)
            self.assertEqual(data["glyphs"]["新"]["x_px"], 13)
            self.assertEqual(data["glyphs"]["新"]["width"], 16)
            self.assertEqual(data["glyphs"]["新"]["height"], 24)
            self.assertEqual(data["glyphs"]["新"]["logical_width"], 16)
            self.assertEqual(data["glyphs"]["新"]["logical_height"], 24)
            self.assertEqual(data["glyphs"]["新"]["advance"], 16)
            self.assertEqual(data["glyphs"]["新"]["render_offset_y"], 0)

    def test_mixed_atlas_wraps_to_multiple_rows(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            output = Path(temp_dir) / "wrapped.png"
            map_output = Path(temp_dir) / "wrapped.json"
            default_layout = self.generator.GlyphLayout(12, 24, 12, 32)
            ascii_layout = self.generator.GlyphLayout(12, 24, 12, 32)
            cjk_layout = self.generator.GlyphLayout(16, 24, 16, 24)
            chars = ["A", "B", "新", "旧"]

            fonts_used, glyphs, columns = self.generator.render_font_png(
                chars=chars,
                fonts=[str(DEJAVU_MONO)],
                font_size=18,
                default_layout=default_layout,
                ascii_fonts=[str(DEJAVU_MONO)],
                ascii_font_size=18,
                ascii_layout=ascii_layout,
                cjk_fonts=[str(UNIFONT)],
                cjk_font_size=16,
                cjk_layout=cjk_layout,
                columns=2,
                single_row=False,
                output=output,
                ink=255,
                allow_missing=False,
                fit_glyphs=False,
            )

            self.generator.write_map(
                chars=chars,
                cjk_words=["新旧"],
                columns=columns,
                default_layout=default_layout,
                glyphs=glyphs,
                output=map_output,
                fonts_used=fonts_used,
            )

            with Image.open(output) as image:
                self.assertEqual(image.size, (33, 56))

            data = json.loads(map_output.read_text(encoding="utf-8"))
            self.assertEqual(columns, 2)
            self.assertEqual(data["glyphs"]["A"]["x_px"], 0)
            self.assertEqual(data["glyphs"]["A"]["y_px"], 0)
            self.assertEqual(data["glyphs"]["B"]["x_px"], 13)
            self.assertEqual(data["glyphs"]["B"]["y_px"], 0)
            self.assertEqual(data["glyphs"]["新"]["x_px"], 0)
            self.assertEqual(data["glyphs"]["新"]["y_px"], 32)
            self.assertEqual(data["glyphs"]["旧"]["x_px"], 17)
            self.assertEqual(data["glyphs"]["旧"]["y_px"], 32)

    def test_single_row_preserves_legacy_horizontal_layout(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            output = Path(temp_dir) / "single-row.png"
            default_layout = self.generator.GlyphLayout(12, 24, 12, 32)

            _, glyphs, columns = self.generator.render_font_png(
                chars=["A", "B", "C"],
                fonts=[str(DEJAVU_MONO)],
                font_size=18,
                default_layout=default_layout,
                ascii_fonts=[str(DEJAVU_MONO)],
                ascii_font_size=18,
                ascii_layout=default_layout,
                cjk_fonts=[str(UNIFONT)],
                cjk_font_size=16,
                cjk_layout=default_layout,
                columns=0,
                single_row=True,
                output=output,
                ink=255,
                allow_missing=False,
                fit_glyphs=False,
            )

            self.assertEqual(columns, 3)
            self.assertEqual(glyphs[0].x_px, 0)
            self.assertEqual(glyphs[1].x_px, 13)
            self.assertEqual(glyphs[2].x_px, 26)
            self.assertEqual(glyphs[2].y_px, 0)


if __name__ == "__main__":
    unittest.main()
