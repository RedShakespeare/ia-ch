#!/usr/bin/env python3
"""Regression tests for the repository-local i18n CSV exporters."""

from __future__ import annotations

import importlib.util
import sys
import tempfile
import unittest
from pathlib import Path


REPO = Path(__file__).resolve().parents[1]
CODEX_EXPORTER = (
    REPO
    / ".codex"
    / "skills"
    / "export-i18n-source-strings"
    / "scripts"
    / "export_i18n_source_strings.py"
)
CLAUDE_EXPORTER = (
    REPO
    / ".claude"
    / "skills"
    / "export-i18n-source-strings"
    / "scripts"
    / "export_i18n_source_strings.py"
)


def load_exporter():
    spec = importlib.util.spec_from_file_location("ia_i18n_exporter", CODEX_EXPORTER)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot load exporter: {CODEX_EXPORTER}")
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


class ExportI18nSourceStringsTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.exporter = load_exporter()

    def test_codex_and_claude_exporters_stay_in_sync(self):
        self.assertEqual(CODEX_EXPORTER.read_bytes(), CLAUDE_EXPORTER.read_bytes())

    def test_scans_format_and_literal_terrain_helper_calls(self):
        source = """
i18n::get("get.key", "Get source");
i18n::format("format.key", "Format {name}", args);
destroyed_into_rubble("terrain.key", "Destroyed.");
destroyed_into_rubble("", "");
destroyed_into_rubble(dynamic_key, dynamic_source);
void Terrain::destroyed_into_rubble(const std::string& key,
                                    const std::string& source) {}
"""
        with tempfile.TemporaryDirectory() as tmp_dir:
            path = Path(tmp_dir) / "terrain_sample.cpp"
            path.write_text(source, encoding="utf-8")
            entries, skipped = self.exporter.scan_file(path)

        self.assertEqual(
            {entry.index: entry.source for entry in entries},
            {
                "format.key": "Format {name}",
                "get.key": "Get source",
                "terrain.key": "Destroyed.",
            },
        )
        self.assertEqual(skipped, [])

    def test_loads_grammar_locale_as_section_keys(self):
        with tempfile.TemporaryDirectory() as tmp_dir:
            repo = Path(tmp_dir)
            locale_dir = repo / "installed_files" / "data" / "locale" / "test"
            locale_dir.mkdir(parents=True)
            (locale_dir / "grammar.ini").write_text(
                "[attack_melee]\nhit_weapon=translated\\n{target}\n",
                encoding="utf-8",
            )

            translations = self.exporter.load_grammar_locale(repo, "test")

        self.assertEqual(
            translations,
            {"attack_melee.hit_weapon": "translated\n{target}"},
        )

    def test_more_prompt_source_uses_msg_log_fallback(self):
        with tempfile.TemporaryDirectory() as tmp_dir:
            src_dir = Path(tmp_dir) / "src"
            src_dir.mkdir()
            config_path = src_dir / "config.cpp"
            config_path.write_text(
                'const char* key = "option.any_key_confirm_more.descr";\n',
                encoding="utf-8",
            )
            (src_dir / "msg_log.cpp").write_text(
                'i18n::get("msg_log.more_prompt", "[continue]");\n',
                encoding="utf-8",
            )

            entries, skipped = self.exporter.scan_file(config_path)

        self.assertEqual(skipped, [])
        self.assertEqual(len(entries), 1)
        self.assertEqual(entries[0].index, "option.any_key_confirm_more.descr")
        self.assertIn('"[continue]" prompts', entries[0].source)


if __name__ == "__main__":
    unittest.main()
