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
    REQUIRE(i18n::get("player_bon.extra_trait_title", "You gain an extra trait!") == "你获得了额外特质！");
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
    REQUIRE(i18n::get("terrain.hear_crash", "I hear a crash.") == "我听到一声撞击。");
    REQUIRE(i18n::get("terrain.topples_prefix", "The ") == "");
    REQUIRE(i18n::get("terrain.topples_suffix", " topples over.") == "倒下了。");
    REQUIRE(i18n::get("terrain.falls_on_me", "It falls on me!") == "它砸到了我！");
    REQUIRE(i18n::get("terrain.falls_on_prefix", "It falls on ") == "它砸到了");
    REQUIRE(i18n::get("terrain.period", ".") == "。");
    REQUIRE(i18n::get("terrain.church_bench_destroyed", "The church bench is destroyed.") == "教堂长椅被摧毁了。");
    REQUIRE(i18n::get("terrain.wiggles", "It wiggles a bit.") == "它摇晃了一下。");
    REQUIRE(i18n::get("terrain.stairs_down_title", "A staircase leading downwards") == "向下的楼梯");
    REQUIRE(i18n::get("terrain.descend_option", "(D)escend") == "(D)下楼");
    REQUIRE(i18n::get("terrain.save_and_quit_option", "(S)ave and quit") == "(S)保存并退出");
    REQUIRE(i18n::get("terrain.descend_stairs", "I descend the stairs.") == "我走下楼梯。");
    REQUIRE(i18n::get("terrain.seems_cleansed_prefix", "The ") == "");
    REQUIRE(i18n::get("terrain.seems_cleansed_suffix", " seems cleansed!") == "看起来被净化了！");
    REQUIRE(i18n::get("terrain.touch_prefix", "I touch ") == "我触摸了");
    REQUIRE(i18n::get("terrain.touch_crystal_object", "I touch some crystal object.") == "我触摸了某个水晶物体。");
    REQUIRE(i18n::get("terrain.nothing_happens", "Nothing happens.") == "什么也没发生。");
    REQUIRE(i18n::get("terrain.light_inside_fades", "The light inside fades.") == "里面的光芒消退了。");
    REQUIRE(i18n::get("terrain.path_opened", "I sense that a path has opened somewhere.") == "我感到某处有一条道路打开了。");
    REQUIRE(i18n::get("terrain.altar_destroyed", "The altar is destroyed.") == "祭坛被摧毁了。");
    REQUIRE(i18n::get("terrain.topples_over", "It topples over.") == "它倒下了。");
    REQUIRE(i18n::get("terrain.no_more_items_of_interest", "There are no more items of interest.") == "没有更多值得关注的物品。");
    REQUIRE(i18n::get("terrain.unload_prompt", "Unload? [u]") == "卸下？[u]");
    REQUIRE(i18n::get("terrain.muffled_shatter", "I hear a muffled shatter.") == "我听到一声闷闷的碎裂声。");
    REQUIRE(i18n::get("terrain_mirror.article_a", "a ") == "");
    REQUIRE(i18n::get("terrain_mirror.article_the", "the ") == "");
    REQUIRE(i18n::get("terrain_mirror.hazy_mirror", "hazy mirror") == "朦胧之镜");
    REQUIRE(i18n::get("terrain_mirror.destroyed_suffix", " is destroyed.") == "被摧毁了。");
    REQUIRE(i18n::get("terrain_monolith.article_a", "a ") == "");
    REQUIRE(i18n::get("terrain_monolith.article_the", "the ") == "");
    REQUIRE(i18n::get("terrain_monolith.carved_monolith", "carved monolith") == "雕刻石碑");
    REQUIRE(i18n::get("terrain_gong.article_a", "a ") == "");
    REQUIRE(i18n::get("terrain_gong.article_the", "the ") == "");
    REQUIRE(i18n::get("terrain_gong.temple_gong_name", "temple gong") == "神殿铜锣");
    REQUIRE(i18n::get("terrain_gong.temple_gong_legend", "Temple Gong") == "神殿铜锣");
    REQUIRE(i18n::get("terrain.tomb_destroyed", "The tomb is destroyed.") == "坟墓被摧毁了。");
    REQUIRE(i18n::get("terrain.tomb_empty", "The tomb is empty.") == "坟墓是空的。");
    REQUIRE(i18n::get("terrain.stone_box_here", "There is a stone box here.") == "这里有一个石盒。");
    REQUIRE(i18n::get("terrain.attempt_push_lid", "I attempt to push the lid.") == "我试着推开盖子。");
    REQUIRE(i18n::get("terrain.seems_futile", "It seems futile.") == "这似乎徒劳无功。");
    REQUIRE(i18n::get("terrain_door.hear_loud_banging", "I hear a loud banging.") == "我听到响亮的撞击声。");
    REQUIRE(i18n::get("terrain_door.open_prefix", "I open the ") == "我打开了");
    REQUIRE(i18n::get("terrain_door.close_prefix", "I close the ") == "我关上了");
    REQUIRE(i18n::get("terrain_door.period", ".") == "。");
    REQUIRE(i18n::get("terrain_door.hear_door_open", "I hear a door open.") == "我听到一扇门打开。");
    REQUIRE(i18n::get("terrain_door.opens_a", " opens a ") == "打开了一扇");
    REQUIRE(i18n::get("terrain_door.see_a", "I see a ") == "我看见一扇");
    REQUIRE(i18n::get("terrain_door.opening_suffix", " opening.") == "正在打开。");
    REQUIRE(i18n::get("terrain_door.fumble_with_a", "I fumble with a ") == "我摸索着一扇");
    REQUIRE(i18n::get("terrain_door.manage_close_suffix", ", but manage to close it.") == "，但设法关上了。");
    REQUIRE(i18n::get("terrain_door.manage_open_suffix", ", but manage to open it.") == "，但设法打开了。");
    REQUIRE(
        i18n::get(
            "terrain_door.hear_door_open_awkwardly",
            "I hear something open a door awkwardly.") == "我听到有什么东西笨拙地打开一扇门。");
    REQUIRE(
        i18n::get(
            "terrain_door.fumbles_manages_open_a",
            "fumbles, but manages to open a ") == "摸索着，但设法打开了一扇");
    REQUIRE(i18n::get("terrain_door.open_awkwardly_suffix", " open awkwardly.") == "笨拙地打开。");
    REQUIRE(
        i18n::get(
            "terrain_door.fumble_blindly_open_prefix",
            "I fumble blindly with a ") == "我盲目地摸索着一扇");
    REQUIRE(i18n::get("terrain_door.fail_open_suffix", ", and fail to open it.") == "，但没能打开。");
    REQUIRE(
        i18n::get(
            "terrain_door.hear_attempt_open_door",
            "I hear something attempting to open a door.") == "我听到有什么东西试图打开一扇门。");
    REQUIRE(i18n::get("terrain_door.futile", "It seems futile.") == "这样做似乎毫无用处。");
    REQUIRE(i18n::get("terrain_door.door_burns_down", "The door burns down.") == "门烧毁了。");
    REQUIRE(
        i18n::get(
            "terrain_door.foreboding_feeling",
            "Something gives me a foreboding feeling.") == "有什么东西给了我一种不祥的感觉。");
    REQUIRE(i18n::get("terrain_door.there_is", "There is ") == "这里有");
    REQUIRE(i18n::get("terrain_door.here", " here.") == "在这里。");
    REQUIRE(i18n::get("terrain_door.secret_revealed", "A secret is revealed.") == "一个秘密被揭示了。");
    REQUIRE(i18n::get("terrain_door.seems_stuck_suffix", " seems to be stuck.") == "似乎被卡住了。");
    REQUIRE(i18n::get("terrain_door.a", "a ") == "一扇");
    REQUIRE(i18n::get("terrain_door.the_lowercase", "the ") == "那扇");
    REQUIRE(i18n::get("terrain_door.jam_prefix", "I jam ") == "我用尖刺卡住了");
    REQUIRE(i18n::get("terrain_door.with_a_spike", " with a spike.") == "。");
    REQUIRE(i18n::get("terrain_door.see_nothing_to_close", "I see nothing there to close.") == "我没看到那里有什么可关闭的东西。");
    REQUIRE(i18n::get("terrain_door.find_nothing_to_close", "I find nothing there to close.") == "我没找到任何可关闭的东西。");
    REQUIRE(i18n::get("terrain_door.opens_suffix", " opens.") == "打开了。");
    REQUIRE(i18n::get("terrain_door.closes_suffix", " closes.") == "关上了。");
    REQUIRE(i18n::get("terrain_door.something_approaches", "Something approaches...") == "有什么东西接近了...");
    REQUIRE(i18n::get("terrain_trap.the_floor", "the floor") == "地板");
    REQUIRE(i18n::get("terrain_trap.exclaim", "!") == "！");
    REQUIRE(i18n::get("terrain_trap.it", "it") == "它");
    REQUIRE(i18n::get("terrain_trap.hear_otherworldly_blaze", "I hear an otherworldly blaze.") == "我听到一阵异界的焰声。");
    REQUIRE(i18n::get("terrain_trap.dart_launched_from", "A dart is launched from ") == "一支飞镖从");
    REQUIRE(i18n::get("terrain_trap.spear_shoots_from", "A spear shoots out from ") == "一支长矛从");
    REQUIRE(i18n::get("terrain_trap.intense_flash", "There is an intense flash of light!") == "有一道强烈的闪光！");
    REQUIRE(i18n::get("terrain_trap.smoke_released", "A burst of smoke is released from a vent in the floor!") == "一股烟雾从地板上的通风口喷出！");
    REQUIRE(i18n::get("terrain_trap.hear_burst_gas", "I hear a burst of gas.") == "我听到一阵气体喷出的声音。");
    REQUIRE(i18n::get("terrain_trap.alarm_sounds", "An alarm sounds!") == "警报响起！");
    REQUIRE(i18n::get("terrain_trap.entangled_suffix", " is entangled in a huge spider web!") == "被一张巨大的蜘蛛网缠住了！");
    REQUIRE(i18n::get("terrain_trap.appears_suffix", " appears!") == "出现了！");
    REQUIRE(i18n::get("terrain_trap.surroundings_change", "The surroundings change!") == "周围的环境改变了！");
    REQUIRE(i18n::get("terrain_trap.no_apparent_effect", "There is no apparent effect.") == "没有明显效果。");
    REQUIRE(i18n::get("terrain_trap.misty_haze_self", "I am surrounded by a misty haze.") == "我被一阵雾霭包围。");
    REQUIRE(i18n::get("terrain_trap.misty_haze_surrounds", "A misty haze surrounds ") == "一阵雾霭笼罩了");
    REQUIRE(i18n::get("terrain_trap.step_into", "Step into ") == "踏入");
    REQUIRE(i18n::get("terrain_trap.query_suffix", "?") == "？");
    REQUIRE(i18n::get("terrain_trap.there_is", "There is ") == "这里有");
    REQUIRE(i18n::get("terrain_trap.here_exclaim", " here!") == "在这里！");
    REQUIRE(i18n::get("terrain_trap.spot_prefix", "I spot ") == "我发现了");
    REQUIRE(i18n::get("terrain_trap.disarm", "I disarm a trap.") == "我拆除一个陷阱。");
    REQUIRE(i18n::get("terrain_trap.crushing_pressure", "There is suddenly a crushing pressure in the air!") == "空气中突然出现一股压迫力！");
    REQUIRE(i18n::get("terrain_trap.cut_free_machete", "I cut myself free with my Machete.") == "我用砍刀割断束缚。");
    REQUIRE(i18n::get("terrain_trap.entangled_web", "I am entangled in a spider web!") == "我被一张蜘蛛网缠住了！");
    REQUIRE(i18n::get("terrain_trap.entangled_threads", "I am entangled in a sticky mass of threads!") == "我被一团黏稠的丝线缠住了！");
    REQUIRE(i18n::get("terrain_trap.tear_down_web", "I tear down a spider web.") == "我拆掉一张蜘蛛网。");
    REQUIRE(i18n::get("terrain_trap.article_a", "a") == "");
    REQUIRE(i18n::get("terrain_trap.article_an", "an") == "");
    REQUIRE(i18n::get("terrain_trap.article_the", "the") == "");
    REQUIRE(i18n::get("terrain_trap.sigil_name", " Sigil") == "印记");
    REQUIRE(i18n::get("terrain_trap.dart_trap_name", " dart trap") == "飞镖陷阱");
    REQUIRE(i18n::get("terrain_trap.spear_trap_name", " spear trap") == "长矛陷阱");
    REQUIRE(i18n::get("terrain_trap.blinding_trap_name", " blinding trap") == "致盲陷阱");
    REQUIRE(i18n::get("terrain_trap.deafening_trap_name", " deafening trap") == "震聋陷阱");
    REQUIRE(i18n::get("terrain_trap.smoke_trap_name", " smoke trap") == "烟雾陷阱");
    REQUIRE(i18n::get("terrain_trap.alarm_trap_name", " alarm trap") == "警报陷阱");
    REQUIRE(i18n::get("terrain_trap.spider_web_name", " spider web") == "蜘蛛网");
    REQUIRE(i18n::get("terrain_trap.boundary_sigil_name", " Boundary Sigil") == "边界印记");
    REQUIRE(i18n::get("terrain.does_not_yield", "It does not yield at all.") == "它完全没有松动。");
    REQUIRE(i18n::get("terrain.resists", "It resists.") == "它抵住了。");
    REQUIRE(i18n::get("terrain.moves_little", "It moves a little!") == "它移动了一点！");
    REQUIRE(i18n::get("terrain.peer_inside_tomb", "I peer inside the tomb.") == "我向坟墓里窥视。");
    REQUIRE(i18n::get("terrain.nothing_of_value_inside", "There is nothing of value inside.") == "里面没有值钱的东西。");
    REQUIRE(i18n::get("terrain.heavy_stone_sliding", "I hear heavy stone sliding.") == "我听到沉重的石头滑动声。");
    REQUIRE(i18n::get("terrain.lid_comes_off", "The lid comes off.") == "盖子脱落了。");
    REQUIRE(i18n::get("terrain.something_inside", "There is something inside.") == "里面有东西。");
    REQUIRE(i18n::get("terrain.gas_burst", "I hear a burst of gas.") == "我听到一阵气体喷出声。");
    REQUIRE(i18n::get("terrain.chest_here", "There is a chest here.") == "这里有一个箱子。");
    REQUIRE(i18n::get("terrain.chest_on_fire", "The chest is on fire.") == "箱子着火了。");
    REQUIRE(i18n::get("terrain.chest_empty", "The chest is empty.") == "箱子是空的。");
    REQUIRE(i18n::get("terrain.chest_locked", "The chest is locked.") == "箱子锁上了。");
    REQUIRE(i18n::get("terrain.search_chest", "I search the chest.") == "我搜索箱子。");
    REQUIRE(i18n::get("terrain.chest_opens", "The chest opens.") == "箱子打开了。");
    REQUIRE(i18n::get("terrain.already_open", "It is already open.") == "它已经打开了。");
    REQUIRE(i18n::get("terrain.lid_slams_open_falls_shut", "The lid slams open, then falls shut.") == "盖子猛地打开，然后又落下关上。");
    REQUIRE(i18n::get("terrain.lock_breaks_lid_flies_open", "The lock breaks and the lid flies open!") == "锁断开，盖子飞开了！");
    REQUIRE(i18n::get("terrain.lock_resists", "The lock resists.") == "锁抵住了。");
    REQUIRE(i18n::get("terrain.fountain_destroyed", "The fountain is destroyed.") == "喷泉被摧毁了。");
    REQUIRE(i18n::get("terrain.fountain_dried_up", "The fountain is dried-up.") == "喷泉已经干涸。");
    REQUIRE(i18n::get("terrain.drink_from_fountain", "I drink from the fountain...") == "我从喷泉中饮水……");
    REQUIRE(i18n::get("terrain.very_refreshing", "It's very refreshing.") == "非常清爽。");
    REQUIRE(i18n::get("terrain.feel_more_powerful", "I feel more powerful!") == "我感觉更强大了！");
    REQUIRE(i18n::get("terrain.fountain_dries_up", "The fountain dries up.") == "喷泉干涸了。");
    REQUIRE(i18n::get("terrain.cabinet_destroyed", "The cabinet is destroyed.") == "柜子被摧毁了。");
    REQUIRE(i18n::get("terrain.cabinet_burns_down", "The cabinet burns down.") == "柜子烧毁了。");
    REQUIRE(i18n::get("terrain.cabinet_here", "There is a cabinet here.") == "这里有一个柜子。");
    REQUIRE(i18n::get("terrain.cabinet_on_fire", "The cabinet is on fire.") == "柜子着火了。");
    REQUIRE(i18n::get("terrain.cabinet_empty", "The cabinet is empty.") == "柜子是空的。");
    REQUIRE(i18n::get("terrain.search_cabinet", "I search the cabinet.") == "我搜索柜子。");
    REQUIRE(i18n::get("terrain.cabinet_opens", "The cabinet opens.") == "柜子打开了。");
    REQUIRE(i18n::get("terrain.bookshelf_destroyed", "The bookshelf is destroyed.") == "书架被摧毁了。");
    REQUIRE(i18n::get("terrain.bookshelf_burns_down", "The bookshelf burns down.") == "书架烧毁了。");
    REQUIRE(i18n::get("terrain.bookshelf_here", "There is a bookshelf here.") == "这里有一个书架。");
    REQUIRE(i18n::get("terrain.bookshelf_on_fire", "The bookshelf is on fire.") == "书架着火了。");
    REQUIRE(i18n::get("terrain.bookshelf_empty", "The bookshelf is empty.") == "书架是空的。");
    REQUIRE(i18n::get("terrain.nothing_of_interest", "There is nothing of interest.") == "没有值得关注的东西。");
    REQUIRE(i18n::get("terrain.alchemist_workbench_destroyed", "The alchemist's workbench is destroyed.") == "炼金术士的工作台被摧毁了。");
    REQUIRE(i18n::get("terrain.alchemist_workbench_burns_down", "The alchemist's workbench burns down.") == "炼金术士的工作台烧毁了。");
    REQUIRE(i18n::get("terrain.alchemist_workbench_here", "There is an alchemist's workbench here.") == "这里有一个炼金术士的工作台。");
    REQUIRE(i18n::get("terrain.alchemist_workbench_on_fire", "The alchemist's workbench is on fire.") == "炼金术士的工作台着火了。");
    REQUIRE(i18n::get("terrain.alchemist_workbench_empty", "The alchemist's workbench is empty.") == "炼金术士的工作台是空的。");
    REQUIRE(i18n::get("terrain.cocoon_destroyed", "The cocoon is destroyed.") == "茧被摧毁了。");
    REQUIRE(i18n::get("terrain.cocoon_burns_down", "The cocoon burns down.") == "茧烧毁了。");
    REQUIRE(i18n::get("terrain.cocoon_here", "There is a cocoon here.") == "这里有一个茧。");
    REQUIRE(i18n::get("terrain.cocoon_on_fire", "The cocoon is on fire.") == "茧着火了。");
    REQUIRE(i18n::get("terrain.cocoon_empty", "The cocoon is empty.") == "茧是空的。");
    REQUIRE(i18n::get("terrain.cocoon_half_dissolved_body", "There is a half-dissolved human body inside!") == "里面有一具半溶解的人体！");
    REQUIRE(i18n::get("terrain.cocoon_spiders_inside", "There are spiders inside!") == "里面有蜘蛛！");
    REQUIRE(i18n::get("terrain.search_cocoon", "I search the Cocoon.") == "我搜索茧。");
    REQUIRE(i18n::get("terrain.it_is_empty", "It is empty.") == "它是空的。");
    REQUIRE(i18n::get("terrain.cocoon_opens", "The cocoon opens.") == "茧打开了。");
    REQUIRE(i18n::get("map.item_legend", "Item") == "物品");
    REQUIRE(i18n::get("marker.control_object.jammed_suffix", " is jammed.") == "被堵住了。");
    REQUIRE(i18n::get("marker.control_object.jam_prefix", "(c) Jam ") == "(c)堵住");
    REQUIRE(i18n::get("marker.control_object.deactivated_suffix", " is deactivated.") == "被停用了。");
    REQUIRE(i18n::get("marker.control_object.deactivate_crystal", "(d) Deactivate crystal") == "(d)停用水晶");
    REQUIRE(i18n::get("marker.control_object.strike_prefix", "(w) Strike ") == "(w)攻击");
    REQUIRE(i18n::get("marker.control_object.nothing_happens", "Nothing happens.") == "什么也没发生。");
    REQUIRE(i18n::get("marker.control_object.choose_another_position", "(space, esc) Choose another position") == "(space, esc) 选择另一个位置");
    REQUIRE(i18n::get("marker.control_object.title", "Control object") == "控制物体");
    REQUIRE(i18n::get("item_head.turns_left_suffix", " turns)") == "回合）");
    REQUIRE(i18n::get("insanity.babbling.char_descr", "Babbling") == "胡言乱语");
    REQUIRE(
        i18n::get(
            "insanity.phobia_rat.history",
            "Gained a phobia of rats") == "患上鼠类恐惧症");
    REQUIRE(
        i18n::get(
            "insanity.phobia_rat.trigger",
            "I am plagued by my phobia of rats!") == "我被对鼠类的恐惧所折磨！");
    REQUIRE(
        i18n::get(
            "insanity.phobia_spider.trigger",
            "I am plagued by my phobia of spiders!") == "我被对蜘蛛的恐惧所折磨！");
    REQUIRE(
        i18n::get(
            "insanity.phobia_reptile_and_amph.reptiles_trigger",
            "I am plagued by my phobia of reptiles!") == "我被对爬行动物的恐惧所折磨！");
    REQUIRE(
        i18n::get(
            "insanity.phobia_reptile_and_amph.amphibians_trigger",
            "I am plagued by my phobia of amphibians!") == "我被对两栖动物的恐惧所折磨！");
    REQUIRE(
        i18n::get(
            "insanity.phobia_canine.trigger",
            "I am plagued by my phobia of canines!") == "我被对犬类的恐惧所折磨！");
    REQUIRE(
        i18n::get(
            "insanity.phobia_dead.trigger",
            "I am plagued by my phobia of the dead!") == "我被对死者的恐惧所折磨！");
    REQUIRE(
        i18n::get(
            "insanity.phobia_deep.trigger",
            "I am plagued by my phobia of deep places!") == "我被对深处的恐惧所折磨！");
    REQUIRE(
        i18n::get(
            "insanity.phobia_dark.trigger",
            "I am plagued by my phobia of the dark!") == "我被对黑暗的恐惧所折磨！");
    REQUIRE(i18n::get("option.skip_intro_level.name", "Skip intro level") == "跳过开场关卡");
    REQUIRE(i18n::get("option.display_hints.once", "Once") == "一次");
    REQUIRE(i18n::get("option.auto_reload_weapons.descr", "Automatically perform a reload action instead if attempting to fire a ranged weapon with no ammo loaded.") == "如果试图在没有装填弹药的情况下开火，则自动执行装填动作。");
    REQUIRE(i18n::get("terrain_mob.cough", "I cough.") == "我咳嗽起来。");
    REQUIRE(i18n::get("terrain_mob.article_the_space", "the ") == "");
    REQUIRE(i18n::get("terrain_mob.article_a", "a") == "");
    REQUIRE(i18n::get("terrain_mob.article_the", "the") == "");
    REQUIRE(i18n::get("terrain_mob.smoke_name", "smoke") == "烟雾");
    REQUIRE(i18n::get("terrain_mob.mist_name", "mist") == "雾霭");
    REQUIRE(i18n::get("terrain_mob.force_field_name", " force field") == "力场");
    REQUIRE(i18n::get("terrain_mob.lit_stick_of_dynamite_name", " lit stick of dynamite") == "点燃的炸药棒");
    REQUIRE(i18n::get("terrain_mob.lit_flare_name", " lit flare") == "点燃的照明棒");
    REQUIRE(i18n::get("knockback.player_knocked_back", "I am knocked back!") == "我被击退了！");
    REQUIRE(
        i18n::get("game_commands.chilling_howl", "I let out a chilling howl.") ==
        "我发出一声令人胆寒的嚎叫。");
    REQUIRE(
        i18n::get("game_commands.make_some_noise", "I make some noise.") ==
        "我弄出一些声响。");
    REQUIRE(i18n::get("game_commands.press_help", "Press [?] for help.") == "按 [?] 查看帮助。");
    REQUIRE(i18n::get("msg_log.no_message_history", "No message history") == "没有消息历史");
    REQUIRE(
        i18n::get("map_controller.mid_game_warning", "") ==
        "没想到这个地方会延伸得这么深，这不可能！而且这里有明显的古代文明迹象，完全违背逻辑和理性！\n\n"
        "回去的路似乎不知为何已经迷失了，就算我想回去，我还能回去吗？");
    REQUIRE(
        i18n::get("map_controller.mid_game_horror", "") ==
        "它无穷无尽！这不可能是真的！我感觉自己像是在梦中行走；恐怖感压得人喘不过气。");
    REQUIRE(
        i18n::get("map_controller.long_sound_warning", "") ==
        "我听到远处传来微弱的回声。这里的每个声音似乎都传得更远 - 我需要小心行走。");
    REQUIRE(
        i18n::get("map_controller.presence_known", "I feel like my presence here is known!") ==
        "我感觉这里知道了我的存在！");
    REQUIRE(
        i18n::get("map_controller.ground_rumbles", "The ground rumbles...") ==
        "地面隆隆作响……");
    REQUIRE(
        i18n::get("map_controller.egypt_intro", "") ==
        "当我深入地下时，周围环境突然变化令我吃了一惊。这个房间里的古老建筑和我见过的一切都不同，"
        "石墙上刻满了复杂的象形文字。\n\n"
        "空气中弥漫着沙土和陌生熏香的气味，让人感觉仿佛被带到了另一个时代和地点。"
        "沉默令人窒息，只有我的脚步声在石地板上回荡。\n\n"
        "当我更仔细地看着这些雕刻时，我忍不住想知道创造它们的文明。"
        "他们是谁，又是如何在这么深的地下建造出这样的东西？");
    REQUIRE(
        i18n::get("map_controller.deep_one_lair_intro", "") ==
        "这里的墙壁因潮湿而滑腻，空气中充满盐水和明显的腐烂气味。"
        "我能感觉湿气贴在皮肤上，呼吸也变得短促。\n\n"
        "有隧道通向更深的地下水道，但我只能猜测它们会通往哪里。"
        "整个洞穴系统仿佛彼此相连，通道可能通向海洋深处，或地下湖泊与河流。");
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
    REQUIRE(i18n::get("actor_move.seeps_prefix", " seeps ") == "渗");
    REQUIRE(i18n::get("actor_move.space", " ") == "");
    REQUIRE(i18n::get("actor_move.squirms_through", " squirms through ") == "钻过");
    REQUIRE(i18n::get("attack_melee.player_miss", "I miss.") == "我没打中。");
    REQUIRE(i18n::get("attack_melee.i_am_hit", "I am hit") == "我被击中了");
    REQUIRE(i18n::get("attack_melee.hear_fighting", "I hear fighting.") == "我听到打斗声。");
    REQUIRE(i18n::get("attack_melee.attack_terrain_prefix", "Attacking ") == "攻击");
    REQUIRE(i18n::get("attack_melee.attack_terrain_with", " with ") == "，使用");
    REQUIRE(
        i18n::get("attack_melee.attack_terrain_suffix", " would be useless.") ==
        "会毫无用处。");
    REQUIRE(
        i18n::get("attack_melee.stopped_at_boundary_suffix", " is stopped at the boundary.") ==
        "被挡在边界处。");
    REQUIRE(i18n::get("property.stopped_at_boundary_suffix", " is stopped at the boundary.") == "被挡在边界处。");
    REQUIRE(i18n::get("property.possessed_by_middle", " was possessed by ") == "被");
    REQUIRE(i18n::get("property.possessed_by_suffix", "!") == "附身了！");
    REQUIRE(i18n::get("property.changes_shape", "It changes shape!") == "它改变了形态！");
    REQUIRE(i18n::get("property.stops_and_gropes_suffix", " stops and gropes about.") == "停下来摸索着。");
    REQUIRE(i18n::get("property.poison_player", "I am suffering from the poison!") == "我正遭受毒素折磨！");
    REQUIRE(i18n::get("property.suffers_from_poisoning_suffix", " suffers from poisoning!") == "中毒受苦！");
    REQUIRE(i18n::get("property.tears_out_spike_suffix", " tears out a spike!") == "扯出了一根钉子！");
    REQUIRE(i18n::get("property.pulls_me_suffix", " pulls me!") == "拉扯着我！");
    REQUIRE(i18n::get("property.splits_suffix", " splits.") == "分裂了。");
    REQUIRE(i18n::get("property.is_spawned_suffix", " is spawned.") == "出现了。");
    REQUIRE(i18n::get("property.spews_ooze_suffix", " spews ooze.") == "喷出了软泥。");
    REQUIRE(i18n::get("property.bewilders_me_suffix", " bewilders me.") == "使我困惑。");
    REQUIRE(i18n::get("property.is_taunting_me_suffix", " is taunting me!") == "正在嘲弄我！");
    REQUIRE(i18n::get("property.collapses_suffix", " collapses!") == "倒塌了！");
    REQUIRE(i18n::get("property.tomb_legions_sound", "A voice is calling forth Tomb-Legions!") == "有个声音正在召唤坟墓军团！");
    REQUIRE(i18n::get("property.their", "their") == "其");
    REQUIRE(i18n::get("property.its", "its") == "它的");
    REQUIRE(i18n::get("property.recognizes_me_as_middle", " recognizes me as ") == "承认我是");
    REQUIRE(i18n::get("property.leader_suffix", " leader.") == "领袖。");
    REQUIRE(i18n::get("property.great_frenzy_sound", "A voice is stirring up a great frenzy!") == "有个声音正在煽动巨大的狂乱！");
    REQUIRE(i18n::get("property.stirs_up_great_frenzy_suffix", " stirs up a great frenzy!") == "激起了巨大的狂乱！");
    REQUIRE(i18n::get("property.calls_plague_of_locusts_suffix", " calls a plague of Locusts!") == "召唤了一场蝗灾！");
    REQUIRE(i18n::get("spells.unexpected_effect", "An unexpected effect was induced by the spell.") == "法术引发了意想不到的效果。");
    REQUIRE(i18n::get("spells.suddenly_flayed_alive_suffix", " is suddenly flayed alive!") == "突然被活剥了！");
    REQUIRE(i18n::get("spells.dark_sphere_fizzles", "A dark sphere materializes, but quickly fizzles out.") == "一个黑暗球体成形，但很快就消散了。");
    REQUIRE(i18n::get("spells.darkbolt_release_sound", "I hear something rushing through the air.") == "我听到有什么东西划破空气飞来。");
    REQUIRE(i18n::get("spells.impact_sound", "I hear an impact.") == "我听到一声撞击。");
    REQUIRE(i18n::get("spells.aza_gaze_sound", "An insane cacophony resounds through the air!") == "疯狂的刺耳巨响在空中回荡！");
    REQUIRE(i18n::get("spells.me", "me") == "我");
    REQUIRE(i18n::get("spells.destruction_rages_prefix", "Destruction rages around ") == "毁灭在");
    REQUIRE(i18n::get("spells.destruction_rages_suffix", "!") == "周围肆虐！");
    REQUIRE(i18n::get("spells.explosion_sound", "I hear an explosion!") == "我听到一声爆炸！");
    REQUIRE(i18n::get("spells.rats_appear", "Rats appear!") == "鼠群出现了！");
    REQUIRE(i18n::get("spells.images_appear", "Images appear!") == "影像出现了！");
    REQUIRE(i18n::get("spells.weapon_visions", "Visions of hacking, crushing and stabbing fill my mind.") == "劈砍、碾碎与穿刺的景象充满了我的脑海。");
    REQUIRE(i18n::get("spells.is_struck_suffix", " is struck.") == "被击中了。");
    REQUIRE(i18n::get("spells.unravels_suffix", " unravels.") == "解体了。");
    REQUIRE(i18n::get("spells.symbol_fails_to_bind", "A symbol flickers briefly, but fails to bind here.") == "一个符号短暂闪烁，但未能在此处绑定。");
    REQUIRE(i18n::get("spells.sense_sigil_failed_to_bind", "I sense that the sigil failed to bind here.") == "我感觉印记未能在此处绑定。");
    REQUIRE(i18n::get("spells.momentary_void", "A momentary void opens and closes.") == "一个瞬间的虚空开启又闭合。");
    REQUIRE(i18n::get("spells.force_pushes_prefix", "A force pushes ") == "一股力量推着");
    REQUIRE(i18n::get("spells.force_pushes_suffix", "!") == "！");
    REQUIRE(i18n::get("spells.bugs_move_feebly", "The bugs on the ground suddenly move very feebly.") == "地上的虫子突然动得非常无力。");
    REQUIRE(i18n::get("spells.faint_stutter_in_time", "There is a faint stutter in time.") == "时间出现了一阵微弱的停顿。");
    REQUIRE(i18n::get("spells.bugs_move_slowly", "The bugs on the ground suddenly move very slowly.") == "地上的虫子突然动得非常缓慢。");
    REQUIRE(i18n::get("spells.bugs_scatter_away", "The bugs on the ground suddenly scatter away.") == "地上的虫子突然四散逃开。");
    REQUIRE(i18n::get("spells.bugs_attack_each_other", "The bugs on the ground all start to attack each other.") == "地上的虫子突然全都开始互相攻击。");
    REQUIRE(i18n::get("spells.scales_grow_over_my_eyes", "Scales grow over my eyes!") == "鳞片长满了我的眼睛！");
    REQUIRE(i18n::get("spells.scales_grow_over_eyes_prefix", "Scales grow over the eyes of ") == "鳞片长满了");
    REQUIRE(i18n::get("spells.period", ".") == "。");
    REQUIRE(i18n::get("spells.sharp_droning", "There is a sharp droning in my head!") == "我的脑中响起尖锐的嗡鸣！");
    REQUIRE(i18n::get("spells.feel_dizzy", "I feel dizzy.") == "我感到头晕。");
    REQUIRE(i18n::get("spells.flames_rising_prefix", "Flames are rising around ") == "火焰在");
    REQUIRE(i18n::get("spells.flames_rising_suffix", "!") == "周围升起！");
    REQUIRE(i18n::get("spells.vague_change_in_air", "There is a vague change in the air.") == "空气中有一阵模糊的变化。");
    REQUIRE(i18n::get("spells.nothing_appears", "Nothing appears.") == "什么也没有出现。");
    REQUIRE(i18n::get("spells.little_to_offer", "I feel like I have very little to offer.") == "我觉得自己几乎无物可献。");
    REQUIRE(i18n::get("spells.nothing_more_to_shed", "There is nothing more to shed.") == "已经没有更多可以舍弃的了。");
    REQUIRE(i18n::get("bash.attack_middle", " ") == "");
    REQUIRE(i18n::get("item_explosive.hear_explosion", "I hear an explosion!") == "我听到了一声爆炸！");
    REQUIRE(i18n::get("terrain_pylon.space", " ") == "");
    REQUIRE(i18n::get("terrain_pylon.destroyed_suffix", " is destroyed.") == "被摧毁了。");
    REQUIRE(i18n::get("terrain_pylon.the", "the ") == "");
    REQUIRE(i18n::get("terrain_pylon.angled_pylon", "Angled Pylon") == "斜角塔柱");
    REQUIRE(i18n::get("terrain_pylon.an_angled_pylon", "an Angled Pylon") == "斜角塔柱");
    REQUIRE(i18n::get("terrain_pylon.arched_pylon", "Arched Pylon") == "拱形塔柱");
    REQUIRE(i18n::get("terrain_pylon.an_arched_pylon", "an Arched Pylon") == "拱形塔柱");
    REQUIRE(i18n::get("terrain_pylon.coiled_pylon", "Coiled Pylon") == "螺旋塔柱");
    REQUIRE(i18n::get("terrain_pylon.a_coiled_pylon", "a Coiled Pylon") == "螺旋塔柱");
    REQUIRE(i18n::get("terrain_pylon.serrated_pylon", "A Serrated Pylon") == "锯齿塔柱");
    REQUIRE(i18n::get("terrain_pylon.a_serrated_pylon", "a Serrated Pylon") == "锯齿塔柱");
    REQUIRE(i18n::get("terrain_pylon.star_crowned_pylon", "Star-crowned Pylon") == "星冠塔柱");
    REQUIRE(i18n::get("terrain_pylon.a_star_crowned_pylon", "a Star-crowned Pylon") == "星冠塔柱");
    REQUIRE(i18n::get("terrain_pylon.two_pronged_pylon", "Two-pronged Pylon") == "双叉塔柱");
    REQUIRE(i18n::get("terrain_pylon.a_two_pronged_pylon", "a Two-pronged Pylon") == "双叉塔柱");
    REQUIRE(i18n::get("terrain_pylon.article_a", "a") == "");
    REQUIRE(i18n::get("terrain_pylon.article_an", "an") == "");
    REQUIRE(i18n::get("terrain_pylon.article_the", "the") == "");
    REQUIRE(i18n::get("terrain_pylon.cloaking_pylon_name", " Cloaking Pylon") == "隐形塔柱");
    REQUIRE(i18n::get("terrain_pylon.cloaking_pylon_effect", "turns creatures invisible") == "使生物隐形");
    REQUIRE(i18n::get("terrain_pylon.slowing_pylon_name", " Slowing Pylon") == "迟缓塔柱");
    REQUIRE(i18n::get("terrain_pylon.slowing_pylon_effect", "slows creatures") == "使生物迟缓");
    REQUIRE(i18n::get("terrain_pylon.accelerating_pylon_name", " Accelerating Pylon") == "加速塔柱");
    REQUIRE(i18n::get("terrain_pylon.accelerating_pylon_effect", "accelerates creatures") == "使生物加速");
    REQUIRE(i18n::get("terrain_pylon.repelling_pylon_name", " Repelling Pylon") == "排斥塔柱");
    REQUIRE(i18n::get("terrain_pylon.repelling_pylon_effect", "repels creatures") == "击退生物");
    REQUIRE(i18n::get("terrain_pylon.teleporting_pylon_name", " Teleporting Pylon") == "传送塔柱");
    REQUIRE(i18n::get("terrain_pylon.teleporting_pylon_effect", "teleports creatures") == "传送生物");
    REQUIRE(i18n::get("terrain_pylon.terror_pylon_name", " Terror Pylon") == "恐惧塔柱");
    REQUIRE(i18n::get("terrain_pylon.terror_pylon_effect", "causes fear") == "造成恐惧");
    REQUIRE(i18n::get("i18n.missing_key", "fallback") == "fallback");
    REQUIRE(i18n::get("drop.player_prefix", "I drop ") == "我丢下了");
    REQUIRE(i18n::get("drop.monster_drops", " drops ") == "丢下了");
    REQUIRE(i18n::get("drop.period", ".") == "。");
    REQUIRE(i18n::get("explosion.player_hit", "I am hit by an explosion!") == "我被爆炸击中了！");
    REQUIRE(i18n::get("explosion.survived_history", "Survived an explosion") == "从爆炸中幸存");
    REQUIRE(i18n::get("explosion.hear", "I hear an explosion!") == "我听到一声爆炸！");
    REQUIRE(i18n::get("item.weight_very_light", "very light") == "很轻");
    REQUIRE(i18n::get("item.weight_light", "light") == "轻");
    REQUIRE(i18n::get("item.weight_a_bit_heavy", "a bit heavy") == "有点重");
    REQUIRE(i18n::get("item.weight_heavy", "heavy") == "重");
    REQUIRE(i18n::get("item.cannot_apply", "I cannot apply that.") == "我不能使用那个。");
    REQUIRE(i18n::get("item_potion.feel_more_at_ease", "I feel more at ease.") == "我感到安心多了。");
    REQUIRE(i18n::get("item_potion.feel_insightful", "I feel insightful.") == "我感到洞察力涌现。");
    REQUIRE(i18n::get("item_potion.feel_fine", "I feel fine.") == "我感觉很好。");
    REQUIRE(i18n::get("item_potion.sinking_sensation_disappears", "I feel a faint sinking sensation, but it soon disappears...") == "我感到一阵微弱的下沉感，但很快就消失了……");
    REQUIRE(i18n::get("item_potion.fade_out_and_reappear", "I fade out of existence and reappear.") == "我从现实中淡出，然后又重新出现。");
    REQUIRE(i18n::get("item_weapon.looks_shocked_suffix", " looks shocked!") == "看起来震惊了！");
    REQUIRE(i18n::get("item_weapon.bows_before_me_suffix", " bows before me.") == "向我俯首。");
    REQUIRE(i18n::get("item_weapon.assailed_by_dark_energy_suffix", " is assailed by dark energy.") == "被黑暗能量袭击了。");
    REQUIRE(i18n::get("item.discovered_prefix", "I have discovered ") == "我发现了");
    REQUIRE(i18n::get("item.exclamation_mark", "!") == "！");
    REQUIRE(i18n::get("item.discovered_history_prefix", "Discovered ") == "发现了");
    REQUIRE(i18n::get("item.hit_suffix", " hit") == "命中");
    REQUIRE(
        i18n::get("throwing.unseen_creature", "An unseen creature") ==
        "有个看不见的生物");
    REQUIRE(i18n::get("throwing.is_hit_suffix", " is hit.") == "被击中了。");
    REQUIRE(i18n::get("throwing.player_throw_prefix", "I throw ") == "我投掷了");
    REQUIRE(i18n::get("throwing.monster_throws", " throws ") == "投掷了");
    REQUIRE(i18n::get("throwing.period", ".") == "。");
    REQUIRE(i18n::get("throwing.creature_hit", "A creature is hit.") == "有生物被击中了。");
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
