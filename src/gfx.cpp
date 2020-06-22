// =============================================================================
// Copyright 2011-2020 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "gfx.hpp"

#include <unordered_map>

#include "pos.hpp"

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------
typedef std::unordered_map<std::string, gfx::TileId> StrToTileIdMap;

static const StrToTileIdMap s_str_to_tile_id_map = {
        {"aim_marker_head", gfx::TileId::aim_marker_head},
        {"aim_marker_line", gfx::TileId::aim_marker_line},
        {"alchemist_bench_empty", gfx::TileId::alchemist_bench_empty},
        {"alchemist_bench_full", gfx::TileId::alchemist_bench_full},
        {"altar", gfx::TileId::altar},
        {"gong", gfx::TileId::gong},
        {"ammo", gfx::TileId::ammo},
        {"amoeba", gfx::TileId::amoeba},
        {"amulet", gfx::TileId::amulet},
        {"ape", gfx::TileId::ape},
        {"armor", gfx::TileId::armor},
        {"axe", gfx::TileId::axe},
        {"barrel", gfx::TileId::barrel},
        {"bat", gfx::TileId::bat},
        {"blast1", gfx::TileId::blast1},
        {"blast2", gfx::TileId::blast2},
        {"bog_tcher", gfx::TileId::bog_tcher},
        {"bookshelf_empty", gfx::TileId::bookshelf_empty},
        {"bookshelf_full", gfx::TileId::bookshelf_full},
        {"brain", gfx::TileId::brain},
        {"brazier", gfx::TileId::brazier},
        {"bush", gfx::TileId::bush},
        {"byakhee", gfx::TileId::byakhee},
        {"cabinet_closed", gfx::TileId::cabinet_closed},
        {"cabinet_open", gfx::TileId::cabinet_open},
        {"cave_wall_front", gfx::TileId::cave_wall_front},
        {"cave_wall_top", gfx::TileId::cave_wall_top},
        {"chains", gfx::TileId::chains},
        {"chest_closed", gfx::TileId::chest_closed},
        {"chest_open", gfx::TileId::chest_open},
        {"chthonian", gfx::TileId::chthonian},
        {"church_bench", gfx::TileId::church_bench},
        {"clockwork", gfx::TileId::clockwork},
        {"club", gfx::TileId::club},
        {"cocoon_closed", gfx::TileId::cocoon_closed},
        {"cocoon_open", gfx::TileId::cocoon_open},
        {"corpse", gfx::TileId::corpse},
        {"corpse2", gfx::TileId::corpse2},
        {"crawling_hand", gfx::TileId::crawling_hand},
        {"crawling_intestines", gfx::TileId::crawling_intestines},
        {"croc_head_mummy", gfx::TileId::croc_head_mummy},
        {"crowbar", gfx::TileId::crowbar},
        {"crystal", gfx::TileId::crystal},
        {"cultist_dagger", gfx::TileId::cultist_dagger},
        {"cultist_firearm", gfx::TileId::cultist_firearm},
        {"cultist_spiked_mace", gfx::TileId::cultist_spiked_mace},
        {"dagger", gfx::TileId::dagger},
        {"deep_one", gfx::TileId::deep_one},
        {"device1", gfx::TileId::device1},
        {"device2", gfx::TileId::device2},
        {"door_broken", gfx::TileId::door_broken},
        {"door_closed", gfx::TileId::door_closed},
        {"door_open", gfx::TileId::door_open},
        {"dynamite", gfx::TileId::dynamite},
        {"dynamite_lit", gfx::TileId::dynamite_lit},
        {"egypt_wall_front", gfx::TileId::egypt_wall_front},
        {"egypt_wall_top", gfx::TileId::egypt_wall_top},
        {"elder_sign", gfx::TileId::elder_sign},
        {"excl_mark", gfx::TileId::excl_mark},
        {"fiend", gfx::TileId::fiend},
        {"flare", gfx::TileId::flare},
        {"flare_gun", gfx::TileId::flare_gun},
        {"flare_lit", gfx::TileId::flare_lit},
        {"floating_skull", gfx::TileId::floating_skull},
        {"floor", gfx::TileId::floor},
        {"fountain", gfx::TileId::fountain},
        {"fungi", gfx::TileId::fungi},
        {"gas_mask", gfx::TileId::gas_mask},
        {"gas_spore", gfx::TileId::gas_spore},
        {"gate_closed", gfx::TileId::gate_closed},
        {"gate_open", gfx::TileId::gate_open},
        {"ghost", gfx::TileId::ghost},
        {"wraith", gfx::TileId::wraith},
        {"ghoul", gfx::TileId::ghoul},
        {"giant_spider", gfx::TileId::giant_spider},
        {"gore1", gfx::TileId::gore1},
        {"gore2", gfx::TileId::gore2},
        {"gore3", gfx::TileId::gore3},
        {"gore4", gfx::TileId::gore4},
        {"gore5", gfx::TileId::gore5},
        {"gore6", gfx::TileId::gore6},
        {"gore7", gfx::TileId::gore7},
        {"gore8", gfx::TileId::gore8},
        {"grate", gfx::TileId::grate},
        {"grave_stone", gfx::TileId::grave_stone},
        {"hammer", gfx::TileId::hammer},
        {"hangbridge_hor", gfx::TileId::hangbridge_hor},
        {"hangbridge_ver", gfx::TileId::hangbridge_ver},
        {"heart", gfx::TileId::heart},
        {"horn", gfx::TileId::horn},
        {"hound", gfx::TileId::hound},
        {"hunting_horror", gfx::TileId::hunting_horror},
        {"incinerator", gfx::TileId::incinerator},
        {"iron_spike", gfx::TileId::iron_spike},
        {"khaga", gfx::TileId::khaga},
        {"lantern", gfx::TileId::lantern},
        {"leech", gfx::TileId::leech},
        {"leng_elder", gfx::TileId::leng_elder},
        {"lever_left", gfx::TileId::lever_left},
        {"lever_right", gfx::TileId::lever_right},
        {"lockpick", gfx::TileId::lockpick},
        {"locust", gfx::TileId::locust},
        {"machete", gfx::TileId::machete},
        {"mantis", gfx::TileId::mantis},
        {"mass_of_worms", gfx::TileId::mass_of_worms},
        {"medical_bag", gfx::TileId::medical_bag},
        {"mi_go", gfx::TileId::mi_go},
        {"mi_go_armor", gfx::TileId::mi_go_armor},
        {"mi_go_gun", gfx::TileId::mi_go_gun},
        {"molotov", gfx::TileId::molotov},
        {"monolith", gfx::TileId::monolith},
        {"mummy", gfx::TileId::mummy},
        {"ooze", gfx::TileId::ooze},
        {"orb", gfx::TileId::orb},
        {"phantasm", gfx::TileId::phantasm},
        {"pharaoh_staff", gfx::TileId::pharaoh_staff},
        {"pillar", gfx::TileId::pillar},
        {"pillar_broken", gfx::TileId::pillar_broken},
        {"pillar_carved", gfx::TileId::pillar_carved},
        {"pistol", gfx::TileId::pistol},
        {"revolver", gfx::TileId::revolver},
        {"rifle", gfx::TileId::rifle},
        {"pit", gfx::TileId::pit},
        {"pitchfork", gfx::TileId::pitchfork},
        {"player_firearm", gfx::TileId::player_firearm},
        {"player_melee", gfx::TileId::player_melee},
        {"polyp", gfx::TileId::polyp},
        {"potion", gfx::TileId::potion},
        {"projectile_std_back_slash", gfx::TileId::projectile_std_back_slash},
        {"projectile_std_dash", gfx::TileId::projectile_std_dash},
        {"projectile_std_front_slash", gfx::TileId::projectile_std_front_slash},
        {"projectile_std_vertical_bar", gfx::TileId::projectile_std_vertical_bar},
        {"pylon", gfx::TileId::pylon},
        {"rat", gfx::TileId::rat},
        {"rat_thing", gfx::TileId::rat_thing},
        {"raven", gfx::TileId::raven},
        {"ring", gfx::TileId::ring},
        {"rock", gfx::TileId::rock},
        {"rod", gfx::TileId::rod},
        {"rubble_high", gfx::TileId::rubble_high},
        {"rubble_low", gfx::TileId::rubble_low},
        {"sarcophagus", gfx::TileId::sarcophagus},
        {"scorched_ground", gfx::TileId::scorched_ground},
        {"scroll", gfx::TileId::scroll},
        {"shadow", gfx::TileId::shadow},
        {"shotgun", gfx::TileId::shotgun},
        {"sledge_hammer", gfx::TileId::sledge_hammer},
        {"smoke", gfx::TileId::smoke},
        {"snake", gfx::TileId::snake},
        {"spider", gfx::TileId::spider},
        {"spiked_mace", gfx::TileId::spiked_mace},
        {"spirit", gfx::TileId::spirit},
        {"square_checkered", gfx::TileId::square_checkered},
        {"square_checkered_sparse", gfx::TileId::square_checkered_sparse},
        {"stairs_down", gfx::TileId::stairs_down},
        {"stalagmite", gfx::TileId::stalagmite},
        {"stopwatch", gfx::TileId::stopwatch},
        {"tentacles", gfx::TileId::tentacles},
        {"the_high_priest", gfx::TileId::the_high_priest},
        {"tomb_closed", gfx::TileId::tomb_closed},
        {"tomb_open", gfx::TileId::tomb_open},
        {"tommy_gun", gfx::TileId::tommy_gun},
        {"trap_general", gfx::TileId::trap_general},
        {"trapez", gfx::TileId::trapez},
        {"tree", gfx::TileId::tree},
        {"tree_fungi", gfx::TileId::tree_fungi},
        {"vines", gfx::TileId::vines},
        {"void_traveler", gfx::TileId::void_traveler},
        {"vortex", gfx::TileId::vortex},
        {"wall_front", gfx::TileId::wall_front},
        {"wall_front_alt1", gfx::TileId::wall_front_alt1},
        {"wall_front_alt2", gfx::TileId::wall_front_alt2},
        {"wall_top", gfx::TileId::wall_top},
        {"water", gfx::TileId::water},
        {"web", gfx::TileId::web},
        {"weight", gfx::TileId::weight},
        {"witch_or_warlock", gfx::TileId::witch_or_warlock},
        {"wolf", gfx::TileId::wolf},
        {"zombie_armed", gfx::TileId::zombie_armed},
        {"zombie_bloated", gfx::TileId::zombie_bloated},
        {"zombie_dust", gfx::TileId::zombie_dust},
        {"zombie_unarmed", gfx::TileId::zombie_unarmed},
        {"", gfx::TileId::END}};

typedef std::unordered_map<gfx::TileId, std::string> TileIdToStrMap;

static const TileIdToStrMap s_tile_id_to_str_map = {
        {gfx::TileId::aim_marker_head, "aim_marker_head"},
        {gfx::TileId::aim_marker_line, "aim_marker_line"},
        {gfx::TileId::alchemist_bench_empty, "alchemist_bench_empty"},
        {gfx::TileId::alchemist_bench_full, "alchemist_bench_full"},
        {gfx::TileId::altar, "altar"},
        {gfx::TileId::gong, "gong"},
        {gfx::TileId::ammo, "ammo"},
        {gfx::TileId::amoeba, "amoeba"},
        {gfx::TileId::amulet, "amulet"},
        {gfx::TileId::ape, "ape"},
        {gfx::TileId::armor, "armor"},
        {gfx::TileId::axe, "axe"},
        {gfx::TileId::barrel, "barrel"},
        {gfx::TileId::bat, "bat"},
        {gfx::TileId::blast1, "blast1"},
        {gfx::TileId::blast2, "blast2"},
        {gfx::TileId::bog_tcher, "bog_tcher"},
        {gfx::TileId::bookshelf_empty, "bookshelf_empty"},
        {gfx::TileId::bookshelf_full, "bookshelf_full"},
        {gfx::TileId::brain, "brain"},
        {gfx::TileId::brazier, "brazier"},
        {gfx::TileId::bush, "bush"},
        {gfx::TileId::byakhee, "byakhee"},
        {gfx::TileId::cabinet_closed, "cabinet_closed"},
        {gfx::TileId::cabinet_open, "cabinet_open"},
        {gfx::TileId::cave_wall_front, "cave_wall_front"},
        {gfx::TileId::cave_wall_top, "cave_wall_top"},
        {gfx::TileId::chains, "chains"},
        {gfx::TileId::chest_closed, "chest_closed"},
        {gfx::TileId::chest_open, "chest_open"},
        {gfx::TileId::chthonian, "chthonian"},
        {gfx::TileId::church_bench, "church_bench"},
        {gfx::TileId::clockwork, "clockwork"},
        {gfx::TileId::club, "club"},
        {gfx::TileId::cocoon_closed, "cocoon_closed"},
        {gfx::TileId::cocoon_open, "cocoon_open"},
        {gfx::TileId::corpse, "corpse"},
        {gfx::TileId::corpse2, "corpse2"},
        {gfx::TileId::crawling_hand, "crawling_hand"},
        {gfx::TileId::crawling_intestines, "crawling_intestines"},
        {gfx::TileId::croc_head_mummy, "croc_head_mummy"},
        {gfx::TileId::crowbar, "crowbar"},
        {gfx::TileId::crystal, "crystal"},
        {gfx::TileId::cultist_dagger, "cultist_dagger"},
        {gfx::TileId::cultist_firearm, "cultist_firearm"},
        {gfx::TileId::cultist_spiked_mace, "cultist_spiked_mace"},
        {gfx::TileId::dagger, "dagger"},
        {gfx::TileId::deep_one, "deep_one"},
        {gfx::TileId::device1, "device1"},
        {gfx::TileId::device2, "device2"},
        {gfx::TileId::door_broken, "door_broken"},
        {gfx::TileId::door_closed, "door_closed"},
        {gfx::TileId::door_open, "door_open"},
        {gfx::TileId::dynamite, "dynamite"},
        {gfx::TileId::dynamite_lit, "dynamite_lit"},
        {gfx::TileId::egypt_wall_front, "egypt_wall_front"},
        {gfx::TileId::egypt_wall_top, "egypt_wall_top"},
        {gfx::TileId::elder_sign, "elder_sign"},
        {gfx::TileId::excl_mark, "excl_mark"},
        {gfx::TileId::fiend, "fiend"},
        {gfx::TileId::flare, "flare"},
        {gfx::TileId::flare_gun, "flare_gun"},
        {gfx::TileId::flare_lit, "flare_lit"},
        {gfx::TileId::floating_skull, "floating_skull"},
        {gfx::TileId::floor, "floor"},
        {gfx::TileId::fountain, "fountain"},
        {gfx::TileId::fungi, "fungi"},
        {gfx::TileId::gas_mask, "gas_mask"},
        {gfx::TileId::gas_spore, "gas_spore"},
        {gfx::TileId::gate_closed, "gate_closed"},
        {gfx::TileId::gate_open, "gate_open"},
        {gfx::TileId::ghost, "ghost"},
        {gfx::TileId::ghoul, "ghoul"},
        {gfx::TileId::giant_spider, "giant_spider"},
        {gfx::TileId::gore1, "gore1"},
        {gfx::TileId::gore2, "gore2"},
        {gfx::TileId::gore3, "gore3"},
        {gfx::TileId::gore4, "gore4"},
        {gfx::TileId::gore5, "gore5"},
        {gfx::TileId::gore6, "gore6"},
        {gfx::TileId::gore7, "gore7"},
        {gfx::TileId::gore8, "gore8"},
        {gfx::TileId::grate, "grate"},
        {gfx::TileId::grave_stone, "grave_stone"},
        {gfx::TileId::hammer, "hammer"},
        {gfx::TileId::hangbridge_hor, "hangbridge_hor"},
        {gfx::TileId::hangbridge_ver, "hangbridge_ver"},
        {gfx::TileId::heart, "heart"},
        {gfx::TileId::horn, "horn"},
        {gfx::TileId::hound, "hound"},
        {gfx::TileId::hunting_horror, "hunting_horror"},
        {gfx::TileId::incinerator, "incinerator"},
        {gfx::TileId::iron_spike, "iron_spike"},
        {gfx::TileId::khaga, "khaga"},
        {gfx::TileId::lantern, "lantern"},
        {gfx::TileId::leech, "leech"},
        {gfx::TileId::leng_elder, "leng_elder"},
        {gfx::TileId::lever_left, "lever_left"},
        {gfx::TileId::lever_right, "lever_right"},
        {gfx::TileId::lockpick, "lockpick"},
        {gfx::TileId::locust, "locust"},
        {gfx::TileId::machete, "machete"},
        {gfx::TileId::mantis, "mantis"},
        {gfx::TileId::mass_of_worms, "mass_of_worms"},
        {gfx::TileId::medical_bag, "medical_bag"},
        {gfx::TileId::mi_go, "mi_go"},
        {gfx::TileId::mi_go_armor, "mi_go_armor"},
        {gfx::TileId::mi_go_gun, "mi_go_gun"},
        {gfx::TileId::molotov, "molotov"},
        {gfx::TileId::monolith, "monolith"},
        {gfx::TileId::mummy, "mummy"},
        {gfx::TileId::ooze, "ooze"},
        {gfx::TileId::orb, "orb"},
        {gfx::TileId::phantasm, "phantasm"},
        {gfx::TileId::pharaoh_staff, "pharaoh_staff"},
        {gfx::TileId::pillar, "pillar"},
        {gfx::TileId::pillar_broken, "pillar_broken"},
        {gfx::TileId::pillar_carved, "pillar_carved"},
        {gfx::TileId::pistol, "pistol"},
        {gfx::TileId::revolver, "revolver"},
        {gfx::TileId::rifle, "rifle"},
        {gfx::TileId::pit, "pit"},
        {gfx::TileId::pitchfork, "pitchfork"},
        {gfx::TileId::player_firearm, "player_firearm"},
        {gfx::TileId::player_melee, "player_melee"},
        {gfx::TileId::polyp, "polyp"},
        {gfx::TileId::potion, "potion"},
        {gfx::TileId::projectile_std_back_slash, "projectile_std_back_slash"},
        {gfx::TileId::projectile_std_dash, "projectile_std_dash"},
        {gfx::TileId::projectile_std_front_slash, "projectile_std_front_slash"},
        {gfx::TileId::projectile_std_vertical_bar, "projectile_std_vertical_bar"},
        {gfx::TileId::pylon, "pylon"},
        {gfx::TileId::rat, "rat"},
        {gfx::TileId::rat_thing, "rat_thing"},
        {gfx::TileId::raven, "raven"},
        {gfx::TileId::ring, "ring"},
        {gfx::TileId::rock, "rock"},
        {gfx::TileId::rod, "rod"},
        {gfx::TileId::rubble_high, "rubble_high"},
        {gfx::TileId::rubble_low, "rubble_low"},
        {gfx::TileId::sarcophagus, "sarcophagus"},
        {gfx::TileId::scorched_ground, "scorched_ground"},
        {gfx::TileId::scroll, "scroll"},
        {gfx::TileId::shadow, "shadow"},
        {gfx::TileId::shotgun, "shotgun"},
        {gfx::TileId::sledge_hammer, "sledge_hammer"},
        {gfx::TileId::smoke, "smoke"},
        {gfx::TileId::snake, "snake"},
        {gfx::TileId::spider, "spider"},
        {gfx::TileId::spiked_mace, "spiked_mace"},
        {gfx::TileId::spirit, "spirit"},
        {gfx::TileId::square_checkered, "square_checkered"},
        {gfx::TileId::square_checkered_sparse, "square_checkered_sparse"},
        {gfx::TileId::stairs_down, "stairs_down"},
        {gfx::TileId::stalagmite, "stalagmite"},
        {gfx::TileId::stopwatch, "stopwatch"},
        {gfx::TileId::tentacles, "tentacles"},
        {gfx::TileId::the_high_priest, "the_high_priest"},
        {gfx::TileId::tomb_closed, "tomb_closed"},
        {gfx::TileId::tomb_open, "tomb_open"},
        {gfx::TileId::tommy_gun, "tommy_gun"},
        {gfx::TileId::trap_general, "trap_general"},
        {gfx::TileId::trapez, "trapez"},
        {gfx::TileId::tree, "tree"},
        {gfx::TileId::tree_fungi, "tree_fungi"},
        {gfx::TileId::vines, "vines"},
        {gfx::TileId::void_traveler, "void_traveler"},
        {gfx::TileId::vortex, "vortex"},
        {gfx::TileId::wall_front, "wall_front"},
        {gfx::TileId::wall_front_alt1, "wall_front_alt1"},
        {gfx::TileId::wall_front_alt2, "wall_front_alt2"},
        {gfx::TileId::wall_top, "wall_top"},
        {gfx::TileId::water, "water"},
        {gfx::TileId::web, "web"},
        {gfx::TileId::weight, "weight"},
        {gfx::TileId::witch_or_warlock, "witch_or_warlock"},
        {gfx::TileId::wolf, "wolf"},
        {gfx::TileId::wraith, "wraith"},
        {gfx::TileId::zombie_armed, "zombie_armed"},
        {gfx::TileId::zombie_bloated, "zombie_bloated"},
        {gfx::TileId::zombie_dust, "zombie_dust"},
        {gfx::TileId::zombie_unarmed, "zombie_unarmed"},
        {gfx::TileId::END, ""}};

// -----------------------------------------------------------------------------
// gfx
// -----------------------------------------------------------------------------
namespace gfx {

P character_pos(const char character)
{
        switch (character) {
        case ' ':
                return P(0, 0);
        case '!':
                return P(1, 0);
        case '"':
                return P(2, 0);
        case '#':
                return P(3, 0);
        case '%':
                return P(4, 0);
        case '&':
                return P(5, 0);
        case 39:
                return P(6, 0);
        case '(':
                return P(7, 0);
        case ')':
                return P(8, 0);
        case '*':
                return P(9, 0);
        case '+':
                return P(10, 0);
        case ',':
                return P(11, 0);
        case '-':
                return P(12, 0);
        case '.':
                return P(13, 0);
        case '/':
                return P(14, 0);
        case '0':
                return P(15, 0);
        case '1':
                return P(0, 1);
        case '2':
                return P(1, 1);
        case '3':
                return P(2, 1);
        case '4':
                return P(3, 1);
        case '5':
                return P(4, 1);
        case '6':
                return P(5, 1);
        case '7':
                return P(6, 1);
        case '8':
                return P(7, 1);
        case '9':
                return P(8, 1);
        case ':':
                return P(9, 1);
        case ';':
                return P(10, 1);
        case '<':
                return P(11, 1);
        case '=':
                return P(12, 1);
        case '>':
                return P(13, 1);
        case '?':
                return P(14, 1);
        case '@':
                return P(15, 1);
        case 'A':
                return P(0, 2);
        case 'B':
                return P(1, 2);
        case 'C':
                return P(2, 2);
        case 'D':
                return P(3, 2);
        case 'E':
                return P(4, 2);
        case 'F':
                return P(5, 2);
        case 'G':
                return P(6, 2);
        case 'H':
                return P(7, 2);
        case 'I':
                return P(8, 2);
        case 'J':
                return P(9, 2);
        case 'K':
                return P(10, 2);
        case 'L':
                return P(11, 2);
        case 'M':
                return P(12, 2);
        case 'N':
                return P(13, 2);
        case 'O':
                return P(14, 2);
        case 'P':
                return P(15, 2);
        case 'Q':
                return P(0, 3);
        case 'R':
                return P(1, 3);
        case 'S':
                return P(2, 3);
        case 'T':
                return P(3, 3);
        case 'U':
                return P(4, 3);
        case 'V':
                return P(5, 3);
        case 'W':
                return P(6, 3);
        case 'X':
                return P(7, 3);
        case 'Y':
                return P(8, 3);
        case 'Z':
                return P(9, 3);
        case '[':
                return P(10, 3);
        case 92:
                return P(11, 3);
        case ']':
                return P(12, 3);
        case '^':
                return P(13, 3);
        case '_':
                return P(14, 3);
        case '`':
                return P(15, 3);
        case 'a':
                return P(0, 4);
        case 'b':
                return P(1, 4);
        case 'c':
                return P(2, 4);
        case 'd':
                return P(3, 4);
        case 'e':
                return P(4, 4);
        case 'f':
                return P(5, 4);
        case 'g':
                return P(6, 4);
        case 'h':
                return P(7, 4);
        case 'i':
                return P(8, 4);
        case 'j':
                return P(9, 4);
        case 'k':
                return P(10, 4);
        case 'l':
                return P(11, 4);
        case 'm':
                return P(12, 4);
        case 'n':
                return P(13, 4);
        case 'o':
                return P(14, 4);
        case 'p':
                return P(15, 4);
        case 'q':
                return P(0, 5);
        case 'r':
                return P(1, 5);
        case 's':
                return P(2, 5);
        case 't':
                return P(3, 5);
        case 'u':
                return P(4, 5);
        case 'v':
                return P(5, 5);
        case 'w':
                return P(6, 5);
        case 'x':
                return P(7, 5);
        case 'y':
                return P(8, 5);
        case 'z':
                return P(9, 5);
        case '{':
                return P(10, 5);
        case '|':
                return P(11, 5);
        case '}':
                return P(12, 5);
        case '~':
                return P(13, 5);
        case 1:
                return P(14, 5);
        case 2:
                return P(0, 6);
        case 3:
                return P(1, 6);
        case 4:
                return P(2, 6);
        case 5:
                return P(3, 6);
        case 6:
                return P(4, 6);
        case 7:
                return P(5, 6);
        case 8:
                return P(6, 6);
        case 9:
                return P(7, 6);
        case 10:
                return P(8, 6);

        default:
                return P(-1, -1);
        }
}

TileId str_to_tile_id(const std::string& str)
{
        return s_str_to_tile_id_map.at(str);
}

std::string tile_id_to_str(TileId id)
{
        return s_tile_id_to_str_map.at(id);
}

} // namespace gfx
