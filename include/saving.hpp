// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef SAVING_HPP
#define SAVING_HPP

#include <string>

namespace saving
{

void init();

void save_game();

void load_game();

void erase_save();

bool is_save_available();

bool is_loading();

//Functions called by modules when saving and loading.
void put_str(const std::string str);
void put_int(const int v);
void put_bool(const bool v);

std::string get_str();
int get_int();
bool get_bool();

} // saving

#endif // SAVING_HPP
