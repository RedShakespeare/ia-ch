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
    REQUIRE(
        i18n::get(
            "view_actor_descr.remember_for_at_least",
            " will remember hostile creatures for at least ") == "至少会记住敌对生物");
    REQUIRE(
        i18n::get(
            "view_actor_descr.color_dark_yellow",
            "{COLOR_DARK_YELLOW}") == "{COLOR_DARK_YELLOW}");
    REQUIRE(
        i18n::get(
            "view_actor_descr.turns_suffix",
            "{_}turns{reset_color}.") == "{_}回合{reset_color}。");
    REQUIRE(
        i18n::get(
            "view_actor_descr.remembers_for_a",
            " remembers hostile creatures for a ") == "会记住敌对生物");
    REQUIRE(
        i18n::get(
            "view_actor_descr.very_long_time",
            "{COLOR_DARK_YELLOW}very long time{reset_color}.") == "{COLOR_DARK_YELLOW}非常久{reset_color}。");
    REQUIRE(i18n::get("view_actor_descr.speed_slowly", "slowly") == "缓慢");
    REQUIRE(i18n::get("view_actor_descr.speed_fast", "fast") == "快速");
    REQUIRE(
        i18n::get(
            "view_actor_descr.speed_very_swiftly",
            "very swiftly") == "非常迅速");
    REQUIRE(
        i18n::get(
            "view_actor_descr.appears_to_move_suffix",
            " appears to move{_}") == "看起来移动");
    REQUIRE(i18n::get("view_actor_descr.period", ".") == "。");
    REQUIRE(
        i18n::get(
            "view_actor_descr.they_appear_to_move",
            "They appear to move{_}") == "它们看起来移动");
    REQUIRE(i18n::get("view_actor_descr.shock_unsettling", "unsettling") == "令人不安");
    REQUIRE(i18n::get("view_actor_descr.shock_frightening", "frightening") == "令人恐惧");
    REQUIRE(i18n::get("view_actor_descr.shock_terrifying", "terrifying") == "恐怖");
    REQUIRE(
        i18n::get(
            "view_actor_descr.shock_mind_shattering",
            "mind shattering") == "震碎心智");
    REQUIRE(i18n::get("view_actor_descr.unique_is", " is ") == "看起来");
    REQUIRE(i18n::get("view_actor_descr.they_are", "They are ") == "它们看起来");
    REQUIRE(i18n::get("view_actor_descr.to_behold", " to behold") == "");
    REQUIRE(i18n::get("view_actor_descr.color_reset", "{reset_color}") == "{reset_color}");
    REQUIRE(i18n::get("view_actor_descr.it", "It") == "它");
    REQUIRE(i18n::get("view_actor_descr.is_wielding", " is wielding ") == "正持有");
    REQUIRE(
        i18n::get(
            "view_actor_descr.full_health",
            "They are at full health.") == "它们生命值全满。");
    REQUIRE(i18n::get("view_actor_descr.health_prefix", "They are at ") == "它们生命值为");
    REQUIRE(i18n::get("view_actor_descr.health_suffix", "% health.") == "%。");
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
    REQUIRE(i18n::get("actor_player.monster_here_prefix", "There is ") == "这里有");
    REQUIRE(i18n::get("actor_player.monster_here_suffix", " here!") == "！");
    REQUIRE(i18n::get("actor_player.more_fervent", "I feel more fervent!") == "我感到更加虔诚！");
    REQUIRE(i18n::get("actor_player.continue_taking_off", "Continue taking off ") == "继续脱下");
    REQUIRE(i18n::get("actor_player.continue_putting_on", "Continue putting on ") == "继续穿上");
    REQUIRE(i18n::get("actor_player.continue_equipping", "Continue equipping ") == "继续装备");
    REQUIRE(i18n::get("actor_player.turns_left_open", " (") == "（");
    REQUIRE(i18n::get("actor_player.turns_left_suffix", " turns left)?") == "回合剩余）？");
    REQUIRE(i18n::get("actor_player.query_suffix", "?") == "？");
    REQUIRE(i18n::get("actor_player.some_foul_entity", "some foul entity") == "某个污秽的实体");
    REQUIRE(i18n::get("actor_player.someone", "someone") == "某人");
    REQUIRE(i18n::get("actor_player.a_creature", "a creature") == "某个生物");
    REQUIRE(i18n::get("actor_player.chill_runs_down_spine", "A chill runs down my spine.") == "一阵寒意顺着我的脊背爬过。");
    REQUIRE(i18n::get("actor_player.sense_great_danger", "I sense a great danger.") == "我感觉到巨大的危险。");
    REQUIRE(i18n::get("actor_player.feel_anxious", "I feel anxious.") == "我感到焦虑。");
    REQUIRE(i18n::get("actor_player.insane_message", "My mind can no longer withstand what it has grasped. I am hopelessly lost.") == "我的心智再也无法承受它所领悟的东西。我已经彻底迷失了。");
    REQUIRE(i18n::get("actor_player.insane_title", "Insane!") == "疯狂！");
    REQUIRE(i18n::get("actor_mon.cultist_phrase_apigami", "Apigami!") == "Apigami!");
    REQUIRE(i18n::get("actor_mon.cultist_phrase_bhuudesco_invisuu", "Bhuudesco invisuu!") == "Bhuudesco invisuu!");
    REQUIRE(i18n::get("actor_mon.cultist_phrase_bhuuesco_marana", "Bhuuesco marana!") == "Bhuuesco marana!");
    REQUIRE(i18n::get("actor_mon.cultist_phrase_crudux_cruo", "Crudux cruo!") == "Crudux cruo!");
    REQUIRE(i18n::get("actor_mon.cultist_phrase_cruento_paashaeximus", "Cruento paashaeximus!") == "Cruento paashaeximus!");
    REQUIRE(i18n::get("actor_mon.cultist_phrase_cruento_pestis_shatruex", "Cruento pestis shatruex!") == "Cruento pestis shatruex!");
    REQUIRE(i18n::get("actor_mon.cultist_phrase_cruo_crunatus_durbe", "Cruo crunatus durbe!") == "Cruo crunatus durbe!");
    REQUIRE(i18n::get("actor_mon.cultist_phrase_cruo_lokemundux", "Cruo lokemundux!") == "Cruo lokemundux!");
    REQUIRE(i18n::get("actor_mon.cultist_phrase_cruo_stragara_na", "Cruo stragara-na!") == "Cruo stragara-na!");
    REQUIRE(i18n::get("actor_mon.cultist_phrase_gero_shay_cruo", "Gero shay cruo!") == "Gero shay cruo!");
    REQUIRE(i18n::get("actor_mon.cultist_phrase_in_marana_domus", "In marana domus-bhaava crunatus!") == "In marana domus-bhaava crunatus!");
    REQUIRE(i18n::get("actor_mon.cultist_phrase_caecux_infirmux", "Caecux infirmux!") == "Caecux infirmux!");
    REQUIRE(i18n::get("actor_mon.cultist_phrase_malax_sayti", "Malax sayti!") == "Malax sayti!");
    REQUIRE(i18n::get("actor_mon.cultist_phrase_marana_pallex", "Marana pallex!") == "Marana pallex!");
    REQUIRE(i18n::get("actor_mon.cultist_phrase_marana_malax", "Marana malax!") == "Marana malax!");
    REQUIRE(i18n::get("actor_mon.cultist_phrase_pallex_ti", "Pallex ti!") == "Pallex ti!");
    REQUIRE(i18n::get("actor_mon.cultist_phrase_peroshay_bibox_malax", "Peroshay bibox malax!") == "Peroshay bibox malax!");
    REQUIRE(i18n::get("actor_mon.cultist_phrase_pestis_cruento", "Pestis Cruento!") == "Pestis Cruento!");
    REQUIRE(i18n::get("actor_mon.cultist_phrase_pestis_cruento_vilomaxus", "Pestis cruento vilomaxus pretiacruento!") == "Pestis cruento vilomaxus pretiacruento!");
    REQUIRE(i18n::get("actor_mon.cultist_phrase_pretaanluxis_cruonit", "Pretaanluxis cruonit!") == "Pretaanluxis cruonit!");
    REQUIRE(i18n::get("actor_mon.cultist_phrase_pretiacruento", "Pretiacruento!") == "Pretiacruento!");
    REQUIRE(i18n::get("actor_mon.cultist_phrase_stragar_naya", "Stragar-Naya!") == "Stragar-Naya!");
    REQUIRE(i18n::get("actor_mon.cultist_phrase_vorox_esco_marana", "Vorox esco marana!") == "Vorox esco marana!");
    REQUIRE(i18n::get("actor_mon.cultist_phrase_vilomaxus", "Vilomaxus!") == "Vilomaxus!");
    REQUIRE(i18n::get("actor_mon.cultist_phrase_prostragaranar_malachtose", "Prostragaranar malachtose!") == "Prostragaranar malachtose!");
    REQUIRE(i18n::get("actor_mon.save_us_suffix", " save us!") == "，救救我们！");
    REQUIRE(i18n::get("actor_mon.will_save_us_suffix", " will save us!") == "会拯救我们！");
    REQUIRE(i18n::get("actor_mon.watches_over_us_suffix", " watches over us!") == "注视着我们！");
    REQUIRE(i18n::get("actor_mon.guide_us_suffix", ", guide us!") == "，指引我们！");
    REQUIRE(i18n::get("actor_mon.guides_us_suffix", " guides us!") == "指引着我们！");
    REQUIRE(i18n::get("actor_mon.for_prefix", "For ") == "为了");
    REQUIRE(i18n::get("actor_mon.exclaim", "!") == "！");
    REQUIRE(i18n::get("actor_mon.blood_for_prefix", "Blood for ") == "鲜血献给");
    REQUIRE(i18n::get("actor_mon.perish_for_prefix", "Perish for ") == "为此毁灭吧：");
    REQUIRE(i18n::get("actor_mon.in_the_name_of_prefix", "In the name of ") == "以此之名：");
    REQUIRE(i18n::get("actor_mon.voice_prefix", "Voice: ") == "声音：");
    REQUIRE(i18n::get("actor_mon.sees_me_suffix", " sees me!") == "看见了我！");
    REQUIRE(i18n::get("actor_common.dies", "dies.") == "死了。");
    REQUIRE(i18n::get("actor_death.agonized_screaming", "I hear agonized screaming.") == "我听到痛苦的尖叫。");
    REQUIRE(i18n::get("actor_death.resurrect_history", "Was brought back from the dead") == "死而复生");
    REQUIRE(i18n::get("actor_eat.ripping_and_chewing", "I hear ripping and chewing.") == "我听到撕咬和咀嚼声。");
    REQUIRE(i18n::get("actor_eat.feed_player_prefix", "I feed on ") == "我啃食");
    REQUIRE(i18n::get("actor_eat.period", ".") == "。");
    REQUIRE(i18n::get("actor_eat.feeds_on", " feeds on ") == "啃食");
    REQUIRE(i18n::get("actor_eat.completely_devoured_suffix", " is completely devoured.") == "被完全吞噬了。");
    REQUIRE(i18n::get("actor_hit.armor_torn_apart_prefix", "My ") == "我的");
    REQUIRE(i18n::get("actor_hit.armor_torn_apart_suffix", " is torn apart!") == "被撕裂了！");
    REQUIRE(i18n::get("actor_hit.crack", "*Crack!*") == "*咔嚓！*");
    REQUIRE(i18n::get("actor_hit.destroyed_suffix", " is destroyed.") == "被摧毁了。");
    REQUIRE(i18n::get("actor_hit.thud", "*Thud!*") == "*砰！*");
    REQUIRE(i18n::get("actor_hit.chop", "*Chop!*") == "*劈砍！*");
    REQUIRE(i18n::get("actor_hit.low_hp_warning", "-LOW HP WARNING!-") == "-低生命值警告！-");
    REQUIRE(i18n::get("actor_hit.wracked_by_light", "I am wracked by light!") == "我被光芒折磨！");
    REQUIRE(i18n::get("actor_hit.sustained_severe_wound_history", "Sustained a severe wound") == "遭受重伤");
    REQUIRE(i18n::get("actor_hit.spirit_drained", "My spirit is drained!") == "我的精神被抽干了！");
    REQUIRE(i18n::get("actor_hit.spirit_depleted", "All my spirit is depleted, I am devoid of life!") == "我的精神已耗尽，我失去了生命！");
    REQUIRE(i18n::get("actor_hit.no_spirit_left_suffix", " has no spirit left!") == "没有精神力了！");
    REQUIRE(i18n::get("ai.looks_desperate_suffix", " looks desperate.") == "看起来绝望了。");
    REQUIRE(i18n::get("actor_move.through", "through") == "穿过");
    REQUIRE(i18n::get("actor_move.under", "under") == "从下方");
    REQUIRE(i18n::get("actor_move.it", "it") == "它");
    REQUIRE(i18n::get("actor_move.seeps_prefix", " seeps ") == "渗");
    REQUIRE(i18n::get("actor_move.squirms_through", " squirms through ") == "钻过");
    REQUIRE(i18n::get("explosion.player_hit", "I am hit by an explosion!") == "我被爆炸击中了！");
    REQUIRE(i18n::get("explosion.survived_history", "Survived an explosion") == "从爆炸中幸存");
    REQUIRE(i18n::get("explosion.hear", "I hear an explosion!") == "我听到一声爆炸！");
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
