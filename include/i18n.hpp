// =============================================================================
// Copyright Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef I18N_HPP
#define I18N_HPP

#include <string>
#include <vector>

namespace i18n
{
void init();
void reload();

std::string get(const std::string& key, const std::string& fallback);

std::string current_language();
std::vector<std::string> available_languages();
std::string language_name(const std::string& code);

std::string localized_file(
    const std::string& relative_path,
    const std::string& fallback_path);
std::string localized_data_file(const std::string& relative_path);

}  // namespace i18n

#endif  // I18N_HPP
