// =============================================================================
// Copyright Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include <filesystem>
#include <fstream>
#include <string>

#include "catch.hpp"
#include "common_text.hpp"
#include "config.hpp"
#include "i18n.hpp"
#include "messages.hpp"
#include "paths.hpp"

TEST_CASE("I18n uses English fallback by default")
{
    config::set_language("en");
    i18n::reload();
    common_text::init();

    REQUIRE(i18n::current_language() == "en");
    REQUIRE(i18n::get("main_menu.options", "(O)Options") == "(O)Options");
    REQUIRE(common_text::g_screen_exit_hint == "[space, esc] to exit");
}

TEST_CASE("I18n loads Chinese UI strings and localized message files")
{
    config::set_language("zh_CN");
    i18n::reload();
    common_text::init();
    messages::init();

    REQUIRE(i18n::current_language() == "zh_CN");
    REQUIRE(i18n::get("main_menu.options", "(O)Options") == "(O)选项");
    REQUIRE(common_text::g_screen_exit_hint == "[space, esc] 退出");
    REQUIRE(i18n::get("create_character.background_title", "What is your background?") == "你的背景是什么？");
    REQUIRE(i18n::get("highscore.browsing_title", "Browsing high scores") == "浏览高分记录");
    REQUIRE(i18n::get("manual.browsing_title", "Browsing manual") == "浏览手册");
    REQUIRE(i18n::get("character_descr.title", "Character description") == "角色描述");
    REQUIRE(i18n::get("game_over_summary.title", "Game summary") == "游戏总结");
    REQUIRE(i18n::get("inventory.slot.weapon", "Weapon") == "武器");
    REQUIRE(i18n::get("inventory.browsing_title", "Browsing inventory") == "浏览物品栏");
    REQUIRE(i18n::get("inventory.throw.title", "Throw which item?") == "投掷哪件物品？");
    REQUIRE(i18n::get("inventory.not_while_burning", "Not while burning.") == "燃烧时不能这么做。");
    REQUIRE(i18n::get("player_spells.known_title", "Known spells") == "已知法术");
    REQUIRE(i18n::get("player_spells.skill_label", "Skill: ") == "技能：");
    REQUIRE(
        i18n::get(
            "terrain_event.wall_collapse_visible",
            "Suddenly, the walls collapse!") == "突然，墙壁崩塌了！");
    REQUIRE(
        i18n::get(
            "terrain_event.rats_discovery_title",
            "A gruesome discovery...") == "可怖的发现……");
    REQUIRE(i18n::get("map.item_legend", "Item") == "物品");
    REQUIRE(i18n::get("item_head.turns_left_suffix", " turns)") == "回合）");
    REQUIRE(i18n::get("insanity.babbling.char_descr", "Babbling") == "胡言乱语");
    REQUIRE(
        i18n::get(
            "insanity.phobia_rat.history",
            "Gained a phobia of rats") == "患上鼠类恐惧症");
    REQUIRE(i18n::get("option.skip_intro_level.name", "Skip intro level") == "跳过开场关卡");
    REQUIRE(i18n::get("option.display_hints.once", "Once") == "一次");
    REQUIRE(i18n::get("option.auto_reload_weapons.descr", "Automatically perform a reload action instead if attempting to fire a ranged weapon with no ammo loaded.") == "如果试图在没有装填弹药的情况下开火，则自动执行装填动作。");
    REQUIRE(i18n::get("terrain_mob.cough", "I cough.") == "我咳嗽起来。");
    REQUIRE(i18n::get("knockback.player_knocked_back", "I am knocked back!") == "我被击退了！");
    REQUIRE(i18n::get("game_commands.press_help", "Press [?] for help.") == "按 [?] 查看帮助。");
    REQUIRE(i18n::get("msg_log.no_message_history", "No message history") == "没有消息历史");
    REQUIRE(i18n::get("item_misc.trapezohedron.beheld_history", "Beheld The Shining Trapezohedron") == "目睹闪耀的偏方三八面体");
    REQUIRE(i18n::get("item_misc.info.open_paren", "(") == "（");
    REQUIRE(i18n::get("item_misc.info.close_paren", ")") == "）");
    REQUIRE(i18n::get("item_misc.info.turns_suffix", " turns") == "回合");
    REQUIRE(i18n::get("item_misc.info.uses_suffix", " uses)") == "次）");
    REQUIRE(i18n::get("item_misc.medical_bag.info_supplies_suffix", " supplies)") == "份）");
    REQUIRE(i18n::get("item_misc.lantern.lit_suffix", ", Lit") == "，亮着");
    REQUIRE(i18n::get("item_misc.lantern.turn_off", "I turn off an Electric Lantern.") == "我关掉了电提灯。");
    REQUIRE(i18n::get("item_misc.lantern.turn_on", "I turn on an Electric Lantern.") == "我打开了电提灯。");
    REQUIRE(i18n::get("item_misc.lantern.expired", "My Electric Lantern has expired.") == "我的电提灯耗尽了。");
    REQUIRE(i18n::get("item_misc.lantern.expired_history", "My Electric Lantern expired") == "我的电提灯耗尽了");
    REQUIRE(i18n::get("item_misc.horn.no_sound", "It makes no sound.") == "它没有发出声音。");
    REQUIRE(i18n::get("item_misc.horn.malice_resounds", "The Horn of Malice resounds!") == "恶意号角响彻四方！");
    REQUIRE(i18n::get("item_misc.horn.banishment_resounds", "The Horn of Banishment resounds!") == "放逐号角响彻四方！");
    REQUIRE(i18n::get("item_misc.holy_symbol.no_faith", "I have no faith that this would help me at the moment.") == "此刻我没有信念相信这会对我有所帮助。");
    REQUIRE(i18n::get("item_misc.holy_symbol.trembling_hands", "With trembling hands ") == "我用颤抖的双手");
    REQUIRE(i18n::get("item_misc.holy_symbol.prayer_prefix", "I make a prayer over the ") == "对着");
    REQUIRE(i18n::get("item_misc.holy_symbol.prayer_suffix", "...") == "做祈祷……");
    REQUIRE(i18n::get("item_misc.holy_symbol.feels_useless", "This feels useless!") == "这感觉毫无用处！");
    REQUIRE(i18n::get("item_misc.holy_symbol.recharged_prefix", "I feel like praying over the ") == "我感觉再次对着");
    REQUIRE(i18n::get("item_misc.holy_symbol.recharged_suffix", " would be beneficent again.") == "祈祷会有益处。");
    REQUIRE(i18n::get("item_misc.holy_symbol.failed_suffix", ", failed") == "，失败");
    REQUIRE(i18n::get("item_misc.clockwork.nothing_happens", "Nothing happens.") == "什么也没发生。");
    REQUIRE(i18n::get("item_misc.clockwork.will_not_move", "It will not move.") == "它不会动。");
    REQUIRE(i18n::get("item_misc.clockwork.wind_up", "I wind up the clockwork.") == "我上紧了发条装置。");
    REQUIRE(i18n::get("item_misc.necronomicon.destroy", "I destroy the profane text!") == "我摧毁了这亵渎的文本！");
    REQUIRE(i18n::get("item_misc.witch_eye.clutch_prefix", "I clutch the ") == "我紧握着");
    REQUIRE(i18n::get("item_misc.witch_eye.clutch_suffix", "...") == "……");
    REQUIRE(i18n::get("item_misc.witch_eye.decomposes", "The eye decomposes.") == "那只眼睛腐烂了。");
    REQUIRE(i18n::get("item_misc.fluctuating_material.stare_prefix", "I stare into the ") == "我凝视着");
    REQUIRE(i18n::get("item_misc.fluctuating_material.stare_suffix", ", and feel myself changing...") == "，感觉自己正在改变……");
    REQUIRE(i18n::get("item_misc.fluctuating_material.gain_trait_title", "Which trait do you gain?") == "你要获得哪项特质？");
    REQUIRE(i18n::get("item_misc.astral_opium.use_prefix", "I use the ") == "我使用了");
    REQUIRE(i18n::get("item_misc.astral_opium.use_suffix", "...") == "……");

    const auto input_mode_descr =
        i18n::get("option.input_mode.descr", "");
    REQUIRE(input_mode_descr.find("\\n") == std::string::npos);
    REQUIRE(input_mode_descr.find("\n\n{COLOR_LIGHT_WHITE}默认：") != std::string::npos);

    const auto path = messages::resolved_path("menu_quotes.txt");

    REQUIRE(path.find("locale/zh_CN/messages/menu_quotes.txt") != std::string::npos);
    REQUIRE(std::filesystem::exists(path));
}

TEST_CASE("I18n falls back to base message file when localized file is missing")
{
    config::set_language("zh_CN");
    i18n::reload();

    const auto unique_rel_path = "messages/test_i18n_fallback_only.txt";
    const auto base_path = paths::data_dir() + "/" + unique_rel_path;

    {
        std::ofstream file(base_path);
        REQUIRE(file.is_open());
        file << "fallback line\n";
    }

    const auto resolved = messages::resolved_path("test_i18n_fallback_only.txt");

    REQUIRE(resolved == base_path);

    std::filesystem::remove(base_path);
}
