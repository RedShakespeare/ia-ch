// =============================================================================
// Copyright Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "i18n.hpp"

#include <algorithm>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

#include "config.hpp"
#include "debug.hpp"
#include "ini.h"
#include "paths.hpp"

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------
static mINI::INIStructure s_ui_ini;
static mINI::INIStructure s_grammar_ini;
static std::string s_current_language = "en";

static std::string locale_root_dir()
{
    return paths::data_dir() + "/locale/";
}

static std::string locale_dir(const std::string& language)
{
    return locale_root_dir() + language + "/";
}

static constexpr const char* kLocaleTextFile = "text.ini";
static constexpr const char* kLegacyLocaleTextFile = "ui.ini";
static constexpr const char* kEmptyLocaleValue = "__EMPTY__";

static std::string ui_ini_path(const std::string& language)
{
    return locale_dir(language) + kLocaleTextFile;
}

static bool has_ui_ini(const std::string& language)
{
    return std::filesystem::exists(ui_ini_path(language));
}

static std::string legacy_ui_ini_path(const std::string& language)
{
    return locale_dir(language) + kLegacyLocaleTextFile;
}

static std::string locale_text_path(const std::string& language)
{
    const auto path = ui_ini_path(language);

    if (std::filesystem::exists(path)) {
        return path;
    }

    const auto legacy_path = legacy_ui_ini_path(language);

    if (std::filesystem::exists(legacy_path)) {
        return legacy_path;
    }

    return path;
}

static std::string decode_locale_escapes(const std::string& value)
{
    std::string result;

    for (size_t i = 0; i < value.size(); ++i) {
        if ((value[i] != '\\') || ((i + 1) >= value.size())) {
            result += value[i];

            continue;
        }

        const char escaped = value[i + 1];

        if (escaped == 'n') {
            result += '\n';
            ++i;
        }
        else if (escaped == 't') {
            result += '\t';
            ++i;
        }
        else if (escaped == '\\') {
            result += '\\';
            ++i;
        }
        else {
            result += value[i];
        }
    }

    return result;
}

// -----------------------------------------------------------------------------
// i18n
// -----------------------------------------------------------------------------
namespace i18n
{
void init()
{
    reload();
}

void reload()
{
    s_ui_ini.clear();
    s_grammar_ini.clear();

    s_current_language = config::language();

    if (s_current_language == "en") {
        return;
    }

    const auto path = locale_text_path(s_current_language);

    if (!std::filesystem::exists(path)) {
        TRACE
            << "Locale ui file not found for language '"
            << s_current_language
            << "', falling back to English"
            << "\n";

        s_current_language = "en";
        return;
    }

    mINI::INIFile file(path);

    if (!file.read(s_ui_ini)) {
        TRACE_ERROR_RELEASE
            << "Unable to read locale ui file: "
            << path
            << "\n";

        s_ui_ini.clear();
        s_current_language = "en";
        return;
    }

    // Load grammar.ini if it exists
    const auto grammar_path = locale_dir(s_current_language) + "grammar.ini";

    if (std::filesystem::exists(grammar_path)) {
        mINI::INIFile grammar_file(grammar_path);

        if (!grammar_file.read(s_grammar_ini)) {
            TRACE_ERROR_RELEASE
                << "Unable to read grammar file: "
                << grammar_path
                << "\n";
        }
    }
}

std::string get(const std::string& key, const std::string& fallback)
{
    if (!s_ui_ini.has("text")) {
        return fallback;
    }

    const auto section = s_ui_ini.get("text");

    if (!section.has(key)) {
        return fallback;
    }

    const auto value = section.get(key);

    if (value.empty()) {
        return fallback;
    }

    if (value == kEmptyLocaleValue) {
        return "";
    }

    return decode_locale_escapes(value);
}

std::string current_language()
{
    return s_current_language;
}

std::vector<std::string> available_languages()
{
    std::vector<std::string> result {"en"};

    if (std::filesystem::exists(locale_root_dir())) {
        for (const auto& entry : std::filesystem::directory_iterator(locale_root_dir())) {
            if (!entry.is_directory()) {
                continue;
            }

            const auto code = entry.path().filename().string();

            if (code == "en") {
                continue;
            }

            if (has_ui_ini(code) || std::filesystem::exists(legacy_ui_ini_path(code))) {
                result.push_back(code);
            }
        }
    }

    std::sort(std::begin(result) + 1, std::end(result));

    return result;
}

std::string language_name(const std::string& code)
{
    if (code == "en") {
        return get("i18n.language.english", "English");
    }

    if (code == "zh_CN") {
        return "简体中文";
    }

    return code;
}

std::string localized_file(
    const std::string& relative_path,
    const std::string& fallback_path)
{
    const auto localized_path = locale_dir(s_current_language) + relative_path;

    if ((s_current_language != "en") && std::filesystem::exists(localized_path)) {
        return localized_path;
    }

    return fallback_path;
}

std::string localized_data_file(const std::string& relative_path)
{
    return localized_file(relative_path, paths::data_dir() + "/" + relative_path);
}

std::string format(
    const std::string& key,
    const std::string& fallback_template,
    const std::unordered_map<std::string, std::string>& args)
{
    // Get template from grammar.ini
    std::string template_str = fallback_template;

    // Parse key into section.key format
    const size_t dot_pos = key.find('.');

    if (dot_pos != std::string::npos) {
        const std::string section = key.substr(0, dot_pos);
        const std::string key_name = key.substr(dot_pos + 1);

        if (s_grammar_ini.has(section)) {
            const auto grammar_section = s_grammar_ini.get(section);

            if (grammar_section.has(key_name)) {
                template_str = grammar_section.get(key_name);
                template_str = decode_locale_escapes(template_str);
            }
        }
    }

    // Replace placeholders: {name} -> value
    std::string result = template_str;

    for (const auto& [placeholder_name, value] : args) {
        const std::string placeholder = "{" + placeholder_name + "}";
        size_t pos = 0;

        while ((pos = result.find(placeholder, pos)) != std::string::npos) {
            result.replace(pos, placeholder.length(), value);
            pos += value.length();
        }
    }

    return result;
}

std::string format(
    const std::string& key,
    const std::string& fallback_template,
    std::initializer_list<std::pair<const std::string, std::string>> args)
{
    std::unordered_map<std::string, std::string> args_map(args);
    return format(key, fallback_template, args_map);
}

}  // namespace i18n
