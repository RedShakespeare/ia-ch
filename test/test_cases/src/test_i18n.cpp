// =============================================================================
// Copyright Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include <filesystem>
#include <fstream>
#include <regex>
#include <string>
#include <vector>

#include "catch.hpp"
#include "common_text.hpp"
#include "config.hpp"
#include "i18n.hpp"
#include "messages.hpp"
#include "paths.hpp"

namespace
{
void require_translation(const std::string& key, const std::string& expected)
{
    INFO("i18n key: " << key);
    REQUIRE(i18n::get(key, "__missing_translation__") == expected);
}
}  // namespace

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
    REQUIRE(i18n::language_name("en") == "英语");
    REQUIRE(i18n::get("main_menu.options", "(O)Options") == "(O)选项");
    REQUIRE(
        i18n::get(
            "version.copyright",
            "(c) 2011-2025 Martin Tornqvist") == "(c) 2011-2025 Martin Tornqvist");
    REQUIRE(
        i18n::get(
            "version.license",
            "Infra Arcana is free software, see LICENSE.txt.") ==
        "Infra Arcana 是自由软件，详见 LICENSE.txt。");
    REQUIRE(common_text::g_screen_exit_hint == "[space, esc] 退出");
    REQUIRE(i18n::get("option.base_delay.query_title", "Projectile delay") == "投射物延迟");
    REQUIRE(i18n::get("hints.title_prefix", "Hint: ") == "提示：");
    REQUIRE(i18n::get("hints.altars.title", "Altars") == "祭坛");
    REQUIRE(
        i18n::get(
            "hints.altars.body",
            "All spells are cast at a higher level when standing "
            "at an altar - this includes both spells cast from "
            "manuscripts and from memory.") ==
        "站在祭坛上时，所有法术都会以更高等级施放——这包括从手稿和记忆中施放的法术。");
    REQUIRE(i18n::get("hints.fountains.title", "Fountains") == "喷泉");
    REQUIRE(
        i18n::get(
            "hints.fountains.body",
            "Drinking from a fountain usually restores a bit of "
            "health, spirit, and mental shock (but they can sometimes "
            "have other effects, both good and bad!). Fountains "
            "can be drunk from several times, but each time there "
            "is a chance that it will dry up permanently.") ==
        "从喷泉饮水通常会恢复少量生命、精神和震惊值（但它们有时也会产生其他效果，有好有坏！）。喷泉可以饮用多次，但每次都有机会永久干涸。");
    REQUIRE(i18n::get("hints.destroying_corpses.title", "Destroying corpses") == "摧毁尸体");
    REQUIRE(
        i18n::get(
            "hints.destroying_corpses.body",
            "Corpses can be destroyed by pressing [k] or [w]. This "
            "can be very useful against certain types of monsters. "
            "Some weapons, such as Machetes, makes it easier to "
            "destroy corpses - check the item description to see "
            "if a weapon has such a bonus. Also, a well-placed "
            "stick of dynamite or Molotov Cocktail is usually an "
            "effective way of stopping persistent monsters.") ==
        "可以按 [k] 或 [w] 摧毁尸体。这在对付某些类型的怪物时非常有用。有些武器，例如砍刀，会让摧毁尸体更容易——查看物品描述即可知道武器是否有这种加成。此外，一根位置合适的炸药或燃烧瓶通常也是阻止顽固怪物的有效方法。");
    REQUIRE(i18n::get("hints.unload_weapons.title", "Unloading weapons") == "卸下武器");
    REQUIRE(
        i18n::get(
            "hints.unload_weapons.body",
            "Ammunition can be unloaded from firearms on the "
            "ground by pressing [u] or [G].") == "可以按 [u] 或 [G] 卸下地上火器中的弹药。");
    REQUIRE(i18n::get("hints.infected.title", "Infected") == "感染");
    REQUIRE(
        i18n::get(
            "hints.infected.body",
            "Infections should be treated as soon as possible. "
            "The common way of doing this is by using the "
            "Medical Bag. It only requires a small number of turns "
            "and resources, but if the work is interrupted, the "
            "effort is wasted (no medical resources are lost "
            "on interruption however)."
            "\n\nAn untreated infection will eventually turn into a "
            "disease (50% maximum hit points), "
            "which can only be removed through special means such as "
            "drinking certain potions.") ==
        "感染应尽快治疗。常见做法是使用医疗包。这只需要少量回合和资源，但如果操作被打断，努力就会白费（不过打断时不会损失医疗资源）。"
        "\n\n未经治疗的感染最终会变成疾病（最大生命值降低 50%），只能通过特殊手段移除，例如饮用某些药水。");
    REQUIRE(i18n::get("hints.overburdened.title", "Overburdened") == "负重过重");
    REQUIRE(
        i18n::get(
            "hints.overburdened.body",
            "Carrying too much weight makes movement take twice "
            "as much time. This is a very dangerous and "
            "detrimental situation.") == "携带过多重量会让移动耗时加倍。这是非常危险且不利的情况。");
    REQUIRE(i18n::get("hints.high_shock.title", "High shock") == "高度震惊");
    REQUIRE(
        i18n::get(
            "hints.high_shock.body",
            "Being in a state of extreme mental shock (stress, paranoia) "
            "will cause a sanity hit. One way to reduce shock, "
            "and thereby avoiding or prolonging the sanity hit, "
            "is to find a source of light - for example through "
            "activating an Electric Lantern or igniting a Flare.") ==
        "处于极端精神震惊（压力、偏执）状态会导致理智受损。降低震惊的一种方式是寻找光源，例如激活电提灯或点燃照明棒，这可以避免或延缓理智受损。");
    REQUIRE(i18n::get("hints.status_effects.title", "Status effects") == "状态效果");
    REQUIRE(
        i18n::get(
            "hints.status_effects.body",
            "A status effect has been applied. "
            "Status effects are various positive, negative or neutral effects "
            "applied on a creature. "
            "Some examples are confusion, burning, invisibility, or "
            "electricity resistance. "
            "A simple list of active status effects is shown in the normal "
            "game screen. "
            "\n\nIn the character screen (accessed by pressing [C] or [@]), a more "
            "detailed list can be seen, including a description of each effect. "
            "\n\nStatus effects shown with CAPITAL LETTERS are \"permanent\", "
            "and are only removed if some special action is taken, for example "
            "using the medical bag to treat a wound.") ==
        "一个状态效果已经生效。状态效果是施加在生物身上的各种正面、负面或中性效果。例如混乱、燃烧、隐形或电击抗性。普通游戏画面会显示一份简略的活动状态效果列表。"
        "\n\n在角色画面中（按 [C] 或 [@] 进入），可以看到更详细的列表，包括每个效果的描述。"
        "\n\n以大写字母显示的状态效果是“永久”的，只有采取某些特殊行动才会移除，例如使用医疗包治疗伤口。");
    REQUIRE(i18n::get("hints.study_inscription.title", "Inscriptions") == "铭文");
    REQUIRE(
        i18n::get(
            "hints.study_inscription.body",
            "There is an inscription here, studying it will yield some experience."
            "\n\nIt may also recall a spell that you have forgotten, "
            "or reveal something about carried manuscripts or potions. "
            "The chance to reveal information about such items is higher with "
            "more unknown items carried.") ==
        "这里有一段铭文，研究它会获得一些经验。"
        "\n\n它也可能让你回忆起遗忘的法术，或揭示携带的手稿或药水的信息。携带的未知物品越多，揭示这些物品信息的机会越高。");
    REQUIRE(i18n::get("hints.kick_brazier.title", "Kicking braziers") == "踢倒火盆");
    REQUIRE(
        i18n::get(
            "hints.kick_brazier.body",
            "Braziers can be kicked over to set creatures on fire "
            "in a small area.") == "可以踢倒火盆，在小范围内点燃生物。");
    REQUIRE(i18n::get("hints.kick_statue.title", "Kicking statues") == "踢倒雕像");
    REQUIRE(
        i18n::get(
            "hints.kick_statue.body",
            "Statues can be kicked over to "
            "damage and stun a creature on the other side.") == "可以踢倒雕像，伤害并击晕另一侧的生物。");
    REQUIRE(
        i18n::get(
            "hints.temporary_and_permanent_shock.title",
            "Temporary and permanent mental shock") == "临时和永久精神震惊");
    REQUIRE(
        i18n::get(
            "hints.temporary_and_permanent_shock.body",
            "Some situations cause \"temporary\" mental shock, "
            "which is removed when the situation changes. "
            "Entering a dark area or standing next to bloodsplatter will "
            "cause your shock to spike until you move away, for example."
            "\n\nStanding in bright light will similarly reduce your shock "
            "until you return to the ambient subterranean gloom."
            "\n\nSeeing monsters, casting spells, spending time, etc cause "
            "\"permanent\" shock, which will not go away until "
            "the next floor is reached, insanity rises (due to shock at 100%), "
            "or the shock is cured somehow.") ==
        "有些情况会造成“临时”精神震惊，当情况改变时它会被移除。例如进入黑暗区域或站在血迹旁边会让你的震惊值飙升，直到你离开。"
        "\n\n站在明亮光线中同样会降低你的震惊值，直到你回到地下环境的昏暗之中。"
        "\n\n看见怪物、施放法术、花费时间等会造成“永久”震惊，它不会消失，直到抵达下一层、震惊达到 100% 导致疯狂上升，或通过某种方式治愈震惊。");
    REQUIRE(i18n::get("create_character.background_title", "What is your background?") == "你的背景是什么？");
    REQUIRE(i18n::get("create_character.bot_name", "Bot") == "机器人");
    REQUIRE(i18n::get("create_character.default_player_name", "Player") == "玩家");
    REQUIRE(i18n::get("spells.domain.channeling", "Channeling") == "导能");
    REQUIRE(i18n::get("spells.domain.corruption", "Corruption") == "腐化");
    REQUIRE(i18n::get("spells.domain.illusion", "Illusion") == "幻象");
    REQUIRE(i18n::get("spells.domain.mind", "Mind") == "心灵");
    REQUIRE(i18n::get("spells.domain.time", "Time") == "时间");
    REQUIRE(i18n::get("spells.domain.warding", "Warding") == "守护");
    REQUIRE(i18n::get("spells.domain.blood", "Blood") == "鲜血");
    REQUIRE(i18n::get("player_bon.background.exorcist.title", "Exorcist") == "驱魔者");
    REQUIRE(i18n::get("player_bon.background.flagellant.title", "Flagellant") == "鞭笞者");
    REQUIRE(i18n::get("player_bon.background.ghoul.title", "Ghoul") == "食尸鬼");
    REQUIRE(i18n::get("player_bon.background.occultist.title", "Occultist") == "神秘学者");
    REQUIRE(i18n::get("player_bon.background.rogue.title", "Rogue") == "盗贼");
    REQUIRE(i18n::get("player_bon.background.war_vet.title", "War Veteran") == "战争老兵");
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
    REQUIRE(
        i18n::get(
            "view_actor_descr.natural_property_physical_damage",
            "{COLOR_GRAY}physical damage{reset_color}") ==
        "{COLOR_GRAY}物理伤害{reset_color}");
    REQUIRE(
        i18n::get(
            "view_actor_descr.natural_property_fire",
            "{COLOR_LIGHT_RED}fire{reset_color}") ==
        "{COLOR_LIGHT_RED}火焰{reset_color}");
    REQUIRE(
        i18n::get(
            "view_actor_descr.natural_property_confusion",
            "confusion") == "混乱");
    REQUIRE(
        i18n::get(
            "view_actor_descr.natural_property_see_in_darkness",
            "see in darkness") == "在黑暗中视物");
    REQUIRE(
        i18n::get(
            "view_actor_descr.natural_property_reduced_pierce_damage",
            "Piercing attacks such as pistol shots or dagger strikes are very "
            "ineffective against them") ==
        "手枪射击或匕首刺击等穿刺攻击对它们非常无效");
    REQUIRE(
        i18n::get(
            "view_actor_descr.natural_property_flammable",
            "They are very flammable, and will quickly ignite other nearby "
            "flammable creatures") ==
        "它们非常易燃，并会迅速点燃附近其他易燃生物");
    REQUIRE(
        i18n::get(
            "view_actor_descr.natural_property_cannot_be_harmed_by",
            "They cannot be harmed by") == "它们不会受到");
    REQUIRE(
        i18n::get(
            "view_actor_descr.natural_property_can",
            "They can") == "它们能");
    REQUIRE(
        i18n::get(
            "view_actor_descr.natural_property_item_separator",
            " ") == "");
    REQUIRE(
        i18n::get(
            "view_actor_descr.natural_property_or",
            "or ") == "或");
    REQUIRE(
        i18n::get(
            "view_actor_descr.melee_hit_chance_prefix",
            "The chance to hit ") == "命中");
    REQUIRE(
        i18n::get(
            "view_actor_descr.melee_hit_chance_same_suffix",
            " with a melee attack or kicking is currently{_}{COLOR_LIGHT_GREEN}") ==
        "的近战攻击或踢击命中率当前为{_}{COLOR_LIGHT_GREEN}");
    REQUIRE(
        i18n::get(
            "view_actor_descr.melee_hit_chance_weapon_suffix",
            " with a melee attack is currently{_}{COLOR_LIGHT_GREEN}") ==
        "的近战攻击命中率当前为{_}{COLOR_LIGHT_GREEN}");
    REQUIRE(
        i18n::get(
            "view_actor_descr.pct_color_suffix",
            "%{reset_color}") == "%{reset_color}");
    REQUIRE(
        i18n::get(
            "view_actor_descr.melee_hit_chance_kick_suffix",
            ", and the chance to hit by kicking is{_}{COLOR_LIGHT_GREEN}") ==
        "，踢击命中率为{_}{COLOR_LIGHT_GREEN}");
    REQUIRE(
        i18n::get(
            "view_actor_descr.because_unaware_suffix",
            " (because they are unaware)") == "（因为它们未察觉）");
    REQUIRE(
        i18n::get(
            "view_actor_descr.sneak_chance_prefix",
            "The chance to remain undetected by ") == "不被");
    REQUIRE(
        i18n::get(
            "view_actor_descr.sneak_chance_suffix",
            " is currently{_}{COLOR_LIGHT_GREEN}") == "发现的几率当前为{_}{COLOR_LIGHT_GREEN}");
    REQUIRE(
        i18n::get(
            "view_actor_descr.cannot_visually_detect",
            "They cannot visually detect other creatures") == "它们无法通过视觉侦测其他生物");
    REQUIRE(
        i18n::get(
            "view_actor_descr.pursues_once_aware_suffix",
            " (but will pursue any threat once aware)") == "（但一旦察觉到威胁就会追击）");
    REQUIRE(
        i18n::get(
            "view_actor_descr.property_title_separator",
            "{color_reset}: ") == "{color_reset}：");
    REQUIRE(i18n::get("game_over_summary.title", "Game summary") == "游戏总结");
    REQUIRE(i18n::get("game.death_message", "-I AM DEAD!-") == "-我死了！-");
    REQUIRE(i18n::get("game_time.sink_downwards", "I sink downwards!") == "我向下沉去！");
    require_translation(
        "game.intro_default",
        "我站在一条鹅卵石森林小径的尽头，面前是一座被人避讳、破败古老的教堂建筑。"
        "这里通向令人憎恶的“星智教团”的领地。我决心进入这些蔓延的地下墓穴，"
        "夺取其中的财宝和知识。在下方某处，躺着我真正的命运，一件非人起源的神器，"
        "被称作{COLOR_YELLOW}“闪耀的偏方三八面体”{reset_color}"
        "——一扇通往宇宙一切秘密的窗口！");
    require_translation(
        "game.intro_exorcist",
        "我站在一条鹅卵石森林小径的尽头，面前是一座被人避讳、破败古老的教堂建筑。"
        "这里通向令人憎恶的“星智教团”的领地。我决心进入这些蔓延的地下墓穴，"
        "肃清其中盘踞的腐化。在下方某处，有一件非人起源的神器，"
        "被称作{COLOR_YELLOW}“闪耀的偏方三八面体”{reset_color}，"
        "据说它是一扇通往宇宙一切秘密的窗口。必须摧毁它，免得再有人受其虚假承诺诱惑！");
    require_translation(
        "game.win_default_approach_crystal",
        "当我接近水晶时，一阵诡异的光芒照亮了周围。我注意到一个身影正在光线边缘观察我。"
        "对于这个实体的本质，我心中毫无疑问。");
    require_translation(
        "game.win_default_panic",
        "我陷入恐慌。为什么我会在这里，在黑暗中跌跌撞撞？这一切都是某个计划的一部分吗？"
        "那个存在召唤我凝视石头。");
    require_translation(
        "game.win_default_visions",
        "在光辉中，我看见超越永恒的异象，看见非现实的现实，看见白昼最明亮的光和疯狂最黑暗的夜。"
        "现在唯有前行，我必须看见，我必须知晓。");
    require_translation("game.win_default_pact", "于是我与邪魔立下契约。");
    require_translation(
        "game.win_default_harness_shadows",
        "我如今驾驭那些跨越世界、播撒死亡与疯狂的阴影。"
        "地上万物，无论生者或死者，其命运皆归于我。");
    REQUIRE(i18n::get("inventory.slot.weapon", "Weapon") == "武器");
    REQUIRE(i18n::get("inventory.browsing_title", "Browsing inventory") == "浏览物品栏");
    REQUIRE(i18n::get("inventory.empty_slot", "<empty>") == "<空>");
    REQUIRE(i18n::get("inventory.drop_how_many_suffix", "- drop how many?") == "- 丢弃多少？");
    REQUIRE(i18n::get("inventory.throw.title", "Throw which item?") == "投掷哪件物品？");
    REQUIRE(i18n::get("inventory.not_while_burning", "Not while burning.") == "燃烧时不能这么做。");
    REQUIRE(i18n::get("item_armor.damage_prefix", "My ") == "我的");
    REQUIRE(i18n::get("item_armor.damage_suffix", " is damaged!") == "受损了！");
    REQUIRE(i18n::get("item_armor.info_suffix", " armor)") == "护甲）");
    REQUIRE(
        i18n::get(
            "item_armor.joins_with_skin",
            "The armor joins with my skin!") == "护甲与我的皮肤融合了！");
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
    REQUIRE(i18n::get("terrain.way_blocked", "The way is blocked.") == "去路被挡住了。");
    REQUIRE(i18n::get("terrain.bump_into_something", "I bump into something.") == "我撞到了什么东西。");
    REQUIRE(i18n::get("terrain.chasm_in_way", "A chasm lies in my way.") == "一道深渊挡住了我的路。");
    REQUIRE(
        i18n::get(
            "terrain.chasm_edge",
            "I realize I am standing on the edge of a chasm.") == "我意识到自己正站在深渊边缘。");
    REQUIRE(i18n::get("terrain.scorched_by_flames_player", "I am scorched by flames.") == "我被火焰灼伤了。");
    REQUIRE(i18n::get("terrain.scorched_by_flames_suffix", " is scorched by flames.") == "被火焰灼伤了。");
    REQUIRE(i18n::get("terrain.fire_spread_here", "Fire has spread here!") == "火势蔓延到了这里！");
    REQUIRE(i18n::get("terrain.step_into_flames_query", "Step into the flames? ") == "要踏入火焰吗？");
    REQUIRE(i18n::get("terrain.catches_fire_suffix", " catches fire.") == "着火了。");
    REQUIRE(i18n::get("terrain.wade_through_water", "I wade slowly through the knee high water.") == "我缓慢涉过齐膝深的水。");
    REQUIRE(i18n::get("terrain.trudge_through_mud", "I trudge slowly through the knee high mud.") == "我缓慢跋涉过齐膝深的泥泞。");
    REQUIRE(i18n::get("terrain.hear_splash", "I hear a splash.") == "我听到水花声。");
    REQUIRE(i18n::get("terrain.legend_inscribed_object", "Inscribed Object") == "铭文物体");
    REQUIRE(i18n::get("terrain.legend_stairs", "Stairs") == "楼梯");
    REQUIRE(i18n::get("terrain.legend_gleaming_crystal", "Gleaming Crystal") == "闪光水晶");
    REQUIRE(i18n::get("terrain.legend_altar", "Altar") == "祭坛");
    REQUIRE(i18n::get("terrain.legend_brazier", "Brazier") == "火盆");
    REQUIRE(i18n::get("terrain.legend_tomb", "Tomb") == "坟墓");
    REQUIRE(i18n::get("terrain.legend_fountain", "Fountain") == "喷泉");
    REQUIRE(i18n::get("terrain.article_a_space", "a ") == "");
    REQUIRE(i18n::get("terrain.article_an_space", "an ") == "");
    REQUIRE(i18n::get("terrain.article_the_space", "the ") == "");
    REQUIRE(i18n::get("terrain.floor_article_a", "") == "");
    REQUIRE(i18n::get("terrain.floor_flames", "flames") == "火焰");
    REQUIRE(i18n::get("terrain.floor_scorched_prefix", "scorched ") == "焦黑的");
    REQUIRE(i18n::get("terrain.floor_stone_floor", "stone floor") == "石地板");
    REQUIRE(i18n::get("terrain.floor_cavern_floor", "cavern floor") == "洞穴地面");
    REQUIRE(i18n::get("terrain.floor_stone_path", "stone path") == "石径");
    REQUIRE(i18n::get("terrain.wall_stone", "stone wall") == "石墙");
    REQUIRE(i18n::get("terrain.wall_alien", "alien wall") == "异星墙壁");
    REQUIRE(i18n::get("terrain.wall_cavern", "cavern wall") == "洞穴墙壁");
    REQUIRE(i18n::get("terrain.wall_cliff", "cliff") == "悬崖");
    REQUIRE(i18n::get("terrain.wall_moss_grown_prefix", "moss-grown ") == "长苔的");
    REQUIRE(i18n::get("terrain.pillar_broken", "broken pillar") == "断裂石柱");
    REQUIRE(i18n::get("terrain.pillar_inscribed", "inscribed pillar") == "刻文石柱");
    REQUIRE(i18n::get("terrain.pillar", "pillar") == "石柱");
    REQUIRE(i18n::get("terrain.vegetation_article_a", "") == "");
    REQUIRE(i18n::get("terrain.grass", "grass") == "草");
    REQUIRE(i18n::get("terrain.grass_withered", "withered grass") == "枯草");
    REQUIRE(i18n::get("terrain.grass_burning", "burning grass") == "燃烧的草");
    REQUIRE(i18n::get("terrain.grass_scorched_ground", "scorched ground") == "焦黑地面");
    REQUIRE(i18n::get("terrain.shrub", "shrub") == "灌木");
    REQUIRE(i18n::get("terrain.shrub_withered", "withered shrub") == "枯萎灌木");
    REQUIRE(i18n::get("terrain.shrub_burning", "burning shrub") == "燃烧的灌木");
    REQUIRE(i18n::get("terrain.vines_hanging", "hanging vines") == "垂藤");
    REQUIRE(i18n::get("terrain.vines_burning", "burning vines") == "燃烧的藤蔓");
    REQUIRE(i18n::get("terrain.vegetation_burning_prefix", "burning ") == "燃烧的");
    REQUIRE(i18n::get("terrain.vegetation_scorched_prefix", "scorched ") == "焦黑的");
    REQUIRE(i18n::get("terrain.tree_giant_fungi", "giant fungi") == "巨型真菌");
    REQUIRE(i18n::get("terrain.tree", "tree") == "树");
    REQUIRE(i18n::get("terrain.petroglyph", "petroglyph") == "岩刻");
    REQUIRE(i18n::get("terrain.debris_big_pile", "big pile of debris") == "大堆碎石");
    REQUIRE(i18n::get("terrain.rubble_article_a", "") == "");
    REQUIRE(i18n::get("terrain.rubble_burning_prefix", "burning ") == "燃烧的");
    REQUIRE(i18n::get("terrain.rubble", "rubble") == "瓦砾");
    REQUIRE(i18n::get("terrain.bones_article_a", "") == "");
    REQUIRE(i18n::get("terrain.bones", "bones") == "骸骨");
    REQUIRE(i18n::get("terrain.gravestone_prefix", "gravestone (\"") == "墓碑（\"");
    REQUIRE(i18n::get("terrain.gravestone_suffix", "\")") == "\"）");
    REQUIRE(i18n::get("terrain.church_bench", "church bench") == "教堂长椅");
    REQUIRE(i18n::get("terrain.statue", "statue") == "雕像");
    REQUIRE(
        i18n::get(
            "terrain.statue_of_ghoulish_creature",
            "statue of a ghoulish creature") == "食尸鬼状生物的雕像");
    REQUIRE(i18n::get("terrain.urn_inscribed", "inscribed urn") == "刻文瓮");
    REQUIRE(i18n::get("terrain.urn", "urn") == "瓮");
    REQUIRE(i18n::get("terrain.stalagmite", "stalagmite") == "石笋");
    REQUIRE(i18n::get("terrain.carpet_article_a", "") == "");
    REQUIRE(i18n::get("terrain.carpet", "carpet") == "地毯");
    REQUIRE(i18n::get("terrain.grate", "grate") == "栅格");
    REQUIRE(i18n::get("terrain.brazier", "brazier") == "火盆");
    REQUIRE(i18n::get("terrain.fixture_burning_prefix", "burning ") == "燃烧的");
    REQUIRE(i18n::get("terrain.cabinet", "cabinet") == "柜子");
    REQUIRE(i18n::get("terrain.bookshelf", "bookshelf") == "书架");
    REQUIRE(
        i18n::get(
            "terrain.alchemist_workbench",
            "alchemist's workbench") == "炼金术士的工作台");
    REQUIRE(i18n::get("terrain.cocoon", "cocoon") == "茧");
    REQUIRE(i18n::get("terrain.tomb_empty_prefix", "empty ") == "空的");
    REQUIRE(i18n::get("terrain.tomb_open_prefix", "open ") == "打开的");
    REQUIRE(i18n::get("terrain.tomb_ornate_prefix", "ornate ") == "华丽的");
    REQUIRE(i18n::get("terrain.tomb_marvelous_prefix", "marvelous ") == "非凡的");
    REQUIRE(i18n::get("terrain.tomb", "tomb") == "坟墓");
    REQUIRE(i18n::get("terrain.chest_wooden_prefix", "wooden ") == "木制");
    REQUIRE(i18n::get("terrain.chest_iron_prefix", "iron ") == "铁制");
    REQUIRE(i18n::get("terrain.chest_empty_prefix", "empty ") == "空的");
    REQUIRE(i18n::get("terrain.chest_open_prefix", "open ") == "打开的");
    REQUIRE(i18n::get("terrain.chest_locked_prefix", "locked ") == "锁住的");
    REQUIRE(i18n::get("terrain.chest", "chest") == "箱子");
    REQUIRE(i18n::get("terrain.fountain_dried_up_name", "dried-up") == "干涸的");
    REQUIRE(i18n::get("terrain.fountain_type_separator", " ") == "");
    REQUIRE(i18n::get("terrain.fountain", "fountain") == "喷泉");
    REQUIRE(i18n::get("terrain.fountain_refreshing", "refreshing") == "清爽的");
    REQUIRE(i18n::get("terrain.fountain_exalting", "exalting") == "振奋的");
    REQUIRE(i18n::get("terrain.fountain_cursed", "cursed") == "受诅咒的");
    REQUIRE(i18n::get("terrain.fountain_diseased", "diseased") == "染病的");
    REQUIRE(i18n::get("terrain.fountain_poisonous", "poisonous") == "有毒的");
    REQUIRE(i18n::get("terrain.fountain_enraging", "enraging") == "激怒的");
    REQUIRE(i18n::get("terrain.fountain_paralyzing", "paralyzing") == "麻痹的");
    REQUIRE(i18n::get("terrain.fountain_blinding", "blinding") == "致盲的");
    REQUIRE(i18n::get("terrain.fountain_sleep_inducing", "sleep-inducing") == "催眠的");
    REQUIRE(i18n::get("terrain.downward_staircase", "downward staircase") == "向下的楼梯");
    REQUIRE(i18n::get("terrain.bridge", "bridge") == "桥");
    REQUIRE(i18n::get("terrain.water", "water") == "水");
    REQUIRE(i18n::get("terrain.shallow_mud", "shallow mud") == "浅泥");
    REQUIRE(i18n::get("terrain.gleaming_pool", "gleaming pool") == "闪光水池");
    REQUIRE(i18n::get("terrain.chasm", "chasm") == "深渊");
    REQUIRE(i18n::get("terrain.crystal_gleaming", "gleaming") == "闪光");
    REQUIRE(i18n::get("terrain.crystal_dead", "dead") == "失活");
    REQUIRE(i18n::get("terrain.crystal_suffix", " crystal") == "水晶");
    REQUIRE(i18n::get("terrain.altar", "altar") == "祭坛");
    REQUIRE(i18n::get("terrain.chains_article_a", "") == "");
    REQUIRE(i18n::get("terrain.chains_article_the", "the ") == "");
    REQUIRE(i18n::get("terrain.chains_name", "rusty chains") == "生锈的锁链");
    REQUIRE(i18n::get("terrain.chains_rattle", "The chains rattle.") == "锁链嘎嘎作响。");
    REQUIRE(i18n::get("terrain.hear_chains_rattling", "I hear chains rattling.") == "我听到锁链嘎嘎作响。");
    REQUIRE(i18n::get("terrain.topples_prefix", "The ") == "");
    REQUIRE(i18n::get("terrain.topples_suffix", " topples over.") == "倒下了。");
    REQUIRE(i18n::get("text_format.and_separator", " and ") == "和");
    REQUIRE(i18n::get("terrain.falls_on_me", "It falls on me!") == "它砸到了我！");
    REQUIRE(i18n::get("terrain.falls_on_prefix", "It falls on ") == "它砸到了");
    REQUIRE(i18n::get("terrain.period", ".") == "。");
    REQUIRE(i18n::get("terrain.church_bench_destroyed", "The church bench is destroyed.") == "教堂长椅被摧毁了。");
    REQUIRE(i18n::get("terrain.wiggles", "It wiggles a bit.") == "它摇晃了一下。");
    REQUIRE(i18n::get("terrain.stairs_down_title", "A staircase leading downwards") == "向下的楼梯");
    REQUIRE(i18n::get("terrain.descend_option", "(D)escend") == "(D)下楼");
    REQUIRE(i18n::get("terrain.save_and_quit_option", "(S)ave and quit") == "(S)保存并退出");
    REQUIRE(i18n::get("terrain.descend_stairs", "I descend the stairs.") == "我走下楼梯。");
    REQUIRE(
        i18n::get(
            "terrain.fake_stairs_body",
            "As I descend the stairs and observe my surroundings, to my "
            "great bewilderment I realize that I have stepped out into "
            "the very same ground from which I started my downward climb! "
            "Turning around, the stairs are nowhere to be found.") ==
        "当我走下楼梯并观察周围时，我惊讶地发现自己竟然踏回了开始下行的同一片地面！转身一看，楼梯已经无处可寻。");
    REQUIRE(i18n::get("terrain.seems_cleansed_prefix", "The ") == "");
    REQUIRE(i18n::get("terrain.seems_cleansed_suffix", " seems cleansed!") == "看起来被净化了！");
    REQUIRE(i18n::get("terrain.touch_prefix", "I touch ") == "我触摸了");
    REQUIRE(i18n::get("terrain.touch_crystal_object", "I touch some crystal object.") == "我触摸了某个水晶物体。");
    REQUIRE(i18n::get("terrain.nothing_happens", "Nothing happens.") == "什么也没发生。");
    REQUIRE(i18n::get("terrain.light_inside_fades", "The light inside fades.") == "里面的光芒消退了。");
    REQUIRE(i18n::get("terrain.path_opened", "I sense that a path has opened somewhere.") == "我感到某处有一条道路打开了。");
    REQUIRE(i18n::get("terrain.altar_destroyed", "The altar is destroyed.") == "祭坛被摧毁了。");
    REQUIRE(
        i18n::get(
            "terrain.diabolic_altar_warning",
            "A diabolic altar has been raised here, it must be "
            "destroyed!") == "这里升起了一座邪恶祭坛，必须将它摧毁！");
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
    REQUIRE(i18n::get("terrain.tomb_air_colder", "The air suddenly feels colder.") == "空气突然变冷了。");
    REQUIRE(i18n::get("terrain.tomb_repulsive_creeps_up", "Something repulsive creeps up from the tomb!") == "有什么令人作呕的东西从坟墓里爬了出来！");
    REQUIRE(i18n::get("terrain.tomb_something_rises", "Something rises from the tomb!") == "有什么东西从坟墓中升起！");
    REQUIRE(i18n::get("terrain.tomb_fumes_burst", "Fumes burst out from the tomb!") == "烟气从坟墓中喷涌而出！");
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
    REQUIRE(
        i18n::get(
            "terrain.fountain_here_dried_up",
            "There is a fountain here, "
            "but it's dried-up.") == "这里有一座喷泉，但它已经干涸了。");
    REQUIRE(i18n::get("terrain.drink_from_fountain", "I drink from the fountain...") == "我从喷泉中饮水……");
    REQUIRE(i18n::get("terrain.drink_from_prefix", "Drink from ") == "从");
    REQUIRE(i18n::get("terrain.query_suffix", "?") == "？");
    REQUIRE(i18n::get("terrain.there_is_prefix", "There is ") == "这里有");
    REQUIRE(i18n::get("terrain.here_drink_from_it_suffix", " here. Drink from it?") == "。要从中饮水吗？");
    REQUIRE(i18n::get("terrain.water_in_prefix", "The water in ") == "喷泉");
    REQUIRE(i18n::get("terrain.seems_clearer_suffix", " seems clearer.") == "中的水似乎更清澈了。");
    REQUIRE(i18n::get("terrain.seems_murkier_suffix", " seems murkier.") == "中的水似乎更浑浊了。");
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
    REQUIRE(i18n::get("terrain.search_bookshelf", "I search the bookshelf.") == "我搜索书架。");
    REQUIRE(i18n::get("terrain.nothing_of_interest", "There is nothing of interest.") == "没有值得关注的东西。");
    REQUIRE(i18n::get("terrain.alchemist_workbench_destroyed", "The alchemist's workbench is destroyed.") == "炼金术士的工作台被摧毁了。");
    REQUIRE(i18n::get("terrain.alchemist_workbench_burns_down", "The alchemist's workbench burns down.") == "炼金术士的工作台烧毁了。");
    REQUIRE(i18n::get("terrain.alchemist_workbench_here", "There is an alchemist's workbench here.") == "这里有一个炼金术士的工作台。");
    REQUIRE(i18n::get("terrain.alchemist_workbench_on_fire", "The alchemist's workbench is on fire.") == "炼金术士的工作台着火了。");
    REQUIRE(i18n::get("terrain.alchemist_workbench_empty", "The alchemist's workbench is empty.") == "炼金术士的工作台是空的。");
    REQUIRE(i18n::get("terrain.search_alchemist_workbench", "I search the alchemist's workbench.") == "我搜索炼金术士的工作台。");
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
    REQUIRE(i18n::get("marker.throwing_prefix", "Throwing ") == "正在投掷");
    REQUIRE(i18n::get("marker.period", ".") == "。");
    REQUIRE(i18n::get("marker.throw_prompt_suffix", "] to throw ") == "] 投掷：");
    REQUIRE(
        i18n::get(
            "marker.ctrl_tele.control_where",
            "I can control where I teleport.") == "我可以控制传送的位置。");
    REQUIRE(
        i18n::get(
            "marker.ctrl_tele.chance_success_suffix",
            "% chance of success.") == "% 成功率。");
    REQUIRE(
        i18n::get(
            "marker.ctrl_tele.try_teleport_here",
            "[enter] to try teleporting here") == "[enter] 尝试传送到这里");
    REQUIRE(i18n::get("marker.ctrl_tele.failed", "I failed to go there...") == "我没能去到那里……");
    REQUIRE(i18n::get("marker.control_object.jammed_suffix", " is jammed.") == "被堵住了。");
    REQUIRE(i18n::get("marker.control_object.jam_prefix", "(c) Jam ") == "(c)堵住");
    REQUIRE(i18n::get("marker.control_object.open_prefix", "(o) Open ") == "(o)打开");
    REQUIRE(i18n::get("marker.control_object.something", "Something") == "有什么东西");
    REQUIRE(i18n::get("marker.control_object.prevents_closing", " prevents closing ") == "阻止关闭");
    REQUIRE(i18n::get("marker.control_object.period", ".") == "。");
    REQUIRE(i18n::get("marker.control_object.close_prefix", "(c) Close ") == "(c)关闭");
    REQUIRE(i18n::get("marker.control_object.deactivated_suffix", " is deactivated.") == "被停用了。");
    REQUIRE(i18n::get("marker.control_object.deactivate_crystal", "(d) Deactivate crystal") == "(d)停用水晶");
    REQUIRE(i18n::get("marker.control_object.strike_prefix", "(w) Strike ") == "(w)攻击");
    REQUIRE(i18n::get("marker.control_object.destroy_prefix", "(w) Destroy ") == "(w)摧毁");
    REQUIRE(i18n::get("marker.control_object.nothing_happens", "Nothing happens.") == "什么也没发生。");
    REQUIRE(i18n::get("marker.control_object.select_object", "Select an object to control.") == "选择要控制的物体。");
    REQUIRE(i18n::get("marker.control_object.distance_prefix", "Distance: ") == "距离：");
    REQUIRE(i18n::get("marker.control_object.distance_separator", "/") == "/");
    REQUIRE(i18n::get("marker.control_object.control_prefix", "[enter] to control ") == "[enter] 控制");
    REQUIRE(i18n::get("marker.control_object.no_vision_here", "I have no vision here.") == "我在这里没有视野。");
    REQUIRE(i18n::get("marker.control_object.distance_too_small", "The distance is too small.") == "距离太近了。");
    REQUIRE(i18n::get("marker.control_object.distance_too_great", "The distance is too great.") == "距离太远了。");
    REQUIRE(i18n::get("marker.control_object.cannot_control_here", "I cannot control any object here.") == "我无法控制这里的任何物体。");
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
    require_translation("actor_data.smell.briny", "这里有一股咸腥味。");
    require_translation("actor_data.smell.death", "这里闻起来像死亡。");
    require_translation("actor_data.smell.decayed_flesh", "这里有一股腐烂血肉的恶臭。");
    require_translation("actor_data.smell.fetid_viscous_odor", "空气中弥漫着浓重黏腻的恶臭。");
    require_translation("actor_data.smell.filth_and_decrepitude", "这里闻起来满是污秽与腐朽。");
    require_translation("actor_data.smell.open_grave", "这里闻起来像敞开的坟墓。");
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
    REQUIRE(
        i18n::get(
            "actor_start_turn.encumbered",
            "I am carrying too much weight, walking will be slower.") == "负重过多，走路会更慢。");
    REQUIRE(i18n::get("actor_start_turn.spot_prefix", "I spot ") == "我发现了");
    REQUIRE(i18n::get("actor_start_turn.exclaim", "!") == "！");
    REQUIRE(i18n::get("actor_start_turn.in_view_suffix", " is in my view.") == "在我的视野中。");
    REQUIRE(
        i18n::get(
            "actor_start_turn.sanity_slipping",
            "I feel my sanity slipping...") == "我感觉理智正在流失...");
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
    require_translation("gods.abholos.descr", "雾中吞噬者");
    require_translation("gods.alala.descr", "S'glhuo 的先驱");
    require_translation("gods.ammutseba.descr", "群星吞噬者");
    require_translation("gods.aphoom_zhah.descr", "冷焰");
    require_translation("gods.apocolothoth.descr", "月神");
    require_translation("gods.atlach_nacha.descr", "蜘蛛之神");
    require_translation("gods.ayiig.descr", "蛇之女神");
    require_translation("gods.aylith.descr", "林中寡妇");
    require_translation("gods.baoht_zuqqa_mogg.descr", "瘟疫带来者");
    require_translation("gods.basatan.descr", "蟹群之主");
    require_translation("gods.bgnu_thun.descr", "寒魂冰神");
    require_translation("gods.bokrug.descr", "巨大水蜥");
    require_translation("gods.bugg_shash.descr", "黑暗者");
    require_translation("gods.cthaat.descr", "黑水之神");
    require_translation("gods.cthugha.descr", "活焰");
    require_translation("gods.cthylla.descr", "克苏鲁的秘密之女");
    require_translation("gods.ctoggha.descr", "梦魇魔");
    require_translation("gods.cyaegha.descr", "毁灭之眼");
    require_translation("gods.dygra.descr", "石之物");
    require_translation("gods.dythalla.descr", "蜥蜴之主");
    require_translation("gods.eihort.descr", "迷宫之神");
    require_translation("gods.ghisguth.descr", "深水之声");
    require_translation("gods.glaaki.descr", "死梦之主");
    require_translation("gods.gleeth.descr", "月之盲神");
    require_translation("gods.gloon.descr", "肉体腐化者");
    require_translation("gods.gog_hoor.descr", "以疯狂者为食者");
    require_translation("gods.gol_goroth.descr", "黑石之神");
    require_translation("gods.groth_golka.descr", "魔鸟之神");
    require_translation("gods.gurathnaka.descr", "梦境吞噬者");
    require_translation("gods.han.descr", "黑暗者");
    require_translation("gods.hastur.descr", "黄衣之王");
    require_translation("gods.hchtelegoth.descr", "伟大触手之神");
    require_translation("gods.hziulquoigmnzhah.descr", "Cykranosh 之神");
    require_translation("gods.inpesca.descr", "海中恐怖");
    require_translation("gods.iod.descr", "闪耀猎手");
    require_translation("gods.istasha.descr", "黑暗女主");
    require_translation("gods.ithaqua.descr", "冰冷白寂之神");
    require_translation("gods.janaingo.descr", "水门守卫");
    require_translation("gods.kaalut.descr", "贪食者");
    require_translation("gods.kassogtha.descr", "疾病利维坦");
    require_translation("gods.kaunuzoth.descr", "伟大者");
    require_translation("gods.lam.descr", "灰者");
    require_translation("gods.lythalia.descr", "森林女神");
    require_translation("gods.mnomquah.descr", "黑湖之主");
    require_translation("gods.mordiggian.descr", "伟大食尸鬼");
    require_translation("gods.mynoghra.descr", "阴影女魔");
    require_translation("gods.ngirrthlu.descr", "狼之物");
    require_translation("gods.northot.descr", "被遗忘之神");
    require_translation("gods.nssu_ghahnb.descr", "永世水蛭");
    require_translation("gods.nycrama.descr", "僵化精粹");
    require_translation("gods.nyogtha.descr", "红色深渊萦绕者");
    require_translation("gods.obmbu.descr", "粉碎者");
    require_translation("gods.othuum.descr", "海洋恐怖");
    require_translation("gods.othuyeg.descr", "厄运行者");
    require_translation("gods.psuchawrl.descr", "旧日长者");
    require_translation("gods.quyagen.descr", "栖于我们脚下者");
    require_translation("gods.rhan_tegoth.descr", "象牙王座之主");
    require_translation("gods.rlim_shaikorth.descr", "白色蠕虫");
    require_translation("gods.ruhtra_dyoll.descr", "火神");
    require_translation("gods.sebek.descr", "鳄鱼神");
    require_translation("gods.sedmelluq.descr", "伟大操纵者");
    require_translation("gods.sfatlicllp.descr", "堕落智慧");
    require_translation("gods.shaklatal.descr", "邪视之眼");
    require_translation("gods.sheb_teth.descr", "灵魂吞噬者");
    require_translation("gods.shterot.descr", "幽暗者");
    require_translation("gods.shudde_mell.descr", "地下掘行者");
    require_translation("gods.shuy_nihl.descr", "地中吞噬者");
    require_translation("gods.sthanee.descr", "迷失者");
    require_translation("gods.summanus.descr", "夜之君主");
    require_translation("gods.thanaroa.descr", "闪耀者");
    require_translation("gods.tharapithia.descr", "绯红光中的阴影");
    require_translation("gods.thog.descr", "Xuthal 魔神");
    require_translation("gods.thrygh.descr", "神兽");
    require_translation("gods.tsathoggua.descr", "蟾蜍之神");
    require_translation("gods.tulushuggua.descr", "地下水居者");
    require_translation("gods.vibur.descr", "天外之物");
    require_translation("gods.volgna_gath.descr", "秘密守护者");
    require_translation("gods.vthyarilops.descr", "海星之神");
    require_translation("gods.xalafu.descr", "恐惧者");
    require_translation("gods.xcthol.descr", "山羊神");
    require_translation("gods.xinlurgash.descr", "永恒吞噬者");
    require_translation("gods.xirdneth.descr", "幻象制造者");
    require_translation("gods.xotli.descr", "恐怖之主");
    require_translation("gods.yegg_ha.descr", "无面者");
    require_translation("gods.ygolonac.descr", "玷污者");
    require_translation("gods.yig.descr", "群蛇之父");
    require_translation("gods.ymnar.descr", "黑暗潜行者");
    require_translation("gods.yog_sapha.descr", "深处居者");
    require_translation("gods.yorith.descr", "最古老的梦者");
    require_translation("gods.ythogtha.descr", "坑中之物");
    require_translation("gods.yug_siturath.descr", "吞噬一切之雾");
    require_translation("gods.zathog.descr", "旋涡黑君");
    require_translation("gods.zindarak.descr", "炽焰信使");
    require_translation("gods.zushakon.descr", "黑暗沉默者");
    require_translation("gods.zvilpogghua.descr", "星辰盛宴者");
    require_translation("gods.gozer.descr", "毁灭者");
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
    REQUIRE(i18n::get("ai.bashes_at_the", " bashes at the ") == "正在撞击");
    REQUIRE(i18n::get("ai.exclaim", "!") == "！");
    REQUIRE(i18n::get("ai.looks_desperate_suffix", " looks desperate.") == "看起来绝望了。");
    REQUIRE(i18n::get("actor_move.through", "through") == "穿过");
    REQUIRE(i18n::get("actor_move.under", "under") == "从下方");
    REQUIRE(i18n::get("actor_move.it", "it") == "它");
    REQUIRE(i18n::get("actor_move.seeps_prefix", " seeps ") == "渗");
    REQUIRE(i18n::get("actor_move.space", " ") == "");
    REQUIRE(i18n::get("actor_move.squirms_through", " squirms through ") == "钻过");
    REQUIRE(i18n::get("attack.it", "it") == "它");
    REQUIRE(i18n::get("attack.attack_prefix", "Attack ") == "攻击");
    REQUIRE(i18n::get("attack.with", " with ") == "，使用");
    REQUIRE(i18n::get("attack.query_suffix", "?") == "？");
    REQUIRE(i18n::get("attack_melee.player_miss", "I miss.") == "我没打中。");
    REQUIRE(i18n::get("attack_melee.i_am_hit", "I am hit") == "我被击中了");
    REQUIRE(i18n::get("attack_melee.it_upper", "It") == "它");
    REQUIRE(i18n::get("attack_melee.me", "me") == "我");
    REQUIRE(i18n::get("attack_melee.it_lower", "it") == "它");
    REQUIRE(i18n::get("attack_melee.misses", " misses ") == "没打中");
    REQUIRE(i18n::get("attack_melee.period", ".") == "。");
    REQUIRE(i18n::get("attack_melee.weak_intrinsic_suffix", " feebly") == "无力地");
    REQUIRE(i18n::get("attack_melee.player_prefix", "I ") == "我");
    REQUIRE(i18n::get("attack_melee.word_separator", " ") == "");
    REQUIRE(i18n::get("attack_melee.weak_prefix", "feebly ") == "无力地");
    REQUIRE(i18n::get("attack_melee.backstab_prefix", "covertly ") == "隐秘地");
    REQUIRE(i18n::get("attack_melee.with_prefix", "with ") == "，使用");
    REQUIRE(i18n::get("attack_melee.with_spaced", " with ") == "，使用");
    REQUIRE(i18n::get("attack_melee.is_hit_suffix", " is hit") == "被击中了");
    REQUIRE(i18n::get("attack_melee.hear_fighting", "I hear fighting.") == "我听到打斗声。");
    REQUIRE(i18n::get("attack_melee.attack_terrain_prefix", "Attacking ") == "攻击");
    REQUIRE(i18n::get("attack_melee.attack_terrain_with", " with ") == "，使用");
    REQUIRE(
        i18n::get("attack_melee.attack_terrain_suffix", " would be useless.") ==
        "会毫无用处。");
    REQUIRE(
        i18n::get("attack_melee.stopped_at_boundary_suffix", " is stopped at the boundary.") ==
        "被挡在边界处。");
    require_translation("attack_ranged.it", "它");
    REQUIRE(i18n::get("property.wounded_open", "Wounded(") == "受伤（");
    REQUIRE(i18n::get("property.close_paren", ")") == "）");
    require_translation("property_handler.ending_suffix", "（即将结束）");
    require_translation("property_handler.indefinite_suffix", "（无限期）");
    require_translation("property_handler.from_item_suffix", "（来自物品）");
    REQUIRE(i18n::get("property.rises_again_suffix", " rises again!!") == "再次站了起来！！");
    REQUIRE(i18n::get("property.crimson_passage_infinite", "INF") == "无限");
    REQUIRE(i18n::get("property.crimson_passage_short_open", "Crims Psg(") == "猩红通道（");
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
    REQUIRE(i18n::get("property.zombie_part_hand_prefix", "The hand of ") == "");
    REQUIRE(
        i18n::get(
            "property.zombie_part_hand_suffix",
            " comes off and starts crawling around!") == "的一只手脱落下来，开始四处爬行！");
    REQUIRE(i18n::get("property.zombie_part_intestines_prefix", "The intestines of ") == "");
    REQUIRE(
        i18n::get(
            "property.zombie_part_intestines_suffix",
            " starts crawling around!") == "的肠子开始四处爬行！");
    REQUIRE(i18n::get("property.zombie_part_head_prefix", "The head of ") == "");
    REQUIRE(
        i18n::get(
            "property.zombie_part_head_suffix",
            " starts floating around!") == "的头颅开始四处漂浮！");
    REQUIRE(i18n::get("property.spews_ooze_suffix", " spews ooze.") == "喷出了软泥。");
    REQUIRE(i18n::get("property.bewilders_me_suffix", " bewilders me.") == "使我困惑。");
    REQUIRE(i18n::get("property.is_taunting_me_suffix", " is taunting me!") == "正在嘲弄我！");
    REQUIRE(i18n::get("property.collapses_suffix", " collapses!") == "倒塌了！");
    REQUIRE(i18n::get("property.thorns_it", "it") == "它");
    REQUIRE(i18n::get("property.thorns_it_upper", "It") == "它");
    REQUIRE(i18n::get("property.thorns_player_retaliate_prefix", "I retaliate upon ") == "我反击了");
    REQUIRE(i18n::get("property.exclaim", "!") == "！");
    REQUIRE(i18n::get("property.thorns_mon_retaliate_player_prefix", "My attack upon ") == "我对");
    REQUIRE(
        i18n::get(
            "property.thorns_mon_retaliate_player_suffix",
            " is retaliated by a magic aura!") == "的攻击被一道魔法灵光反击了！");
    REQUIRE(i18n::get("property.thorns_mon_retaliate_mon_middle", "retaliates upon ") == "以魔法灵光反击了");
    REQUIRE(i18n::get("property.thorns_mon_retaliate_mon_suffix", " by a magic aura!") == "！");
    REQUIRE(i18n::get("property.tomb_legions_sound", "A voice is calling forth Tomb-Legions!") == "有个声音正在召唤坟墓军团！");
    REQUIRE(i18n::get("property.their", "their") == "其");
    REQUIRE(i18n::get("property.its", "its") == "它的");
    REQUIRE(i18n::get("property.recognizes_me_as_middle", " recognizes me as ") == "承认我是");
    REQUIRE(i18n::get("property.leader_suffix", " leader.") == "领袖。");
    REQUIRE(i18n::get("property.great_frenzy_sound", "A voice is stirring up a great frenzy!") == "有个声音正在煽动巨大的狂乱！");
    REQUIRE(i18n::get("property.stirs_up_great_frenzy_suffix", " stirs up a great frenzy!") == "激起了巨大的狂乱！");
    REQUIRE(i18n::get("property.calls_plague_of_locusts_suffix", " calls a plague of Locusts!") == "召唤了一场蝗灾！");
    require_translation("property_data.r_phys.name", "物理抗性");
    require_translation("property_data.r_phys.name_short", "物抗");
    require_translation("property_data.r_phys.descr", "不会受到物理攻击伤害。");
    require_translation("property_data.r_phys.msg_start_player", "我感觉物理攻击伤不到我。");
    require_translation("property_data.r_phys.msg_start_mon", "{}看起来坚如钢铁。");
    require_translation("property_data.r_phys.msg_end_player", "我感觉容易受到物理攻击伤害。");
    require_translation("property_data.r_phys.msg_end_mon", "{}看起来没那么坚韧了。");
    require_translation("property_data.r_fire.name", "火焰抗性");
    require_translation("property_data.r_fire.name_short", "火抗");
    require_translation("property_data.r_fire.descr", "不会受到火焰伤害。");
    require_translation("property_data.r_fire.msg_start_player", "我感到对火焰有抗性。");
    require_translation("property_data.r_fire.msg_start_mon", "{}对火焰有抗性。");
    require_translation("property_data.r_fire.msg_end_player", "我感觉容易受到火焰伤害。");
    require_translation("property_data.r_fire.msg_end_mon", "{}容易受到火焰伤害。");
    require_translation("property_data.r_poison.name", "毒素抗性");
    require_translation("property_data.r_poison.name_short", "毒抗");
    require_translation("property_data.r_poison.descr", "不会受到毒素伤害。");
    require_translation("property_data.r_poison.msg_start_player", "我感到对毒素有抗性。");
    require_translation("property_data.r_poison.msg_start_mon", "{}对毒素有抗性。");
    require_translation("property_data.r_poison.msg_end_player", "我感觉容易受到毒素伤害。");
    require_translation("property_data.r_poison.msg_end_mon", "{}容易受到毒素伤害。");
    require_translation("property_data.r_elec.name", "电击抗性");
    require_translation("property_data.r_elec.name_short", "电抗");
    require_translation("property_data.r_elec.descr", "不会受到电击伤害。");
    require_translation("property_data.r_elec.msg_start_player", "我感到对电击有抗性。");
    require_translation("property_data.r_elec.msg_start_mon", "{}对电击有抗性。");
    require_translation("property_data.r_elec.msg_end_player", "我感觉容易受到电击伤害。");
    require_translation("property_data.r_elec.msg_end_mon", "{}容易受到电击伤害。");
    require_translation("property_data.r_sleep.name", "睡眠抗性");
    require_translation("property_data.r_sleep.name_short", "睡抗");
    require_translation("property_data.r_sleep.descr", "不会昏厥或被催眠。");
    require_translation("property_data.r_sleep.msg_start_player", "我感到十分清醒。");
    require_translation("property_data.r_sleep.msg_start_mon", "{}十分清醒。");
    require_translation("property_data.r_sleep.msg_end_player", "我感觉没那么清醒了。");
    require_translation("property_data.r_sleep.msg_end_mon", "{}没那么清醒了。");
    require_translation("property_data.r_fear.name", "恐惧抗性");
    require_translation("property_data.r_fear.name_short", "恐抗");
    require_translation("property_data.r_fear.descr", "不受恐惧影响。");
    require_translation("property_data.r_fear.msg_start_player", "我不会被恐惧动摇。");
    require_translation("property_data.r_fear.msg_start_mon", "{}对恐惧有抗性。");
    require_translation("property_data.r_fear.msg_end_player", "我感觉容易受到恐惧影响。");
    require_translation("property_data.r_fear.msg_end_mon", "{}容易受到恐惧影响。");
    require_translation("property_data.r_slow.name", "减速抗性");
    require_translation("property_data.r_slow.name_short", "缓抗");
    require_translation("property_data.r_slow.descr", "不会被魔法减速。");
    require_translation("property_data.r_slow.msg_start_player", "我感到坚定不移。");
    require_translation("property_data.r_slow.msg_end_player", "我感觉更容易受时间影响。");
    require_translation("property_data.r_conf.name", "混乱抗性");
    require_translation("property_data.r_conf.name_short", "乱抗");
    require_translation("property_data.r_conf.descr", "不会陷入混乱。");
    require_translation("property_data.r_conf.msg_start_player", "我感到对混乱有抗性。");
    require_translation("property_data.r_conf.msg_start_mon", "{}对混乱有抗性。");
    require_translation("property_data.r_conf.msg_end_player", "我感觉容易陷入混乱。");
    require_translation("property_data.r_conf.msg_end_mon", "{}容易陷入混乱。");
    require_translation("property_data.r_disease.name", "疾病抗性");
    require_translation("property_data.r_disease.name_short", "病抗");
    require_translation("property_data.r_disease.descr", "不会染上疾病。");
    require_translation("property_data.r_disease.msg_start_player", "我感到对疾病有抗性。");
    require_translation("property_data.r_disease.msg_start_mon", "{}对疾病有抗性。");
    require_translation("property_data.r_disease.msg_end_player", "我感觉容易染上疾病。");
    require_translation("property_data.r_disease.msg_end_mon", "{}容易染上疾病。");
    require_translation("property_data.r_blind.name", "失明抗性");
    require_translation("property_data.r_blind.name_short", "盲抗");
    require_translation("property_data.r_blind.descr", "不会失明。");
    require_translation("property_data.r_para.name", "麻痹抗性");
    require_translation("property_data.r_para.name_short", "麻抗");
    require_translation("property_data.r_para.descr", "不会被麻痹。");
    require_translation("property_data.r_para.msg_start_player", "我感到更加稳定。");
    require_translation("property_data.r_para.msg_start_mon", "{}看起来更加稳定。");
    require_translation("property_data.r_para.msg_end_player", "我感觉没那么稳定了。");
    require_translation("property_data.r_para.msg_end_mon", "{}看起来没那么稳定了。");
    require_translation("property_data.r_breath.descr", "不会受到呼吸受阻伤害。");
    require_translation("property_data.r_breath.msg_start_player", "我可以安全呼吸。");
    require_translation("property_data.r_breath.msg_start_mon", "{}可以安全呼吸。");
    require_translation("property_data.r_spell.name", "法术抗性");
    require_translation("property_data.r_spell.name_short", "法抗");
    require_translation("property_data.r_spell.descr", "不会受到有害法术影响。");
    require_translation("property_data.r_spell.msg_start_player", "我能抵御有害法术！");
    require_translation("property_data.r_spell.msg_start_mon", "{}正在抵御有害法术。");
    require_translation("property_data.r_spell.msg_end_player", "我感觉容易受到法术影响。");
    require_translation("property_data.r_spell.msg_end_mon", "{}容易受到法术影响。");
    require_translation("property_data.r_shock.name", "震惊抗性");
    require_translation("property_data.r_shock.name_short", "震抗");
    require_translation("property_data.r_shock.descr", "不受震惊事件影响。");
    require_translation("property_data.r_shock.msg_start_player", "没有什么能扰乱我的心智！");
    require_translation("property_data.r_shock.msg_end_player", "我感觉又容易受到此地恐怖的影响了。");
    require_translation("property_data.light_sensitive.name", "光敏");
    require_translation("property_data.light_sensitive.name_short", "光敏");
    require_translation("property_data.light_sensitive.descr", "对光线脆弱。");
    require_translation("property_data.light_sensitive.msg_start_player", "我感觉容易受到光线伤害！");
    require_translation("property_data.light_sensitive.msg_start_mon", "{}对光线很脆弱。");
    require_translation("property_data.light_sensitive.msg_end_player", "我不再感觉容易受到光线伤害。");
    require_translation("property_data.light_sensitive.msg_end_mon", "{}不再对光线脆弱。");
    require_translation("property_data.blind.name", "失明");
    require_translation("property_data.blind.name_short", "失明");
    require_translation("property_data.blind.descr", "无法看见，命中率-20%，闪避攻击几率-50%。");
    require_translation("property_data.blind.msg_start_player", "我失明了！");
    require_translation("property_data.blind.msg_start_mon", "{}失明了。");
    require_translation("property_data.blind.msg_end_player", "我又能看见了！");
    require_translation("property_data.blind.msg_end_mon", "{}又能看见了。");
    require_translation("property_data.blind.historic_msg_start_permanent", "永久失明");
    require_translation("property_data.blind.historic_msg_end_permanent", "我的视力恢复了");
    require_translation("property_data.deaf.name", "耳聋");
    require_translation("property_data.deaf.name_short", "耳聋");
    require_translation("property_data.deaf.descr", "无法听见声音。");
    require_translation("property_data.deaf.msg_start_player", "我耳聋了！");
    require_translation("property_data.deaf.msg_start_mon", "{}不再对任何噪声作出反应。");
    require_translation("property_data.deaf.msg_end_player", "我又能听见了。");
    require_translation("property_data.deaf.msg_end_mon", "{}又开始对噪声作出反应。");
    require_translation("property_data.deaf.historic_msg_start_permanent", "永久失聪");
    require_translation("property_data.deaf.historic_msg_end_permanent", "我的听力恢复了");
    require_translation("property_data.fainted.name", "昏厥");
    require_translation("property_data.fainted.name_short", "昏厥");
    require_translation("property_data.fainted.descr", "暂时失去意识，受到任何伤害或经过足够时间后会醒来。");
    require_translation("property_data.fainted.msg_start_player", "我昏厥了！");
    require_translation("property_data.fainted.msg_start_mon", "{}昏厥了。");
    require_translation("property_data.fainted.msg_end_player", "我醒了。");
    require_translation("property_data.fainted.msg_end_mon", "{}醒了。");
    require_translation("property_data.fainted.msg_res_player", "我抵抗了昏厥。");
    require_translation("property_data.fainted.msg_res_mon", "{}抵抗了昏厥。");
    require_translation("property_data.burning.name", "燃烧");
    require_translation("property_data.burning.name_short", "燃烧");
    require_translation("property_data.burning.descr", "每回合受到伤害，尝试阅读或施法时有50%几率失败。");
    require_translation("property_data.burning.msg_start_player", "我着火了！");
    require_translation("property_data.burning.msg_start_mon", "{}正在燃烧。");
    require_translation("property_data.burning.msg_end_player", "火焰被扑灭了。");
    require_translation("property_data.burning.msg_end_mon", "{}不再燃烧。");
    require_translation("property_data.burning.msg_res_player", "我抵抗了燃烧。");
    require_translation("property_data.burning.msg_res_mon", "{}抵抗了燃烧。");
    require_translation("property_data.poisoned.name", "中毒");
    require_translation("property_data.poisoned.name_short", "中毒");
    require_translation(
        "property_data.poisoned.descr",
        "生命高于最大生命值50%时会持续受到伤害，生命不会自然恢复，近战伤害-25%，近战命中率-10%。再次中毒会叠加持续时间。");
    require_translation("property_data.poisoned.msg_start_player", "我中毒了！");
    require_translation("property_data.poisoned.msg_start_mon", "{}中毒了。");
    require_translation("property_data.poisoned.msg_end_player", "我体内的毒素被清除了！");
    require_translation("property_data.poisoned.msg_end_mon", "{}体内的毒素被清除了。");
    require_translation("property_data.poisoned.msg_res_player", "我抵抗了中毒。");
    require_translation("property_data.poisoned.msg_res_mon", "{}抵抗了中毒。");
    require_translation("property_data.paralyzed.name", "麻痹");
    require_translation("property_data.paralyzed.name_short", "麻痹");
    require_translation("property_data.paralyzed.descr", "无法移动。");
    require_translation("property_data.paralyzed.msg_start_player", "我麻痹了！");
    require_translation("property_data.paralyzed.msg_start_mon", "{}麻痹了。");
    require_translation("property_data.paralyzed.msg_end_player", "我又能动了！");
    require_translation("property_data.paralyzed.msg_end_mon", "{}又能动了。");
    require_translation("property_data.paralyzed.msg_res_player", "我抵抗了麻痹。");
    require_translation("property_data.paralyzed.msg_res_mon", "{}抵抗了麻痹。");
    require_translation("property_data.terrified.name", "惊恐");
    require_translation("property_data.terrified.name_short", "惊恐");
    require_translation("property_data.terrified.descr", "无法进行近战攻击，远程命中率-20%，闪避攻击几率+20%。");
    require_translation("property_data.terrified.msg_start_player", "我惊恐万分！");
    require_translation("property_data.terrified.msg_start_mon", "{}看起来惊恐万分。");
    require_translation("property_data.terrified.msg_end_player", "我不再惊恐了！");
    require_translation("property_data.terrified.msg_end_mon", "{}不再惊恐了。");
    require_translation("property_data.terrified.msg_res_player", "我抵抗了恐惧。");
    require_translation("property_data.terrified.msg_res_mon", "{}抵抗了恐惧。");
    require_translation("property_data.confused.name", "混乱");
    require_translation("property_data.confused.name_short", "混乱");
    require_translation("property_data.confused.descr", "有时会向随机方向移动，无法阅读或施法，无法搜索隐藏门或陷阱。");
    require_translation("property_data.confused.msg_start_player", "我混乱了！");
    require_translation("property_data.confused.msg_start_mon", "{}看起来很混乱。");
    require_translation("property_data.confused.msg_end_player", "我恢复了清醒。");
    require_translation("property_data.confused.msg_end_mon", "{}不再混乱。");
    require_translation("property_data.confused.msg_res_player", "我设法保持了清醒。");
    require_translation("property_data.confused.msg_res_mon", "{}抵抗了混乱。");
    require_translation("property_data.hallucinating.name", "幻觉");
    require_translation("property_data.hallucinating.name_short", "幻觉");
    require_translation("property_data.hallucinating.descr", "感官并不总是可信。");
    require_translation("property_data.hallucinating.msg_start_player", "我开始怀疑自己的感官。");
    require_translation("property_data.hallucinating.msg_end_player", "我对自己的感官更有把握了。");
    require_translation("property_data.hallucinating.msg_res_player", "我设法抓住了现实。");
    require_translation("property_data.stunned.name", "眩晕");
    require_translation("property_data.stunned.name_short", "眩晕");
    require_translation("property_data.stunned.msg_start_player", "我被击晕了！");
    require_translation("property_data.stunned.msg_start_mon", "{}被击晕了。");
    require_translation("property_data.stunned.msg_end_player", "我不再眩晕了。");
    require_translation("property_data.stunned.msg_end_mon", "{}不再眩晕了。");
    require_translation("property_data.stunned.msg_res_player", "我抵抗了眩晕。");
    require_translation("property_data.stunned.msg_res_mon", "{}抵抗了眩晕。");
    require_translation("property_data.slowed.name", "减速");
    require_translation("property_data.slowed.name_short", "减速");
    require_translation("property_data.slowed.descr", "行动变慢。");
    require_translation("property_data.slowed.msg_start_player", "我周围的一切似乎加速了。");
    require_translation("property_data.slowed.msg_start_mon", "{}慢了下来。");
    require_translation("property_data.slowed.msg_end_player", "我周围的一切似乎慢了下来。");
    require_translation("property_data.slowed.msg_end_mon", "{}加速了。");
    require_translation("property_data.slowed.msg_res_player", "我抵抗了减速。");
    require_translation("property_data.slowed.msg_res_mon", "{}抵抗了减速。");
    require_translation("property_data.slowed.historic_msg_start_permanent", "永久减速");
    require_translation("property_data.slowed.historic_msg_end_permanent", "我的迟缓结束了");
    require_translation("property_data.hasted.name", "加速");
    require_translation("property_data.hasted.name_short", "加速");
    require_translation("property_data.hasted.descr", "行动变快。");
    require_translation("property_data.hasted.msg_start_player", "我周围的一切似乎慢了下来。");
    require_translation("property_data.hasted.msg_start_mon", "{}加速了。");
    require_translation("property_data.hasted.msg_end_player", "我周围的一切似乎加速了。");
    require_translation("property_data.hasted.msg_end_mon", "{}慢了下来。");
    require_translation("property_data.hasted.historic_msg_start_permanent", "永久加速");
    require_translation("property_data.hasted.historic_msg_end_permanent", "我的加速结束了");
    require_translation("property_data.extra_hasted.name", "极度加速");
    require_translation("property_data.extra_hasted.name_short", "极快");
    require_translation("property_data.extra_hasted.descr", "行动非常快。");
    require_translation("property_data.extra_hasted.msg_start_player", "我周围的一切突然显得静止。");
    require_translation("property_data.extra_hasted.msg_end_player", "我周围的一切似乎加快了很多。");
    require_translation("property_data.summoned.name", "召唤物");
    require_translation("property_data.summoned.descr", "被魔法召唤到这里。");
    require_translation("property_data.summoned.msg_end_mon", "{}突然消失了。");
    require_translation("property_data.nailed.name", "钉住");
    require_translation("property_data.nailed.descr", "被尖刺固定。拔出来会相当痛苦。");
    require_translation("property_data.nailed.msg_start_player", "我被尖刺固定住了！");
    require_translation("property_data.nailed.msg_start_mon", "{}被尖刺固定住了。");
    require_translation("property_data.nailed.msg_end_player", "我挣脱了！");
    require_translation("property_data.nailed.msg_end_mon", "{}挣脱了！");
    require_translation("property_data.wound.name", "受伤");
    require_translation(
        "property_data.wound.descr",
        "每道伤口：近战命中率-5%，闪避攻击几率-5%，生命值-10%，并降低生命恢复速度。三道并发伤口会让行走花费额外回合。五道并发伤口会致命。");
    require_translation("property_data.wound.msg_start_player", "我受伤了！");
    require_translation("property_data.wound.msg_res_player", "我抵抗了受伤！");
    require_translation("property_data.infected.name", "感染");
    require_translation("property_data.infected.name_short", "感染");
    require_translation("property_data.infected.descr", "一种讨厌的感染，若不治疗会变得更糟。");
    require_translation("property_data.infected.msg_start_player", "我感染了！");
    require_translation("property_data.infected.msg_start_mon", "{}感染了。");
    require_translation("property_data.infected.msg_end_player", "我的感染被治愈了！");
    require_translation("property_data.infected.msg_end_mon", "{}不再感染。");
    require_translation("property_data.diseased.name", "患病");
    require_translation("property_data.diseased.name_short", "患病");
    require_translation("property_data.diseased.descr", "最大生命值-50%。");
    require_translation("property_data.diseased.msg_start_player", "我患病了！");
    require_translation("property_data.diseased.msg_start_mon", "{}患病了。");
    require_translation("property_data.diseased.msg_end_player", "我的疾病被治愈了！");
    require_translation("property_data.diseased.msg_end_mon", "{}不再患病。");
    require_translation("property_data.diseased.msg_res_player", "我抵抗了疾病。");
    require_translation("property_data.diseased.msg_res_mon", "{}抵抗了疾病。");
    require_translation("property_data.diseased.historic_msg_start_permanent", "染上可怕的疾病");
    require_translation("property_data.diseased.historic_msg_end_permanent", "可怕的疾病被治愈");
    require_translation("property_data.weakened.name", "虚弱");
    require_translation("property_data.weakened.name_short", "虚弱");
    require_translation("property_data.weakened.descr", "近战伤害-50%，无法背刺，无法撞开门或箱子、撞倒重物等。");
    require_translation("property_data.weakened.msg_start_player", "我感到虚弱。");
    require_translation("property_data.weakened.msg_start_mon", "{}看起来虚弱了。");
    require_translation("property_data.weakened.msg_end_player", "我感觉更强壮了！");
    require_translation("property_data.weakened.msg_end_mon", "{}看起来更强壮了！");
    require_translation("property_data.weakened.msg_res_player", "我抵抗了虚弱。");
    require_translation("property_data.weakened.msg_res_mon", "{}抵抗了虚弱。");
    require_translation("property_data.frenzied.name", "狂暴");
    require_translation("property_data.frenzied.name_short", "狂暴");
    require_translation(
        "property_data.frenzied.descr",
        "无法远离看见的敌人，行动更快，近战伤害+1，近战命中率+10%，免疫混乱、昏厥、恐惧和虚弱，无法阅读或施法，狂暴结束时会变得虚弱。");
    require_translation("property_data.frenzied.msg_start_player", "我感到凶猛无比！！！");
    require_translation("property_data.frenzied.msg_start_mon", "{}看起来凶猛无比！");
    require_translation("property_data.frenzied.msg_end_player", "我冷静下来了。");
    require_translation("property_data.frenzied.msg_end_mon", "{}稍稍冷静下来。");
    require_translation("property_data.blessed.name", "祝福");
    require_translation("property_data.blessed.name_short", "祝福");
    require_translation("property_data.blessed.descr", "命中率、闪避、潜行和搜索+10%。");
    require_translation("property_data.blessed.msg_start_player", "我感觉更幸运了。");
    require_translation("property_data.blessed.msg_end_player", "我的运气恢复正常。");
    require_translation("property_data.blessed.historic_msg_start_permanent", "获得永恒祝福");
    require_translation("property_data.blessed.historic_msg_end_permanent", "我的伟大祝福结束了");
    require_translation("property_data.cursed.name", "诅咒");
    require_translation("property_data.cursed.name_short", "诅咒");
    require_translation("property_data.cursed.descr", "命中率、闪避、潜行和搜索-10%。");
    require_translation("property_data.cursed.msg_start_player", "我感到不幸。");
    require_translation("property_data.cursed.msg_start_mon", "{}被诅咒了。");
    require_translation("property_data.cursed.msg_end_player", "我感到更幸运了。");
    require_translation("property_data.cursed.msg_end_mon", "{}不再被诅咒。");
    require_translation("property_data.cursed.msg_res_player", "我抵抗了厄运。");
    require_translation("property_data.cursed.msg_res_mon", "{}抵抗了厄运。");
    require_translation("property_data.cursed.historic_msg_start_permanent", "永久诅咒降临到我身上");
    require_translation("property_data.cursed.historic_msg_end_permanent", "可怕的诅咒从我身上解除");
    require_translation("property_data.doomed.name", "厄运缠身");
    require_translation("property_data.doomed.name_short", "厄运");
    require_translation("property_data.doomed.descr", "命中率、闪避、潜行和搜索-20%，尝试阅读或施法时有10%几率失败。");
    require_translation("property_data.doomed.msg_start_player", "我感到厄运临头！");
    require_translation("property_data.doomed.msg_start_mon", "{}被厄运笼罩！");
    require_translation("property_data.doomed.msg_end_player", "我的厄运似乎不再那么确定了。");
    require_translation("property_data.doomed.msg_end_mon", "{}不再被厄运笼罩。");
    require_translation("property_data.doomed.msg_res_player", "我抵抗了一场大厄运。");
    require_translation("property_data.doomed.historic_msg_start_permanent", "我的厄运已被写定");
    require_translation("property_data.doomed.historic_msg_end_permanent", "希望再度归来");
    require_translation("property_data.extra_skill.name", "技艺提升");
    require_translation("property_data.extra_skill.name_short", "技能");
    require_translation("property_data.extra_skill.descr", "命中率、闪避、潜行和搜索+10%。");
    require_translation("property_data.extra_skill.msg_start_player", "我感觉更有技巧了。");
    require_translation("property_data.extra_skill.msg_start_mon", "{}看起来更有技巧了。");
    require_translation("property_data.extra_skill.msg_end_player", "我感觉没那么有技巧了。");
    require_translation("property_data.extra_skill.msg_end_mon", "{}看起来没那么有技巧了。");
    require_translation("property_data.magic_carapace.name", "甲壳");
    require_translation("property_data.magic_carapace.name_short", "甲壳");
    require_translation("property_data.magic_carapace.descr", "护甲点+3，抵抗燃烧几率+25%。");
    require_translation("property_data.magic_carapace.msg_start_player", "一层保护性甲壳在我周围形成。");
    require_translation("property_data.magic_carapace.msg_start_mon", "{}被一层保护性甲壳包裹。");
    require_translation("property_data.magic_carapace.msg_end_player", "我的甲壳开裂并崩碎。");
    require_translation("property_data.magic_carapace.msg_end_mon", "{}脱落了一层甲壳。");
    require_translation("property_data.premonition.name", "预知");
    require_translation("property_data.premonition.name_short", "预知");
    require_translation("property_data.premonition.descr", "闪避攻击几率+75%。");
    require_translation("property_data.premonition.msg_start_player", "我感到不可侵犯。");
    require_translation("property_data.premonition.msg_start_mon", "{}看起来不可侵犯。");
    require_translation("property_data.premonition.msg_end_player", "我感觉更脆弱了。");
    require_translation("property_data.premonition.msg_end_mon", "{}现在看起来更容易命中。");
    require_translation("property_data.erudition.name", "博学");
    require_translation("property_data.erudition.name_short", "博学");
    require_translation("property_data.erudition.descr", "法术技能提升一级。");
    require_translation("property_data.erudition.msg_start_player", "秘法奥秘向我揭示！");
    require_translation("property_data.erudition.msg_end_player", "我感到无知。");
    require_translation("property_data.clairvoyance.name", "灵视");
    require_translation("property_data.clairvoyance.name_short", "灵视");
    require_translation("property_data.clairvoyance.descr", "魔法揭示周围区域中的事物。");
    require_translation("property_data.clairvoyance.msg_start_player", "隐藏的秘密向我揭示。");
    require_translation("property_data.clairvoyance.msg_end_player", "我不再能看见隐藏之物。");
    require_translation("property_data.descend.name", "下降中");
    require_translation("property_data.descend.name_short", "下降");
    require_translation("property_data.descend.descr", "很快会移动到更深层。");
    require_translation("property_data.descend.msg_start_player", "我感到一阵下沉感。");
    require_translation("property_data.entangled.name", "缠绕");
    require_translation("property_data.entangled.name_short", "缠绕");
    require_translation("property_data.entangled.descr", "被某物缠住。");
    require_translation("property_data.entangled.msg_start_player", "我被缠住了！");
    require_translation("property_data.entangled.msg_start_mon", "{}被缠住了。");
    require_translation("property_data.entangled.msg_end_player", "我挣脱了！");
    require_translation("property_data.entangled.msg_end_mon", "{}挣脱了！");
    require_translation("property_data.stuck.name", "卡住");
    require_translation("property_data.stuck.name_short", "卡住");
    require_translation("property_data.stuck.descr", "被某物卡住。");
    require_translation("property_data.stuck.msg_start_player", "我卡住了！");
    require_translation("property_data.stuck.msg_start_mon", "{}卡住了。");
    require_translation("property_data.stuck.msg_end_player", "我把自己拔了出来！");
    require_translation("property_data.stuck.msg_end_mon", "{}拔脱了！");
    require_translation("property_data.radiant_fov.name", "光辉");
    require_translation("property_data.radiant_fov.name_short", "光辉");
    require_translation("property_data.radiant_fov.descr", "发出明亮光芒。");
    require_translation("property_data.radiant_fov.msg_start_player", "明亮光芒照耀在我周围。");
    require_translation("property_data.radiant_fov.msg_end_player", "周围突然变暗了。");
    require_translation("property_data.invis.name", "隐形");
    require_translation("property_data.invis.name_short", "隐形");
    require_translation("property_data.invis.descr", "无法被普通视线发现。");
    require_translation("property_data.invis.msg_start_player", "我离开了视线！");
    require_translation("property_data.invis.msg_start_mon", "{}离开了视线！");
    require_translation("property_data.invis.msg_end_player", "我不再隐形。");
    require_translation("property_data.cloaked.name", "隐匿");
    require_translation("property_data.cloaked.name_short", "隐匿");
    require_translation("property_data.cloaked.descr", "无法被普通视线发现，攻击或施法时结束。");
    require_translation("property_data.cloaked.msg_start_player", "我隐匿起来！");
    require_translation("property_data.cloaked.msg_start_mon", "{}隐匿起来！");
    require_translation("property_data.cloaked.msg_end_player", "我的隐匿消退了。");
    require_translation("property_data.see_invis.name", "看见隐形");
    require_translation("property_data.see_invis.name_short", "见隐");
    require_translation("property_data.see_invis.descr", "可以看见隐形生物，且不会失明。");
    require_translation("property_data.see_invis.msg_start_player", "我的眼睛能感知隐形之物。");
    require_translation("property_data.see_invis.msg_start_mon", "{}似乎非常敏锐。");
    require_translation("property_data.see_invis.msg_end_player", "我的眼睛不再能感知隐形之物。");
    require_translation("property_data.see_invis.msg_end_mon", "{}似乎没那么敏锐了。");
    require_translation("property_data.astral_opium_addiction.name", "星界鸦片成瘾");
    require_translation("property_data.astral_opium_addiction.name_short", "成瘾");
    require_translation(
        "property_data.astral_opium_addiction.descr",
        "对星界鸦片成瘾——如果不再使用星界鸦片，成瘾最终会停止，但戒断很快会引起症状（最低震惊值增加）。这种成瘾过于强大且超凡，坚毅药水无法治愈。");
    require_translation("property_data.astral_opium_addiction.msg_start_player", "那感觉太美妙了！");
    require_translation("property_data.astral_opium_addiction.msg_end_player", "我突然意识到自己不再渴望星界鸦片了。");
    require_translation("property_data.meditative_focused.name", "专注");
    require_translation("property_data.meditative_focused.name_short", "专注");
    require_translation("property_data.meditative_focused.descr", "下一个法术会在不花费回合的情况下施放，并且消耗降低。");
    require_translation("property_data.meditative_focused.msg_start_player", "我感到非常专注。");
    require_translation("property_data.meditative_focused.msg_end_player", "我没那么专注了。");
    require_translation("property_data.thorns.name", "荆棘");
    require_translation("property_data.thorns.name_short", "荆棘");
    require_translation("property_data.thorns.descr", "每当施法者受到近战攻击、远程攻击或伤害性法术伤害时，攻击者都会被一股不可抗拒的力量击中。");
    require_translation("property_data.thorns.msg_start_player", "报复已被封印！");
    require_translation("property_data.thorns.msg_end_player", "我停止返还伤害。");
    require_translation("property_data.crimson_passage.name", "猩红通道");
    require_translation("property_data.crimson_passage.name_short", "猩红道");
    require_translation(
        "property_data.crimson_passage.descr",
        "行走不花费时间，但每迈出一步都会消耗2点生命值（原地等待仍可正常进行）。效果会在行走一定步数后结束，或在没有足够生命值可供消耗时结束。");
    require_translation("property_data.crimson_passage.msg_start_player", "一条可怖的道路在前方展开。");
    require_translation("property_data.crimson_passage.msg_end_player", "我的移动恢复正常。");
    require_translation("property_data.sanctuary.name", "庇护");
    require_translation("property_data.sanctuary.name_short", "庇护");
    require_translation("property_data.sanctuary.descr", "所有敌对生物都会忽略它。移动或进行近战/远程攻击时效果结束。");
    require_translation("property_data.sanctuary.msg_start_player", "我感到非常安全。");
    require_translation("property_data.sanctuary.msg_end_player", "我感觉安全感大幅减弱。");
    require_translation("property_data.moribund.name", "垂死");
    require_translation("property_data.moribund.name_short", "垂死");
    require_translation("property_data.moribund.descr", "近战伤害+3，近战命中率+30%，护甲点+3。");
    require_translation("property_data.temporal_echo.name", "时间回声");
    require_translation("property_data.temporal_echo.name_short", "回声");
    require_translation("property_data.temporal_echo.descr", "受到的所有伤害会在效果结束时再次结算。");
    require_translation("property_data.temporal_echo.msg_start_player", "我被时间回声标记了。");
    require_translation("property_data.temporal_echo.msg_start_mon", "{}被时间回声标记了。");
    require_translation("property_data.temporal_echo.msg_end_player", "时间重现！");
    require_translation("property_data.temporal_echo.msg_end_mon", "时间在{}身上重现。");
    require_translation("property_data.tele_ctrl.name", "传送控制");
    require_translation("property_data.tele_ctrl.name_short", "传控");
    require_translation("property_data.tele_ctrl.descr", "可以控制传送目的地。");
    require_translation("property_data.tele_ctrl.msg_start_player", "我感到一切尽在掌控。");
    require_translation("property_data.tele_ctrl.msg_end_player", "我感觉没那么能掌控了。");
    require_translation("property_data.aiming.name", "瞄准");
    require_translation("property_data.aiming.name_short", "瞄准");
    require_translation("property_data.aiming.descr", "远程攻击效果提高。");
    require_translation("property_data.conflict.name", "冲突");
    require_translation("property_data.conflict.name_short", "冲突");
    require_translation("property_data.conflict.descr", "将所有生物都视为敌人。");
    require_translation("property_data.conflict.msg_start_mon", "{}看起来陷入冲突。");
    require_translation("property_data.conflict.msg_end_mon", "{}看起来更加坚定。");
    require_translation("property_data.aura_of_decay.name", "衰败光环");
    require_translation("property_data.aura_of_decay.name_short", "衰败环");
    require_translation("property_data.aura_of_decay.descr", "距离两步以内的生物每个标准回合都会受到伤害。");
    require_translation("property_data.aura_of_decay.msg_start_player", "枯萎环绕着我。");
    require_translation("property_data.aura_of_decay.msg_start_mon", "{}似乎散发着死亡与衰败。");
    require_translation("property_data.aura_of_decay.msg_end_player", "衰败平息了。");
    require_translation("property_data.aura_of_decay.msg_end_mon", "{}不再散发衰败。");
    require_translation("property_data.regenerating.name", "再生");
    require_translation("property_data.regenerating.name_short", "再生");
    require_translation("property_data.regenerating.descr", "每回合额外恢复1点生命值。");
    require_translation("property_data.regenerating.msg_start_player", "我的身体开始更快地自我愈合。");
    require_translation("property_data.regenerating.msg_start_mon", "{}开始非常快速地再生伤害。");
    require_translation("property_data.regenerating.msg_end_player", "我的身体自我愈合变慢了。");
    require_translation("property_data.regenerating.msg_end_mon", "{}停止快速再生伤害。");
    require_translation("property_data.ethereal.name", "灵体");
    require_translation("property_data.ethereal.name_short", "灵体");
    require_translation("property_data.ethereal.descr", "可以穿过实体物体，闪避攻击几率+50%。");
    require_translation("property_data.burrowing.name", "掘地");
    require_translation("property_data.burrowing.name_short", "掘地");
    require_translation("property_data.burrowing.descr", "可以掘穿墙壁和瓦砾。");
    require_translation("property_data.burrowing.msg_start_player", "泥土与石头在我面前崩碎。");
    require_translation("property_data.burrowing.msg_start_mon", "{}可以穿过泥土移动。");
    require_translation("property_data.burrowing.msg_end_player", "大地再次变得坚实。");
    require_translation("property_data.burrowing.msg_end_mon", "{}不再能穿过泥土移动。");
    require_translation("property_data.hit_chance_penalty_curse.msg_start_player", "我的瞄准变差了。");
    require_translation("property_data.hit_chance_penalty_curse.msg_end_player", "我的瞄准变好了。");
    require_translation("property_data.increased_shock_curse.msg_start_player", "我更加焦虑了！");
    require_translation("property_data.increased_shock_curse.msg_end_player", "我没那么焦虑了。");
    require_translation("property_data.cannot_read_curse.msg_start_player", "我觉得自己无法阅读了！");
    require_translation("property_data.cannot_read_curse.msg_end_player", "我又能阅读了。");
    require_translation("item_data.item_type.melee_wpn.land_on_hard_snd_msg", "我听到一阵金属叮当声。");
    require_translation(
        "item_data.item_type.scroll.base_descr_1",
        "一份奥秘咒文的简短抄本。纸张上有一种奇异灵光，仿佛某种力量被灌注在纸张本身之中。");
    require_translation("item_data.item_type.scroll.base_descr_2", "它应该可以被正确念出，但用途并不明朗。");
    require_translation("item_scroll.manuscript_titled_prefix", "题为");
    require_translation("item_scroll.manuscript_titled_suffix", "的手稿");
    require_translation("item_scroll.manuscripts_titled_prefix", "题为");
    require_translation("item_scroll.manuscripts_titled_suffix", "的手稿");
    require_translation("item_scroll.a_manuscript_titled_prefix", "一份题为");
    require_translation("item_scroll.a_manuscript_titled_suffix", "的手稿");
    require_translation("item_scroll.manuscript_of_prefix", "");
    require_translation("item_scroll.manuscript_of_suffix", "手稿");
    require_translation("item_scroll.manuscripts_of_prefix", "");
    require_translation("item_scroll.manuscripts_of_suffix", "手稿");
    require_translation("item_scroll.a_manuscript_of_prefix", "一份");
    require_translation("item_scroll.a_manuscript_of_suffix", "手稿");
    require_translation("item_scroll.fake_name.cruensseasrjit", "Cruensseasrjit");
    require_translation("item_scroll.fake_name.rudsceleratus", "Rudsceleratus");
    require_translation("item_scroll.fake_name.rudminuox", "Rudminuox");
    require_translation("item_scroll.fake_name.cruo_stragara_na", "Cruo stragara-na");
    require_translation("item_scroll.fake_name.praya_navita", "Praya navita");
    require_translation("item_scroll.fake_name.pretia_cruento", "Pretia Cruento");
    require_translation("item_scroll.fake_name.pestis_cruento", "Pestis Cruento");
    require_translation("item_scroll.fake_name.cruento_pestis", "Cruento Pestis");
    require_translation("item_scroll.fake_name.domus_bhaava", "Domus-bhaava");
    require_translation("item_scroll.fake_name.acerbus_shatruex", "Acerbus-shatruex");
    require_translation("item_scroll.fake_name.pretaanluxis", "Pretaanluxis");
    require_translation("item_scroll.fake_name.praansilenux", "Praansilenux");
    require_translation("item_scroll.fake_name.quodpipax", "Quodpipax");
    require_translation("item_scroll.fake_name.lokemundux", "Lokemundux");
    require_translation("item_scroll.fake_name.profanuxes", "Profanuxes");
    require_translation("item_scroll.fake_name.shaantitus", "Shaantitus");
    require_translation("item_scroll.fake_name.geropayati", "Geropayati");
    require_translation("item_scroll.fake_name.vilomaxus", "Vilomaxus");
    require_translation("item_scroll.fake_name.bhuudesco", "Bhuudesco");
    require_translation("item_scroll.fake_name.durbentia", "Durbentia");
    require_translation("item_scroll.fake_name.bhuuesco", "Bhuuesco");
    require_translation("item_scroll.fake_name.maravita", "Maravita");
    require_translation("item_scroll.fake_name.infirmux", "Infirmux");
    require_translation("item_scroll.fake_word.cruo", "Cruo");
    require_translation("item_scroll.fake_word.cruonit", "Cruonit");
    require_translation("item_scroll.fake_word.cruentu", "Cruentu");
    require_translation("item_scroll.fake_word.marana", "Marana");
    require_translation("item_scroll.fake_word.domus", "Domus");
    require_translation("item_scroll.fake_word.malax", "Malax");
    require_translation("item_scroll.fake_word.caecux", "Caecux");
    require_translation("item_scroll.fake_word.eximha", "Eximha");
    require_translation("item_scroll.fake_word.vorox", "Vorox");
    require_translation("item_scroll.fake_word.bibox", "Bibox");
    require_translation("item_scroll.fake_word.pallex", "Pallex");
    require_translation("item_scroll.fake_word.profanx", "Profanx");
    require_translation("item_scroll.fake_word.invisuu", "Invisuu");
    require_translation("item_scroll.fake_word.invisux", "Invisux");
    require_translation("item_scroll.fake_word.odiosuu", "Odiosuu");
    require_translation("item_scroll.fake_word.odiosux", "Odiosux");
    require_translation("item_scroll.fake_word.vigra", "Vigra");
    require_translation("item_scroll.fake_word.crudux", "Crudux");
    require_translation("item_scroll.fake_word.desco", "Desco");
    require_translation("item_scroll.fake_word.esco", "Esco");
    require_translation("item_scroll.fake_word.gero", "Gero");
    require_translation("item_scroll.fake_word.klaatu", "Klaatu");
    require_translation("item_scroll.fake_word.barada", "Barada");
    require_translation("item_scroll.fake_word.nikto", "Nikto");
    require_translation("item_scroll.keep_to_reveal", "也许带着它一段时间，会揭示一些关于它的事情。");
    require_translation("item_scroll.domain_reveal_prefix", "我觉得");
    require_translation("item_scroll.domain_reveal_mid", "属于");
    require_translation("item_scroll.domain_reveal_suffix", "领域。");
    require_translation("map_mode_gui.wpn_label", "武器");
    require_translation("map_mode_gui.alt_wpn_label", "备用");
    require_translation("map_mode_gui.health_label", "生命");
    require_translation("map_mode_gui.spirit_label", "精神");
    require_translation("map_mode_gui.fervor_label", "热忱");
    require_translation("map_mode_gui.shock_label", "震惊");
    require_translation("map_mode_gui.insanity_label", "疯狂");
    require_translation("map_mode_gui.weight_label", "负重");
    require_translation("map_mode_gui.turn_label", "回合");
    require_translation("map_mode_gui.armor_label", "护甲");
    require_translation("map_mode_gui.level_label", "等级");
    require_translation("map_mode_gui.depth_label", "深度");
    require_translation("map_mode_gui.lantern_label", "提灯");
    require_translation("map_mode_gui.none", "无");
    require_translation("map_mode_gui.medical_supplies_label", "医疗品");
    require_translation("map_mode_gui.dark_area", "黑暗区域");
    require_translation("map_mode_gui.gj_mode_enabled", "GJ模式开启");
    require_translation("item_data.item_type.potion.base_descr", "一个小玻璃瓶，里面装着神秘调合物。");
    require_translation("item_data.item_type.device.unidentified_name", "奇异装置");
    require_translation("item_data.item_type.device.unidentified_name_plural", "奇异装置");
    require_translation("item_data.item_type.device.unidentified_name_a", "一个奇异装置");
    require_translation(
        "item_data.item_type.device.base_descr",
        "一小块机器。它不可能是人类心智设计出来的。即使体积很小，也显得极其复杂。通过普通手段没有希望理解它的目的或功能。");
    require_translation("item_data.item_type.device.land_on_hard_snd_msg", "我听到一阵金属叮当声。");
    require_translation(
        "item_data.item_type.rod.base_descr",
        "一个圆柱形金属装置。它似乎是为人类双手设计的，不知有何邪恶用途。");
    require_translation("item_data.item_type.rod.land_on_hard_snd_msg", "我听到一阵金属叮当声。");
    require_translation("item_data.attack.strike.player", "打击");
    require_translation("item_data.attack.strike.other", "打击");
    require_translation("item_data.attack.fire.player", "射击");
    require_translation("item_data.attack.fire.other", "射击");
    require_translation("item_data.sawed_off.name", "短管霰弹枪");
    require_translation("item_data.sawed_off.name_plural", "短管霰弹枪");
    require_translation("item_data.sawed_off.name_a", "一把短管霰弹枪");
    require_translation(
        "item_data.sawed_off.base_descr",
        "与标准霰弹枪相比，短管霰弹枪有效射程更短；不过在近距离更具毁灭性。它有两根枪管，两发都射出后需要重新装填。");
    require_translation("item_data.sawed_off.ranged_snd_msg", "我听到一声霰弹枪轰鸣。");
    require_translation("item_data.pump_shotgun.name", "泵动霰弹枪");
    require_translation("item_data.pump_shotgun.name_plural", "泵动霰弹枪");
    require_translation("item_data.pump_shotgun.name_a", "一把泵动霰弹枪");
    require_translation(
        "item_data.pump_shotgun.base_descr",
        "泵动霰弹枪有一个可前后拉动的护木，用于退出已击发的弹壳并将新弹上膛。它有一根枪管，下面是装入霰弹的管状弹仓。弹仓容量为8发。");
    require_translation("item_data.pump_shotgun.ranged_snd_msg", "我听到一声霰弹枪轰鸣。");
    require_translation("item_data.shotgun_shell.name", "霰弹枪弹");
    require_translation("item_data.shotgun_shell.name_plural", "霰弹枪弹");
    require_translation("item_data.shotgun_shell.name_a", "一枚霰弹枪弹");
    require_translation("item_data.shotgun_shell.base_descr", "为霰弹枪发射而设计的弹药。");
    require_translation("item_data.morphic_blaster.name", "变形爆能枪");
    require_translation("item_data.morphic_blaster.name_plural", "变形爆能枪");
    require_translation("item_data.morphic_blaster.name_a", "一把变形爆能枪");
    require_translation(
        "item_data.morphic_blaster.base_descr_1",
        "米-戈制造的武器。它发射的弹体会在命中时释放爆炸性能量。武器会自我调适，与持用者的生理结合。一个导引系统会与大脑整合，确保弹体命中预定目标而不受瞄准技巧影响（不过它仍有小概率在路径上撞上非预期目标）。");
    require_translation(
        "item_data.morphic_blaster.base_descr_2_prefix",
        "当缺乏米-戈所用奇特能源的生物持用时，这件武器会改为从持用者的生命力中抽取能量（");
    require_translation("item_data.morphic_blaster.base_descr_2_hp_drained_infix", "点生命值/每次攻击，");
    require_translation("item_data.morphic_blaster.base_descr_2_regen_disabled_prefix", "被动生命恢复会被禁用");
    require_translation("item_data.morphic_blaster.base_descr_2_turns_suffix", "回合，且下一回合无法行动）。");
    require_translation("item_data.morphic_blaster.ranged_snd_msg", "我听到发射出的弹体爆响。");
    require_translation("item_data.tommy_gun.name", "汤米枪");
    require_translation("item_data.tommy_gun.name_plural", "汤米枪");
    require_translation("item_data.tommy_gun.name_a", "一把汤米枪");
    require_translation(
        "item_data.tommy_gun.base_descr",
        "“汤米枪”是汤普森冲锋枪的昵称，是一种配有弹鼓和垂直前握把的自动枪械。它发射.45 ACP弹药，弹鼓容量为50发。");
    require_translation("item_data.tommy_gun.ranged_snd_msg", "我听到机枪连射声。");
    require_translation("item_data.drum_of_bullets.name", ".45 ACP弹鼓");
    require_translation("item_data.drum_of_bullets.name_plural", ".45 ACP弹鼓");
    require_translation("item_data.drum_of_bullets.name_a", "一个.45 ACP弹鼓");
    require_translation("item_data.drum_of_bullets.base_descr", "汤米枪使用的弹药。");
    require_translation("item_data.revolver.name", "S&W左轮手枪");
    require_translation("item_data.revolver.name_plural", "S&W左轮手枪");
    require_translation("item_data.revolver.name_a", "一把S&W左轮手枪");
    require_translation("item_data.revolver.base_descr", "一把六发双动左轮手枪。");
    require_translation("item_data.revolver.ranged_snd_msg", "我听到左轮手枪开火声。");
    require_translation("item_data.revolver_bullet.name", "左轮.38子弹");
    require_translation("item_data.revolver_bullet.name_plural", "左轮.38子弹");
    require_translation("item_data.revolver_bullet.name_a", "一枚左轮.38子弹");
    require_translation("item_data.revolver_bullet.base_descr", "S&W Model 10左轮手枪使用的弹药。");
    require_translation("item_data.pistol.name", "M1911柯尔特手枪");
    require_translation("item_data.pistol.name_plural", "M1911柯尔特手枪");
    require_translation("item_data.pistol.name_a", "一把M1911柯尔特手枪");
    require_translation("item_data.pistol.base_descr", "一把使用弹匣供弹、发射.45 ACP弹药的半自动手枪。");
    require_translation("item_data.pistol.ranged_snd_msg", "我听到手枪开火声。");
    require_translation("item_data.pistol_mag.name", "柯尔特.45 ACP弹匣");
    require_translation("item_data.pistol_mag.name_plural", "柯尔特.45 ACP弹匣");
    require_translation("item_data.pistol_mag.name_a", "一个柯尔特.45 ACP弹匣");
    require_translation("item_data.pistol_mag.base_descr", "柯尔特手枪使用的弹药。");
    require_translation("item_data.rifle.name", "温彻斯特步枪");
    require_translation("item_data.rifle.name_plural", "温彻斯特步枪");
    require_translation("item_data.rifle.name_a", "一把温彻斯特步枪");
    require_translation("item_data.rifle.base_descr_1", "杠杆式连发步枪。");
    require_translation("item_data.rifle.base_descr_2", "这件武器在近距离有命中率惩罚。");
    require_translation("item_data.rifle.ranged_snd_msg", "我听到步枪开火声。");
    require_translation("item_data.rifle_bullet.name", "温彻斯特.30子弹");
    require_translation("item_data.rifle_bullet.name_plural", "温彻斯特.30子弹");
    require_translation("item_data.rifle_bullet.name_a", "一枚温彻斯特.30子弹");
    require_translation("item_data.rifle_bullet.base_descr", "温彻斯特步枪使用的弹药。");
    require_translation("item_data.spike_gun.name", "尖刺枪");
    require_translation("item_data.spike_gun.name_plural", "尖刺枪");
    require_translation("item_data.spike_gun.name_a", "一把尖刺枪");
    require_translation(
        "item_data.spike_gun.base_descr",
        "一件非常奇异而粗陋的武器，能够以足够穿透血肉（甚至岩石）的力量发射铁尖刺。它几乎像是被刻意设计用来施加残酷，而不是单纯为了制止目标。");
    require_translation("item_data.spike_gun.ranged_snd_msg", "我听到一件非常粗陋的武器开火。");
    require_translation("item_data.electric_gun.name", "电击枪");
    require_translation("item_data.electric_gun.name_plural", "电击枪");
    require_translation("item_data.electric_gun.name_a", "一把电击枪");
    require_translation("item_data.electric_gun.base_descr_1", "米-戈制造的武器。它会发射毁灭性的电流束。");
    require_translation(
        "item_data.electric_gun.base_descr_2_prefix",
        "当缺乏米-戈所用奇特能源的生物持用时，这件武器会改为从持用者的生命力中抽取能量（");
    require_translation("item_data.electric_gun.base_descr_2_hp_drained_infix", "点生命值/每次攻击，");
    require_translation("item_data.electric_gun.base_descr_2_regen_disabled_prefix", "被动生命恢复会被禁用");
    require_translation("item_data.electric_gun.base_descr_2_turns_suffix", "回合）。");
    require_translation("item_data.electric_gun.ranged_snd_msg", "我听到一道电流爆响。");
    require_translation("item_data.trap_dart.ranged_snd_msg", "我听到弹体发射声。");
    require_translation("item_data.attack.stab.player", "刺击");
    require_translation("item_data.attack.stab.other", "刺击");
    require_translation("item_data.attack.smash.player", "砸击");
    require_translation("item_data.attack.smash.other", "砸击");
    require_translation("item_data.attack.chop.player", "劈砍");
    require_translation("item_data.attack.chop.other", "劈砍");
    require_translation("item_data.dynamite.name", "炸药");
    require_translation("item_data.dynamite.name_plural", "炸药棒");
    require_translation("item_data.dynamite.name_a", "一根炸药棒");
    require_translation("item_data.dynamite.base_descr", "一种以硝化甘油为基础的爆炸材料。这个名字来自古希腊语中表示“力量”的词。");
    require_translation("item_data.flare.name", "照明棒");
    require_translation("item_data.flare.name_plural", "照明棒");
    require_translation("item_data.flare.name_a", "一根照明棒");
    require_translation("item_data.flare.base_descr", "一种烟火装置，可以产生明亮光芒或强烈热量而不会爆炸。");
    require_translation("item_data.molotov.name", "燃烧瓶");
    require_translation("item_data.molotov.name_plural", "燃烧瓶");
    require_translation("item_data.molotov.name_a", "一个燃烧瓶");
    require_translation(
        "item_data.molotov.base_descr",
        "一种临时制作的燃烧武器，由装有易燃液体的玻璃瓶和用于点火的布料组成。使用时点燃布料并将瓶子掷向目标，立即造成一团火球，随后燃起熊熊烈火。");
    require_translation("item_data.smoke_grenade.name", "烟雾弹");
    require_translation("item_data.smoke_grenade.name_plural", "烟雾弹");
    require_translation("item_data.smoke_grenade.name_a", "一枚烟雾弹");
    require_translation(
        "item_data.smoke_grenade.base_descr",
        "一个薄钢筒，点燃后会从排放孔释放烟雾。它们主要用于制造烟幕以便隐蔽。产生的烟雾会伤害眼睛、喉咙和肺部，因此建议佩戴防护面具。");
    require_translation("item_data.thr_knife.name", "飞刀");
    require_translation("item_data.thr_knife.name_plural", "飞刀");
    require_translation("item_data.thr_knife.name_a", "一把飞刀");
    require_translation("item_data.thr_knife.base_descr", "一种经过专门设计和配重、便于有效投掷的刀。");
    require_translation("item_data.rock.name", "石块");
    require_translation("item_data.rock.name_plural", "石块");
    require_translation("item_data.rock.name_a", "一块石块");
    require_translation("item_data.rock.base_descr", "虽然不是很起眼的武器，但有技巧地使用也能产生一些效果。");
    require_translation("item_data.dagger.name", "匕首");
    require_translation("item_data.dagger.name_plural", "匕首");
    require_translation("item_data.dagger.name_a", "一把匕首");
    require_translation("item_data.dagger.base_descr_1", "常与欺骗、潜行和背叛联系在一起。许多暗杀都是用匕首完成的。");
    require_translation(
        "item_data.dagger.base_descr_2",
        "用匕首对未警觉的对手进行近战攻击造成+200%伤害（此外还享受潜行攻击通常的+50%伤害）。");
    require_translation("item_data.dagger.base_descr_3", "用匕首进行近战攻击是无声的。");
    require_translation("item_data.hatchet.name", "短柄斧");
    require_translation("item_data.hatchet.name_plural", "短柄斧");
    require_translation("item_data.hatchet.name_a", "一把短柄斧");
    require_translation(
        "item_data.hatchet.base_descr_1",
        "一把短柄小斧。短柄斧是可靠的武器——易于使用，并且相对于重量能造成不错的伤害。");
    require_translation("item_data.hatchet.base_descr_2", "用短柄斧进行近战攻击是无声的。");
    require_translation("item_data.club.name", "棍棒");
    require_translation("item_data.club.name_plural", "棍棒");
    require_translation("item_data.club.name_a", "一根棍棒");
    require_translation("item_data.club.base_descr_1", "自史前时代以来就被使用。");
    require_translation("item_data.club.base_descr_2", "用棍棒进行近战攻击是无声的。");
    require_translation("item_data.club.land_on_hard_snd_msg", "我听到一声闷响。");
    require_translation("item_data.hammer.name", "锤子");
    require_translation("item_data.hammer.name_plural", "锤子");
    require_translation("item_data.hammer.name_a", "一把锤子");
    require_translation("item_data.hammer.base_descr_1", "通常用于建筑施工，但作为武器挥舞时也相当具有毁灭性。");
    require_translation("item_data.hammer.base_descr_2", "用锤子进行近战攻击会发出声响。");
    require_translation("item_data.machete.name", "砍刀");
    require_translation("item_data.machete.name_plural", "砍刀");
    require_translation("item_data.machete.name_a", "一把砍刀");
    require_translation("item_data.machete.base_descr_1", "一把类似大型菜刀的刀。既适合作为切割工具，也适合作为武器。");
    require_translation("item_data.machete.base_descr_2", "用砍刀进行近战攻击会发出声响。");
    require_translation("item_data.axe.name", "斧头");
    require_translation("item_data.axe.name_plural", "斧头");
    require_translation("item_data.axe.name_a", "一把斧头");
    require_translation(
        "item_data.axe.base_descr_1",
        "用于砍树、劈木材等工作的工具。作为武器使用时可以造成毁灭性打击，尽管需要一定技巧才能有效使用。");
    require_translation("item_data.axe.base_descr_2", "用斧头进行近战攻击会发出声响。");
    require_translation("item_data.spiked_mace.name", "钉头锤");
    require_translation("item_data.spiked_mace.name_plural", "钉头锤");
    require_translation("item_data.spiked_mace.name_a", "一把钉头锤");
    require_translation("item_data.spiked_mace.base_descr_1", "一种残酷的武器，结合了钝击力与穿刺。");
    require_translation("item_data.spiked_mace.base_descr_2", "用这件武器攻击有25%几率击晕受害者，使其短时间无法行动。");
    require_translation("item_data.spiked_mace.base_descr_3", "用钉头锤进行近战攻击会发出声响。");
    require_translation("item_data.pitchfork.name", "草叉");
    require_translation("item_data.pitchfork.name_plural", "草叉");
    require_translation("item_data.pitchfork.name_a", "一把草叉");
    require_translation("item_data.pitchfork.base_descr_1", "一根长杆，末端有四齿叉头。");
    require_translation("item_data.pitchfork.base_descr_2", "草叉有助于让攻击者保持距离——被刺中的受害者会被推开。");
    require_translation("item_data.spear.name", "长矛");
    require_translation("item_data.spear.name_plural", "长矛");
    require_translation("item_data.spear.name_a", "一根长矛");
    require_translation("item_data.spear.base_descr", "一种由木柄和钢制矛头组成的长柄武器。");
    require_translation("item_data.sledgehammer.name", "大锤");
    require_translation("item_data.sledgehammer.name_plural", "大锤");
    require_translation("item_data.sledgehammer.name_a", "一把大锤");
    require_translation("item_data.sledgehammer.base_descr", "它能造成毁灭性伤害，但携带起来很笨重，也需要一定技巧才能有效使用。");
    require_translation("item_data.iron_spike.name", "铁尖刺");
    require_translation("item_data.iron_spike.name_plural", "铁尖刺");
    require_translation("item_data.iron_spike.name_a", "一根铁尖刺");
    require_translation("item_data.iron_spike.base_descr", "可用于楔住东西，使其保持关闭。");
    require_translation("item_data.player_kick.attack_player", "踢");
    require_translation("item_data.player_stomp.attack_player", "践踏");
    require_translation("item_data.player_punch.name", "拳击");
    require_translation("item_data.player_punch.name_a", "一记拳击");
    require_translation("item_data.player_punch.attack_player", "拳击");
    require_translation("item_data.player_ghoul_claw.name", "利爪");
    require_translation("item_data.player_ghoul_claw.name_a", "爪击");
    require_translation("item_data.player_ghoul_claw.attack_player", "爪击");
    require_translation("item_data.intr_kick.attack_other", "踢");
    require_translation("item_data.intr_bite.attack_other", "咬");
    require_translation("item_data.intr_claw.attack_other", "抓挠");
    require_translation("item_data.intr_strike.attack_other", "打击");
    require_translation("item_data.intr_punch.attack_other", "拳击");
    require_translation("item_data.intr_punch_knockback.attack_other", "拳击");
    require_translation("item_data.intr_headbutt.attack_other", "猛撞");
    require_translation("item_data.intr_putrid_spit.attack_other", "喷吐脓液");
    require_translation("item_data.intr_putrid_spit.ranged_snd_msg", "我听到喷吐声。");
    require_translation("item_data.intr_snake_venom_spit.attack_other", "喷吐毒液");
    require_translation("item_data.intr_snake_venom_spit.ranged_snd_msg", "我听到嘶嘶声和喷吐声。");
    require_translation("item_data.intr_earth_breath.attack_other", "喷吐出巨大密度");
    require_translation("item_data.intr_earth_breath.ranged_snd_msg", "我听到锤击般的声音。");
    require_translation("item_data.intr_water_breath.attack_other", "喷吐出汹涌激流");
    require_translation("item_data.intr_water_breath.ranged_snd_msg", "我听到水流奔涌声。");
    require_translation("item_data.intr_fire_breath.attack_other", "喷吐火焰");
    require_translation("item_data.intr_fire_breath.ranged_snd_msg", "我听到一阵火焰爆燃声。");
    require_translation("item_data.intr_lightning_breath.attack_other", "喷吐闪电");
    require_translation("item_data.intr_lightning_breath.ranged_snd_msg", "我听到一阵雷电爆响。");
    require_translation("item_data.intr_raven_peck.attack_other", "啄");
    require_translation("item_data.intr_vampiric_bite.attack_other", "咬");
    require_translation("item_data.intr_strangle.attack_other", "勒住");
    require_translation("item_data.intr_ghost_touch.attack_other", "伸手抓向");
    require_translation("item_data.intr_sting.attack_other", "刺");
    require_translation("item_data.intr_mind_leech_sting.attack_other", "刺");
    require_translation("item_data.intr_spear_thrust.attack_other", "刺击");
    require_translation("item_data.intr_net_throw.attack_other", "投出一张网");
    require_translation("item_data.intr_net_throw.ranged_snd_msg", "我听到呼啸声。");
    require_translation("item_data.intr_maul.attack_other", "撕咬");
    require_translation("item_data.intr_pus_spew.attack_other", "喷吐脓液到");
    require_translation("item_data.intr_strange_color_touch.attack_other", "触碰");
    require_translation("item_data.intr_dust_engulf.attack_other", "吞没");
    require_translation("item_data.intr_fire_engulf.attack_other", "吞没");
    require_translation("item_data.intr_energy_engulf.attack_other", "吞没");
    require_translation("item_data.intr_spores.attack_other", "释放孢子到");
    require_translation("item_data.intr_web_bola.attack_other", "发射蛛网流星索");
    require_translation("item_data.armor_leather_jacket.name", "皮夹克");
    require_translation("item_data.armor_leather_jacket.name_a", "一件皮夹克");
    require_translation("item_data.armor_leather_jacket.base_descr", "它提供一些保护。");
    require_translation("item_data.armor_heavy_coat.name", "厚外套");
    require_translation("item_data.armor_heavy_coat.name_a", "一件厚外套");
    require_translation("item_data.armor_heavy_coat.base_descr", "它提供不错的保护，代价是让移动稍微更困难（-5%潜行，-5%闪避）。");
    require_translation("item_data.armor_iron_suit.name", "铁甲");
    require_translation("item_data.armor_iron_suit.name_a", "一套铁甲");
    require_translation("item_data.armor_iron_suit.base_descr_1", "一套由金属板、螺栓和皮带构成的粗制铠甲。");
    require_translation("item_data.armor_iron_suit.base_descr_2", "它可以吸收大量伤害，但会让移动困难得多（-20%潜行，-20%闪避）。");
    require_translation("item_data.armor_iron_suit.land_on_hard_snd_msg", "我听到一声撞击巨响。");
    require_translation("item_data.armor_flak_jacket.name", "防弹背心");
    require_translation("item_data.armor_flak_jacket.name_a", "一件防弹背心");
    require_translation("item_data.armor_flak_jacket.base_descr_1", "一种由缝入背心的钢板组成的护甲。");
    require_translation("item_data.armor_flak_jacket.base_descr_2", "它相对于重量能提供非常好的保护，但穿起来有些笨重（-10%潜行，-10%闪避）。");
    require_translation("item_data.armor_flak_jacket.land_on_hard_snd_msg", "我听到一声闷响。");
    require_translation("item_data.armor_asb_suit.name", "石棉防护服");
    require_translation("item_data.armor_asb_suit.name_a", "一套石棉防护服");
    require_translation("item_data.armor_asb_suit.base_descr_1", "一件石棉织物制成的连体工作服，包括兜帽、炉用面罩、手套和鞋子。");
    require_translation("item_data.armor_asb_suit.base_descr_2", "它保护穿戴者免受火焰和电击伤害，也能抵御烟、烟雾和毒气。");
    require_translation("item_data.armor_asb_suit.base_descr_3", "它穿起来有些笨重（-10%潜行，-10%闪避）。");
    require_translation("item_data.armor_mi_go.name", "米-戈生体护甲");
    require_translation("item_data.armor_mi_go.name_a", "一件米-戈生体护甲");
    require_translation("item_data.armor_mi_go.base_descr", "米-戈创造的极其耐用的生物护甲。");
    require_translation("item_data.gas_mask.name", "防毒面具");
    require_translation("item_data.gas_mask.name_a", "一个防毒面具");
    require_translation(
        "item_data.gas_mask.base_descr_1",
        "保护眼睛、喉咙和肺部免受烟尘与烟雾伤害。它的有效使用寿命有限，与滤芯的吸附能力有关。");
    require_translation(
        "item_data.gas_mask.base_descr_2",
        "由于眼窗很小，瞄准会稍微更困难，也更难发现潜行的敌人和隐藏物体（-10%近战和远程命中率，-6%搜索）。");
    require_translation("item_data.torture_collar.name", "折磨项圈");
    require_translation("item_data.torture_collar.name_a", "一个折磨项圈");
    require_translation("item_data.torture_collar.base_descr_1", "一种可怖的折磨器具，尖刺刺入佩戴者颈部。它无法被取下。");
    require_translation("item_data.torture_collar.base_descr_2", "戴着项圈行走需要额外回合，潜行和闪避降低20%。不过，佩戴项圈会让苦修者更能承受肉体痛苦，护甲增加3点。");
    require_translation("item_data.device_blaster.name", "爆能装置");
    require_translation("item_data.device_blaster.name_plural", "爆能装置");
    require_translation("item_data.device_blaster.name_a", "一个爆能装置");
    require_translation("item_data.device_rejuvenator.name", "回春装置");
    require_translation("item_data.device_rejuvenator.name_plural", "回春装置");
    require_translation("item_data.device_rejuvenator.name_a", "一个回春装置");
    require_translation("item_data.device_translocator.name", "移位装置");
    require_translation("item_data.device_translocator.name_plural", "移位装置");
    require_translation("item_data.device_translocator.name_a", "一个移位装置");
    require_translation("item_data.device_sentry_drone.name", "哨戒无人机装置");
    require_translation("item_data.device_sentry_drone.name_plural", "哨戒无人机装置");
    require_translation("item_data.device_sentry_drone.name_a", "一个哨戒无人机装置");
    require_translation("item_data.device_force_field.name", "力场装置");
    require_translation("item_data.device_force_field.name_plural", "力场装置");
    require_translation("item_data.device_force_field.name_a", "一个力场装置");
    require_translation("item_data.medical_bag.name", "医疗包");
    require_translation("item_data.medical_bag.name_plural", "医疗包");
    require_translation("item_data.medical_bag.name_a", "一个医疗包");
    require_translation("item_data.medical_bag.base_descr", "一个便携式医疗用品包。可用于处理伤口或感染。");
    require_translation("item_data.lantern.name", "电提灯");
    require_translation("item_data.lantern.name_plural", "电提灯");
    require_translation("item_data.lantern.name_a", "一盏电提灯");
    require_translation("item_data.lantern.base_descr", "一个便携式光源。");
    require_translation("item_data.lantern.land_on_hard_snd_msg", "我听到一阵金属叮当声。");
    require_translation("item_data.pharaoh_staff.name", "法老权杖");
    require_translation("item_data.pharaoh_staff.name_a", "法老权杖");
    require_translation(
        "item_data.pharaoh_staff.base_descr_1",
        "古代统治者所持的强大神器，能支配那些曾受它束缚的存在。任何看见持有者的木乃伊最终都会被转化（携带此武器时每回合有10%几率）。");
    require_translation(
        "item_data.pharaoh_staff.base_descr_2",
        "此外，被这件武器击中的目标可能遭受毁灭性的诅咒（50%几率施加厄运，大幅降低受害者的命中率、闪避和搜索能力，并使施法时有小概率失败）。");
    require_translation("item_data.flagellant_whip.name", "苦修鞭");
    require_translation("item_data.flagellant_whip.name_a", "一条苦修鞭");
    require_translation("item_data.flagellant_whip.base_descr_1", "一条残酷的鞭子，上面装有磨尖的骨片和金属尖刺。");
    require_translation("item_data.flagellant_whip.base_descr_2", "被它撕裂血肉的咬噬击中的受害者，可能因痛苦而麻痹（20%几率）。");
    require_translation("item_data.onyx_drop.name", "缟玛瑙滴坠");
    require_translation("item_data.onyx_drop.name_a", "缟玛瑙滴坠");
    require_translation("item_data.onyx_drop.base_descr", "饮用恶性药水时，其效果也会施加到附近所有生物身上（最大距离为6）。");
    require_translation("item_data.refl_talisman.name", "反射护符");
    require_translation("item_data.refl_talisman.name_a", "反射护符");
    require_translation("item_data.refl_talisman.base_descr", "每当因法术抗性阻挡敌对法术时，该法术也会被反射。重新获得法术抗性所需的回合数减半。");
    require_translation("item_data.resurrect_talisman.name", "复活护符");
    require_translation("item_data.resurrect_talisman.name_a", "复活护符");
    require_translation("item_data.resurrect_talisman.base_descr", "这个强大的护符会在持有者肉体死亡时使其复活。不过护符会在过程中被摧毁，因此只能复活一次。");
    require_translation("item_data.tele_ctrl_talisman.name", "传送控制护符");
    require_translation("item_data.tele_ctrl_talisman.name_a", "传送控制护符");
    require_translation("item_data.tele_ctrl_talisman.base_descr", "赋予持有者在传送时控制目的地的能力。");
    require_translation("item_data.holy_symbol.name", "圣符");
    require_translation("item_data.holy_symbol.name_a", "圣符");
    require_translation(
        "item_data.holy_symbol.base_descr_1",
        "为灵魂与心智提供力量和指引的焦点。对圣符祈祷会获得1-4点精神点，并获得对精神震惊和恐惧的抗性，持续6-12回合。");
    require_translation(
        "item_data.holy_symbol.base_descr_2",
        "祈祷要经过一段时间才保证再次生效，但也可以在这段时间过去前尝试（有25%几率成功）。如果提前尝试失败，会暂时失去对圣符的信心，并且必须经过很长时间才能再次使用。");
    require_translation("item_data.clockwork.name", "奥术发条装置");
    require_translation("item_data.clockwork.name_a", "奥术发条装置");
    require_translation("item_data.clockwork.base_descr", "一件由主发条驱动、品质与美感都超乎现实的发条装置。上紧发条后，会使持有者在短时间内行动极为迅速。");
    require_translation("item_data.horn_of_malice.name", "恶意号角");
    require_translation("item_data.horn_of_malice.name_a", "恶意号角");
    require_translation(
        "item_data.horn_of_malice.base_descr",
        "吹响时，这件阴森神器会发出怪异共鸣，腐蚀听力范围内所有生物（吹号者除外）的心智，使它们以强烈仇恨和不信任看待所有其他生物。");
    require_translation("item_data.horn_of_banishment.name", "放逐号角");
    require_translation("item_data.horn_of_banishment.name_a", "放逐号角");
    require_translation("item_data.horn_of_banishment.base_descr", "吹响时，这件乐器会迫使听力范围内所有魔法召唤的生物返回其原本的领域。");
    require_translation("item_data.shadow_dagger.name", "加哈纳，黑色匕首");
    require_translation("item_data.shadow_dagger.name_a", "加哈纳，黑色匕首");
    require_translation("item_data.shadow_dagger.base_descr_1", "一把有精致装饰的漆黑匕首。刀刃显得模糊，仿佛永远笼罩在黑暗雾霭中。");
    require_translation(
        "item_data.shadow_dagger.base_descr_2",
        "被这把武器击中的生物会受到诅咒，永远栖身于黑暗中，否则便承受巨大痛苦（永久变得光敏，并受到来自光的+1额外伤害）。天生发光的生物（如火焰或能量存在）则会改为受到1-4点不可抵抗伤害。");
    require_translation("item_data.shadow_dagger.base_descr_3", "用匕首攻击未警觉的对手造成+200%伤害（此外还享受潜行攻击通常的+50%伤害）。");
    require_translation("item_data.orb_of_life.name", "生命之球");
    require_translation("item_data.orb_of_life.name_a", "生命之球");
    require_translation("item_data.orb_of_life.base_descr", "+4生命值，获得对毒素和疾病的抗性。");
    require_translation("item_data.necronomicon.name", "死灵之书");
    require_translation("item_data.necronomicon.name_a", "死灵之书");
    require_translation(
        "item_data.necronomicon.base_descr_1",
        "这就是令人畏惧的死灵之书——亡者之书！书页中记载着许多关于秘奥事物的可怖知识。携带时，所有法术都会以更高技能等级施放，并且可以达到第四等级“超凡”。");
    require_translation(
        "item_data.necronomicon.base_descr_2",
        "施放法术受到的所有精神震惊加倍，并且查阅这类知识者的存在会被强烈感知（-20%潜行，每回合2%几率惊动附近生物）。");
    require_translation("item_data.zombie_dust.name", "僵尸尘");
    require_translation("item_data.zombie_dust.name_plural", "一把把僵尸尘");
    require_translation("item_data.zombie_dust.name_a", "一把僵尸尘");
    require_translation("item_data.zombie_dust.base_descr", "投向活着的（非不死）生物时，这种粉末会导致麻痹。");
    require_translation("item_data.witch_eye.name", "女巫之眼");
    require_translation("item_data.witch_eye.name_plural", "女巫之眼");
    require_translation("item_data.witch_eye.name_a", "一只女巫之眼");
    require_translation(
        "item_data.witch_eye.base_descr",
        "一位强大女巫的眼睛。把它攥在手中会暂时获得魔法视觉——门、陷阱、楼梯和周围其他有趣地点会被侦测到，并且物品和生物的存在也会显露。");
    require_translation("item_data.bone_charm.name", "骨符");
    require_translation("item_data.bone_charm.name_plural", "骨符");
    require_translation("item_data.bone_charm.name_a", "一枚骨符");
    require_translation("item_data.bone_charm.base_descr_1", "一截刻着细小符号的古老指骨。");
    require_translation("item_data.bone_charm.base_descr_2", "将它折成两半可获得对有害法术的保护，持续6-12回合，或直到阻挡一个法术为止。");
    require_translation(
        "item_data.bone_charm.base_descr_3",
        "它还会驱散所有已看见的符印（地面上的“奇异形状”）。每驱散一个陷阱，便获得1-6点精神点，这可能使精神超过最大值。");
    require_translation("item_data.fluctuating_material.name", "波动物质");
    require_translation("item_data.fluctuating_material.name_plural", "波动物质块");
    require_translation("item_data.fluctuating_material.name_a", "一块波动物质");
    require_translation("item_data.fluctuating_material.base_descr_1", "很难判断它究竟是石头、金属，还是某种有机物。它似乎永远在变化，内部不断扭动、转动并流动。");
    require_translation(
        "item_data.fluctuating_material.base_descr_2",
        "瞥一眼这种物质就像望进万花筒——并且感觉凝视太深会转化观察者的本质（选择移除一个特质，然后选择一个新特质）。");
    require_translation("item_data.fluctuating_material.land_on_hard_snd_msg", "我听到一声闷响。");
    require_translation("item_data.astral_opium.name", "星界鸦片");
    require_translation("item_data.astral_opium.name_plural", "星界鸦片剂");
    require_translation("item_data.astral_opium.name_a", "一剂星界鸦片");
    require_translation("item_data.astral_opium.base_descr_1", "一种从源自外星的植物中提取的药物。它让使用者处于完全宁静、无所畏惧的状态。");
    require_translation("item_data.astral_opium.base_descr_2", "然而这是有代价的，因为它会导致强烈的致幻妄想，而且也极易成瘾。");
    require_translation("item_data.astral_opium.land_on_hard_snd_msg", "我听到一阵叮当声。");
    REQUIRE(i18n::get("spells.unexpected_effect", "An unexpected effect was induced by the spell.") == "法术引发了意想不到的效果。");
    REQUIRE(i18n::get("spells.resist_player", "I resist the spell!") == "我抵抗了法术！");
    REQUIRE(i18n::get("spells.resists_suffix", " resists the spell!") == "抵抗了法术！");
    REQUIRE(i18n::get("spells.reflected", "The spell is reflected!") == "法术被反射了！");
    REQUIRE(i18n::get("spells.projectile_hit_player_prefix", "I am") == "我");
    REQUIRE(i18n::get("spells.projectile_hit_it", "It") == "它");
    REQUIRE(i18n::get("spells.projectile_hit_mon_suffix", " is") == "");
    REQUIRE(i18n::get("spells.projectile_hit_space", " ") == "");
    REQUIRE(
        i18n::get(
            "spells.not_alerting_mon_descr",
            "Casting this spell does not alert the victim to the caster's presence.") ==
        "施放此法术不会让受害者察觉施法者的存在。");
    REQUIRE(i18n::get("spells.duration_prefix", "The spell lasts ") == "法术持续");
    REQUIRE(i18n::get("spells.duration_suffix", " turns.") == "回合。");
    REQUIRE(i18n::get("spells.duration_indefinite", "The spell lasts indefinitely.") == "法术无限期持续。");
    REQUIRE(
        i18n::get(
            "spells.cast_requires_sounds",
            "Casting this spell requires making sounds.") == "施放此法术需要发出声音。");
    REQUIRE(i18n::get("spells.cast_silently", "The spell can be cast silently.") == "此法术可以无声施放。");
    REQUIRE(i18n::get("spells.skill_descr_prefix", "The spell can be cast at ") == "此法术可以以");
    REQUIRE(i18n::get("spells.skill_descr_suffix", " level") == "等级施放");
    REQUIRE(i18n::get("spells.skill.basic", "basic") == "基础");
    REQUIRE(i18n::get("spells.skill.expert", "expert") == "专家");
    REQUIRE(i18n::get("spells.skill.master", "master") == "大师");
    REQUIRE(i18n::get("spells.skill.transcendent", "transcendent") == "超凡");
    REQUIRE(i18n::get("spells.skill_bonus.manuscript", "manuscript") == "手稿");
    REQUIRE(i18n::get("spells.skill_bonus.altar", "altar") == "祭坛");
    REQUIRE(i18n::get("spells.skill_bonus.erudition", "erudition") == "博学");
    REQUIRE(i18n::get("spells.skill_bonus.necronomicon", "necronomicon") == "死灵之书");
    REQUIRE(
        i18n::get(
            "spells.forgotten_hint",
            "Forgotten spells can be recalled by "
            "studying inscribed objects "
            "or by casting them from a manuscript.") == "遗忘的法术可以通过研究铭文物体或从手稿中施放来回忆。");
    REQUIRE(
        i18n::get(
            "spells.forgotten_descr_prefix",
            "Forgotten - this spell can no longer be "
            "cast from memory. ") == "已遗忘 - 此法术不能再凭记忆施放。");
    REQUIRE(
        i18n::get(
            "spells.tenebrous_descr_prefix",
            "Tenebrous - this spell will be instantly "
            "forgotten if cast from memory. ") == "晦暗 - 如果凭记忆施放，此法术会立刻被遗忘。");
    REQUIRE(i18n::get("spells.domain_descr_prefix", "It belongs to the \"") == "它属于“");
    REQUIRE(i18n::get("spells.domain_descr_suffix", "\" domain.") == "”领域。");
    REQUIRE(i18n::get("spells.someone", "Someone") == "某人");
    REQUIRE(i18n::get("spells.something", "Something") == "某物");
    REQUIRE(i18n::get("spells.aura_of_decay.name", "Aura of Decay") == "腐朽光环");
    REQUIRE(
        i18n::get(
            "spells.aura_of_decay.descr",
            "The caster exudes death and decay. Creatures within a "
            "distance of two steps take damage each standard turn.") ==
        "施法者散发死亡与腐朽。两步距离内的生物每个标准回合都会受到伤害。");
    REQUIRE(i18n::get("spells.aura_of_decay.dmg_prefix", "The spell deals ") == "法术造成");
    REQUIRE(i18n::get("spells.aura_of_decay.dmg_suffix", " damage to each creature.") == "点伤害给每个生物。");
    REQUIRE(
        i18n::get(
            "spells.aura_of_decay.instant_kill_descr",
            "Any time a creature takes damage from the spell, "
            "they may be destroyed immediately (2% chance).") ==
        "每当一个生物受到此法术伤害时，它都可能立即被摧毁（2% 几率）。");
    REQUIRE(i18n::get("spells.force_bolt.hit_msg_ending", "struck by a bolt!") == "被力能箭击中！");
    REQUIRE(i18n::get("spells.force_bolt.name", "Force Bolt") == "力能箭");
    REQUIRE(i18n::get("spells.darkbolt.hit_msg_ending", "struck by a blast!") == "被暗能冲击击中！");
    REQUIRE(i18n::get("spells.darkbolt.name", "Darkbolt") == "暗能箭");
    REQUIRE(
        i18n::get(
            "spells.darkbolt.descr",
            "A bolt of siphoned energy is hurled towards a target "
            "with great force. "
            "The conjured bolt has some will on its own - "
            "once released, it seeks creatures that pose a threat, "
            "precise control is therefore not possible.") ==
        "一支被汲取出的能量箭以强大力量射向目标。被召唤出的能量箭有一定自主意志——一旦释放，它会寻找构成威胁的生物，因此无法精确控制。");
    REQUIRE(i18n::get("spells.darkbolt.impact_dmg_prefix", "The impact deals ") == "冲击造成");
    REQUIRE(i18n::get("spells.darkbolt.impact_dmg_suffix", " damage.") == "点伤害。");
    REQUIRE(i18n::get("spells.darkbolt.paralyze_burn", " The target is paralyzed and set aflame.") == "目标被麻痹并被点燃。");
    REQUIRE(
        i18n::get(
            "spells.darkbolt.distant_explosion",
            " If the target is sufficiently far away from "
            "the caster, the bolt explodes on impact.") == "如果目标距离施法者足够远，能量箭会在命中时爆炸。");
    REQUIRE(i18n::get("spells.darkbolt.paralyze", " The target is paralyzed.") == "目标被麻痹。");
    REQUIRE(i18n::get("spells.gnawing_torrent.hit_msg_ending", "fed upon!") == "被吞噬生命！");
    REQUIRE(i18n::get("spells.gnawing_torrent.name", "Gnawing Torrent") == "啃噬洪流");
    REQUIRE(
        i18n::get(
            "spells.gnawing_torrent.descr",
            "Unleashes a stream of devouring energy upon the caster's victims.") ==
        "向施法者的受害者释放一股吞噬能量流。");
    REQUIRE(
        i18n::get(
            "spells.gnawing_torrent.projectiles_prefix",
            " projectiles are conjured, each dealing ") == "枚投射物被召唤，每枚造成");
    REQUIRE(i18n::get("spells.gnawing_torrent.projectiles_suffix", " damage.") == "点伤害。");
    REQUIRE(
        i18n::get(
            "spells.gnawing_torrent.life_feed_descr",
            "Each impact feeds life force back to the caster, providing 1 hit point "
            "(only against creatures of flesh and blood; "
            "ethereal creatures cannot be fed upon for example).") ==
        "每次命中都会把生命力反馈给施法者，提供 1 点生命值（仅对血肉生物有效；例如无法从以太生物身上吞噬生命）。");
    REQUIRE(
        i18n::get(
            "spells.gnawing_torrent.above_max_hp_descr",
            "Hit points can be raised above the normal maximum level.") == "生命值可以被提升到正常上限以上。");
    REQUIRE(i18n::get("spells.aza_gaze.name", "Azathoth's Gaze") == "阿撒托斯凝视");
    REQUIRE(i18n::get("spells.aza_gaze.player_hit_prefix", "I am") == "我");
    REQUIRE(i18n::get("spells.aza_gaze.mon_hit_middle", " is") == "");
    REQUIRE(i18n::get("spells.aza_gaze.wracked_by_chaos_suffix", " wracked by chaos.") == "被混沌折磨。");
    REQUIRE(
        i18n::get(
            "spells.aza_gaze.descr",
            "Channels the chaos of Azathoth unto all visible enemies. "
            "The channel can only be opened for a fraction of a second, "
            "but even this is enough to cause great physical and mental "
            "devastation.") == "将阿撒托斯的混沌引向所有可见敌人。通道只能开启一瞬，但即便如此也足以造成巨大的肉体与精神毁灭。");
    REQUIRE(i18n::get("spells.aza_gaze.dmg_prefix", "The spell deals ") == "法术造成");
    REQUIRE(i18n::get("spells.aza_gaze.dmg_suffix", " damage to each creature.") == "点伤害给每个生物。");
    REQUIRE(i18n::get("spells.aza_gaze.faint_prefix", "Causes the victims to faint for ") == "使受害者昏厥");
    REQUIRE(i18n::get("spells.aza_gaze.faint_suffix", " turns, if they are susceptible.") == "回合，如果它们会受此影响。");
    REQUIRE(i18n::get("spells.aza_gaze.conflict_prefix", "The victims become conflicted for ") == "受害者陷入冲突");
    REQUIRE(
        i18n::get(
            "spells.aza_gaze.conflict_suffix",
            " turns, causing them to view any creature as "
            "their enemy.") == "回合，使它们把任何生物都视为敌人。");
    REQUIRE(i18n::get("spells.cataclysm.name", "Cataclysm") == "大灾变");
    REQUIRE(i18n::get("spells.cataclysm.descr", "Blasts the surrounding area with terrible force.") == "以可怕的力量轰击周围区域。");
    REQUIRE(
        i18n::get(
            "spells.cataclysm.skill_descr",
            "Higher skill levels increases the magnitude of the destruction.") == "更高的技能等级会增强毁灭的规模。");
    REQUIRE(i18n::get("spells.pestilence.name", "Pestilence") == "瘟疫");
    REQUIRE(i18n::get("spells.pestilence.descr", "A pack of rats appear around the caster.") == "一群鼠类出现在施法者周围。");
    REQUIRE(i18n::get("spells.pestilence.summons_prefix", "Summons ") == "召唤");
    REQUIRE(i18n::get("spells.pestilence.summons_middle", " rats. They exist for ") == "只老鼠。它们存在");
    REQUIRE(i18n::get("spells.pestilence.summons_suffix", " turns (their own turns).") == "回合（以它们自己的回合计）。");
    REQUIRE(i18n::get("spells.pestilence.hasted_rats", "The rats are Hasted (moves faster).") == "这些老鼠获得加速（移动更快）。");
    REQUIRE(
        i18n::get(
            "spells.pestilence.transcendent_rats",
            "Some of the rats are ethereal "
            "(much harder to hit, can move through solid objects), "
            "are immune to magic, can cast spells, and have "
            "extra hit points and damage.") ==
        "其中一些老鼠是以太形态（更难命中，可以穿过实体物体），免疫魔法，可以施放法术，并拥有额外生命值和伤害。");
    REQUIRE(i18n::get("spells.mirror_images.name", "Mirror Images") == "镜像");
    REQUIRE(
        i18n::get(
            "spells.mirror_images.descr",
            "Conjures illusory duplicates of the caster "
            "to mislead enemies and draw their attacks.") == "召唤施法者的幻象复制体，以误导敌人并吸引它们的攻击。");
    REQUIRE(
        i18n::get(
            "spells.mirror_images.presence_descr",
            "The mirror images project a powerful magical presence, "
            "causing attackers to prefer them over the caster. "
            "As magical apparitions rather than living creatures, "
            "they are extremely difficult to strike with conventional attacks. "
            "They are immune to elemental damage and largely unaffected by physical "
            "or mental afflictions.") ==
        "镜像散发强大的魔法存在感，使攻击者更倾向于攻击它们而不是施法者。它们是魔法幻影而非活物，常规攻击极难命中。它们免疫元素伤害，并且基本不受身体或精神异常影响。");
    REQUIRE(i18n::get("spells.mirror_images.creates_prefix", "Creates ") == "创造");
    REQUIRE(i18n::get("spells.mirror_images.creates_middle", " mirror images. They exist for ") == "个镜像。它们存在");
    REQUIRE(i18n::get("spells.mirror_images.creates_suffix", " turns (their own turns).") == "回合（以它们自己的回合计）。");
    REQUIRE(i18n::get("spells.projected_strike.name", "Projected Strike") == "投影打击");
    REQUIRE(
        i18n::get(
            "spells.projected_strike.descr",
            "Launches a psychic projection of the caster's carried melee weapons.") == "发射施法者携带的近战武器的灵能投影。");
    REQUIRE(
        i18n::get(
            "spells.projected_strike.attack_prefix",
            "Each projection attacks a visible enemy, using the caster's combat skill with +") ==
        "每个投影攻击一个可见敌人，使用施法者的战斗技能并获得 +");
    REQUIRE(
        i18n::get(
            "spells.projected_strike.attack_suffix",
            "% hit chance bonus. "
            "No enemy can be targeted more than once.") == "% 命中率加成。没有敌人会被选为目标超过一次。");
    REQUIRE(
        i18n::get(
            "spells.projected_strike.unlimited_weapons",
            "An unlimited number of weapons can be used for atacking.") == "可以使用无限数量的武器进行攻击。");
    REQUIRE(i18n::get("spells.projected_strike.max_weapons_prefix", "A maximum of ") == "最多可使用");
    REQUIRE(i18n::get("spells.projected_strike.max_weapons_middle", " ") == "");
    REQUIRE(i18n::get("spells.projected_strike.weapon_singular", "weapon") == "件武器");
    REQUIRE(i18n::get("spells.projected_strike.weapon_plural", "weapons") == "件武器");
    REQUIRE(i18n::get("spells.projected_strike.max_weapons_suffix", " may be used for attacking.") == "进行攻击。");
    REQUIRE(
        i18n::get(
            "spells.projected_strike.attacker_descr",
            "The caster acts as attacker - all normal conditions that affect "
            "hit chance or damage apply "
            "(e.g. bonus damage from melee traits, or damage penalty from being weakened).") ==
        "施法者被视为攻击者 - 所有影响命中率或伤害的常规条件都会生效（例如近战特质的额外伤害，或虚弱造成的伤害惩罚）。");
    REQUIRE(i18n::get("spells.control_object.name", "Control Object") == "控制物体");
    REQUIRE(
        i18n::get(
            "spells.control_object.descr",
            "Opens doors, chests, tombs, or cabinets. "
            "Closes or jams doors. "
            "Strikes doors, braziers, or statues.") == "打开门、箱子、坟墓或柜子。关闭或堵住门。攻击门、火盆或雕像。");
    REQUIRE(i18n::get("spells.control_object.walls_destroyed", " Walls can be destroyed.") == "墙壁可以被摧毁。");
    REQUIRE(i18n::get("spells.control_object.max_distance_prefix", "Maximum control distance is ") == "最大控制距离为");
    REQUIRE(i18n::get("spells.control_object.max_distance_suffix", ".") == "。");
    REQUIRE(
        i18n::get(
            "spells.control_object.select_descr",
            "When casting the spell, select a seen object to control "
            "within the maximum distance.") == "施放法术时，在最大距离内选择一个已看见的物体来控制。");
    REQUIRE(i18n::get("spells.cleansing_fire.name", "Cleansing Fire") == "净化之火");
    REQUIRE(i18n::get("spells.cleansing_fire.burn_prefix", "Causes the spell's victims to burn for ") == "使法术受害者燃烧");
    REQUIRE(
        i18n::get(
            "spells.cleansing_fire.burn_suffix",
            " turns, and scorches the ground around them with fire "
            "(be careful with hitting adjacent creatures).") == "回合，并用火焰灼烧它们周围的地面（小心击中相邻生物）。");
    REQUIRE(i18n::get("spells.target.one_visible_hostile", "Affects one random visible hostile creature.") == "影响一个随机可见敌对生物。");
    REQUIRE(i18n::get("spells.target.all_visible_hostile", "Affects all visible hostile creatures.") == "影响所有可见敌对生物。");
    REQUIRE(i18n::get("spells.sanctuary.name", "Sanctuary") == "圣域");
    REQUIRE(
        i18n::get(
            "spells.sanctuary.descr",
            "The caster is ignored by all hostile creatures for the "
            "duration of the spell. The effect is interrupted if the "
            "caster moves or performs a melee or ranged attack.") == "在法术持续期间，所有敌对生物都会忽视施法者。如果施法者移动或进行近战或远程攻击，效果会被中断。");
    REQUIRE(i18n::get("spells.purge.name", "Purge") == "肃清");
    REQUIRE(
        i18n::get(
            "spells.purge.destroy_adjacent_descr",
            "Destroys any altars, monoliths, gongs, or mirrors adjacent to the caster.") ==
        "摧毁施法者相邻的任何祭坛、巨石、铜锣或镜子。");
    REQUIRE(
        i18n::get(
            "spells.purge.undead_struck_prefix",
            "All Undead creatures adjacent to the caster (seen or not) are "
            "struck with ") == "施法者相邻的所有亡灵生物（无论是否可见）都会被击中，受到");
    REQUIRE(i18n::get("spells.purge.undead_struck_middle", " damage, and become terrified for ") == "点伤害，并恐惧");
    REQUIRE(i18n::get("spells.purge.undead_struck_suffix", " turns (unless they resist fear).") == "回合（除非它们抵抗恐惧）。");
    REQUIRE(i18n::get("spells.frenzy.name", "Incite Frenzy") == "激怒狂乱");
    REQUIRE(
        i18n::get(
            "spells.frenzy.descr",
            "Incites a great rage in the caster, who will charge their "
            "enemies with a terrible, uncontrollable fury.") ==
        "激起施法者的极度愤怒，他们将以可怕的、无法控制的狂怒冲向敌人。");
    REQUIRE(i18n::get("spells.bless.name", "Bless") == "祝福");
    REQUIRE(
        i18n::get(
            "spells.bless.descr",
            "The caster becomes more lucky "
            "(+10% to hit chance, evasion, stealth, and searching).") ==
        "施法者变得更加幸运（命中率、闪避、潜行和搜索各+10%）。");
    REQUIRE(i18n::get("spells.cancellation.name", "Cancellation") == "消解");
    REQUIRE(
        i18n::get(
            "spells.cancellation.descr_main",
            "Cancels temporary effects on nearby creatures. "
            "Pierces through and removes Spell Shield.") ==
        "消解附近生物的临时效果。穿透并移除法术护盾。");
    REQUIRE(
        i18n::get(
            "spells.cancellation.descr_vulnerable_prefix",
            "Outer Beings, Undead or Summoned creatures also take ") ==
        "外界存在、不死生物或被召唤生物还会受到");
    REQUIRE(i18n::get("spells.cancellation.descr_vulnerable_suffix", " damage.") == "点伤害。");
    REQUIRE(
        i18n::get(
            "spells.cancellation.descr_range_prefix",
            "The spell has a maximum range of ") ==
        "法术的最大范围为");
    REQUIRE(
        i18n::get(
            "spells.cancellation.descr_range_suffix",
            " steps, reaching through solid obstacles.") ==
        "步，可穿透坚固障碍物。");
    REQUIRE(
        i18n::get(
            "spells.cancellation.descr_enemies_prefix",
            "Effects removed from enemies: All resistances, ") ==
        "从敌人身上移除的效果：所有抗性、");
    REQUIRE(i18n::get("spells.cancellation.descr_enemies_suffix", ".") == "。");
    REQUIRE(
        i18n::get(
            "spells.cancellation.descr_allies_prefix",
            "From caster/allies: ") ==
        "从施法者/盟友身上移除：");
    REQUIRE(i18n::get("spells.cancellation.descr_allies_suffix", ".") == "。");
    REQUIRE(i18n::get("spells.boundary_sigil.name", "Inscribe Boundary Sigil") == "铭刻边界印记");
    REQUIRE(
        i18n::get(
            "spells.boundary_sigil.descr_main",
            "Inscribes a magical sigil upon the ground, "
            "preventing Outer Beings, Undead and Summoned creatures "
            "from entering it or making melee attacks across its boundary.") ==
        "在地面上铭刻一道魔法印记，阻止外界存在、不死生物和被召唤生物进入其中，或隔着其边界进行近战攻击。");
    REQUIRE(
        i18n::get(
            "spells.boundary_sigil.descr_actions_prefix",
            "The sigil can prevent ") ==
        "该印记可阻止");
    REQUIRE(
        i18n::get(
            "spells.boundary_sigil.descr_actions_suffix",
            " actions before it fades, "
            "though it also has a small chance to fade each turn.") ==
        "次行动后消散，但每回合也有很小几率自行消退。");
    REQUIRE(
        i18n::get(
            "spells.boundary_sigil.descr_floor_only",
            "Can only be inscribed on floor, but may overwrite an existing sigil.") ==
        "只能铭刻在地板上，但可以覆盖已有的印记。");
    REQUIRE(i18n::get("spells.light.name", "Light") == "光明");
    REQUIRE(
        i18n::get(
            "spells.light.descr_main",
            "Illuminates the area around the caster.") ==
        "照亮施法者周围的区域。");
    REQUIRE(
        i18n::get(
            "spells.light.descr_blind_prefix",
            "On casting, causes a blinding flash centered on the "
            "caster (but not affecting the caster itself). "
            "The blinding effect lasts ") ==
        "施放时，会以施法者为中心产生一道致盲闪光（但不会影响施法者自身）。致盲效果持续");
    REQUIRE(i18n::get("spells.light.descr_blind_suffix", " turns.") == "回合。");
    REQUIRE(
        i18n::get(
            "spells.light.descr_burn_prefix",
            "The flash is so intense that any victim caught in it "
            "will also burn for ") ==
        "闪光强烈到使任何被其波及的受害者还会燃烧");
    REQUIRE(i18n::get("spells.light.descr_burn_suffix", " turns.") == "回合。");
    REQUIRE(i18n::get("spells.invisibility.name", "Invisibility") == "隐形");
    REQUIRE(
        i18n::get(
            "spells.invisibility.descr_main",
            "Makes the caster invisible to normal vision for a "
            "brief time.") ==
        "使施法者暂时对普通视觉隐形。");
    REQUIRE(
        i18n::get(
            "spells.invisibility.descr_basic",
            "Attacking or casting spells reveals the caster.") ==
        "攻击或施放法术会暴露施法者。");
    REQUIRE(
        i18n::get(
            "spells.invisibility.descr_advanced",
            "The caster is truly invisible for the duration of "
            "the the spell, and can freely attack or cast "
            "spells without breaking the invisibility.") ==
        "在法术持续期间，施法者将真正隐形，可以自由攻击或施放法术而不会打破隐形。");
    REQUIRE(i18n::get("spells.see_invisible.name", "See Invisible") == "看见隐形");
    REQUIRE(
        i18n::get(
            "spells.see_invisible.descr",
            "Grants the caster the ability to see the invisible.") ==
        "赋予施法者看见隐形之物的能力。");
    REQUIRE(i18n::get("spells.spell_shield.name", "Spell Shield") == "法术护盾");
    REQUIRE(
        i18n::get(
            "spells.spell_shield.descr",
            "Grants protection against harmful spells. The effect lasts "
            "until a spell is blocked.") ==
        "赋予对有害法术的防护。效果会持续到有一个法术被阻挡为止。");
    REQUIRE(i18n::get("spells.haste.name", "Haste") == "加速");
    REQUIRE(
        i18n::get(
            "spells.haste.descr",
            "The caster moves faster relative to the world around them.") ==
        "施法者相对于周围世界移动得更快。");
    REQUIRE(i18n::get("spells.premonition.name", "Premonition") == "预感");
    REQUIRE(
        i18n::get(
            "spells.premonition.descr",
            "Grants foresight of attacks against the caster, "
            "making it extremely difficult for assailants to achieve a "
            "succesful hit.") ==
        "赋予施法者对针对自身攻击的预知，使袭击者极难成功命中。");
    REQUIRE(i18n::get("spells.erudition.name", "Erudition") == "博学");
    REQUIRE(
        i18n::get(
            "spells.erudition.descr_main",
            "Temporarily bestows the caster with an expanded understanding "
            "of the esoteric mechanisms behind magical practice. "
            "The caster's skill is improved by one level for all spells.") ==
        "暂时赋予施法者对魔法实践背后神秘机制的更深理解。施法者所有法术的技能等级都会提高一级。");
    REQUIRE(i18n::get("spells.erudition.duration_prefix", "The spell lasts ") == "法术持续");
    REQUIRE(i18n::get("spells.erudition.duration_turns", " turns") == "回合");
    REQUIRE(
        i18n::get(
            "spells.erudition.duration_transcendent_suffix",
            ". The effect does not end when casting spells, "
            "only when the duration expires.") ==
        "。施放法术时效果不会结束，只有在持续时间到期时才会结束。");
    REQUIRE(
        i18n::get(
            "spells.erudition.duration_normal_suffix",
            ", or until a spell is cast (either from a Manuscript "
            "or from memory).") ==
        "，或者直到施放一个法术为止（无论来自手稿还是记忆）。");
    REQUIRE(i18n::get("spells.identify.name", "Identify") == "鉴定");
    REQUIRE(
        i18n::get(
            "spells.identify.descr_all_items",
            "Immediately identifies all carried items.") ==
        "立即鉴定所有携带中的物品。");
    REQUIRE(
        i18n::get(
            "spells.identify.descr_one_item",
            "Identifies one carried item.") ==
        "鉴定一件携带中的物品。");
    REQUIRE(i18n::get("spells.identify.allowed_prefix", "The spell can identify ") == "该法术可以鉴定");
    REQUIRE(i18n::get("spells.identify.allowed_basic", "Manuscripts") == "手稿");
    REQUIRE(i18n::get("spells.identify.allowed_expert", "Manuscripts and Potions") == "手稿和药水");
    REQUIRE(i18n::get("spells.identify.allowed_master", "all items") == "所有物品");
    REQUIRE(i18n::get("spells.identify.allowed_suffix", ".") == "。");
    REQUIRE(i18n::get("spells.teleport.name", "Teleport") == "传送");
    REQUIRE(
        i18n::get(
            "spells.teleport.descr_main",
            "Instantly moves the caster to a different position.") ==
        "立即将施法者移动到另一个位置。");
    REQUIRE(
        i18n::get(
            "spells.teleport.max_dist_prefix",
            "Maximum teleport distance is ") ==
        "最大传送距离为");
    REQUIRE(i18n::get("spells.teleport.max_dist_suffix", ".") == "。");
    REQUIRE(
        i18n::get(
            "spells.teleport.invis_prefix",
            "On teleporting, the caster is invisible for ") ==
        "传送后，施法者会隐形");
    REQUIRE(i18n::get("spells.teleport.invis_suffix", " turns.") == "回合。");
    REQUIRE(i18n::get("spells.expulsion.name", "Expulsion") == "驱逐");
    REQUIRE(
        i18n::get(
            "spells.expulsion.descr_all",
            "All visible hostile creatures are teleported away.") ==
        "所有可见敌对生物都会被传送走。");
    REQUIRE(
        i18n::get(
            "spells.expulsion.descr_one",
            "One random visible hostile creature is teleported away.") ==
        "一个随机可见敌对生物会被传送走。");
    REQUIRE(i18n::get("spells.expulsion.max_dist_prefix", "Max distance is ") == "最大距离为");
    REQUIRE(i18n::get("spells.expulsion.max_dist_suffix", " steps.") == "步。");
    REQUIRE(
        i18n::get(
            "spells.expulsion.forced",
            "The teleportation is forced; the target can never control it.") ==
        "这是强制传送；目标永远无法控制它。");
    REQUIRE(i18n::get("spells.curse.name", "Curse") == "诅咒");
    REQUIRE(
        i18n::get(
            "spells.curse.victims_prefix",
            "The spell's victims are ") ==
        "法术的受害者会变成");
    REQUIRE(i18n::get("spells.curse.prop_open_paren", " (") == "（");
    REQUIRE(i18n::get("spells.curse.close_paren", ")") == "）");
    REQUIRE(
        i18n::get(
            "spells.curse.doom_chance_prefix",
            "With ") ==
        "有");
    REQUIRE(
        i18n::get(
            "spells.curse.doom_chance_middle",
            "% chance, the victims instead become ") ==
        "%几率，受害者会转而变成");
    REQUIRE(i18n::get("spells.poison.name", "Poison") == "中毒");
    REQUIRE(
        i18n::get(
            "spells.poison.victims_prefix",
            "The spell's victims are ") ==
        "法术的受害者会变成");
    REQUIRE(i18n::get("spells.poison.prop_open_paren", " (") == "（");
    REQUIRE(i18n::get("spells.poison.close_paren", ")") == "）");
    REQUIRE(i18n::get("spells.enfeeble.name", "Enfeeble") == "虚弱");
    REQUIRE(
        i18n::get(
            "spells.enfeeble.descr",
            "Physically enfeebles the spell's victims, causing them to "
            "only do half damage in melee combat.") ==
        "从肉体上削弱法术的受害者，使其在近战中只能造成一半伤害。");
    REQUIRE(i18n::get("spells.temporal_echo.name", "Temporal Echo") == "时间回响");
    REQUIRE(
        i18n::get(
            "spells.temporal_echo.descr_main",
            "For all visible enemies, time is manipulated so that damage taken during a "
            "brief period will recur when the effect ends.") ==
        "对于所有可见敌人，时间会被操纵，使其在短暂期间内受到的伤害在效果结束时再次重现。");
    REQUIRE(i18n::get("spells.temporal_echo.duration_prefix", "The effect lasts for ") == "效果持续");
    REQUIRE(
        i18n::get(
            "spells.temporal_echo.duration_middle",
            " turns (their turns). ") ==
        "回合（按它们自己的回合计）。期间受到的伤害中有");
    REQUIRE(
        i18n::get(
            "spells.temporal_echo.duration_suffix",
            "% of the damage taken during the effect is dealt again.") ==
        "%会再次结算。");
    REQUIRE(i18n::get("spells.slow.name", "Slow") == "迟缓");
    REQUIRE(
        i18n::get(
            "spells.slow.descr",
            "Causes the spell's victims to move more slowly.") ==
        "使法术的受害者移动得更慢。");
    REQUIRE(i18n::get("spells.terrify.name", "Terrify") == "恐惧");
    REQUIRE(
        i18n::get(
            "spells.terrify.descr",
            "Inflicts a nightmare illusion that overwhelms its victims with dread.") ==
        "施加一场噩梦般的幻象，使受害者被恐惧淹没。");
    REQUIRE(
        i18n::get(
            "spells.terrify.descr_transcendent",
            "Affected creatures also faint.") ==
        "受影响的生物还会昏厥。");
    REQUIRE(i18n::get("spells.terrify.creature_singular", "creature") == "生物");
    REQUIRE(i18n::get("spells.terrify.creature_plural", "creatures") == "生物");
    REQUIRE(i18n::get("spells.terrify.faint_chance_prefix", "Has a ") == "有");
    REQUIRE(
        i18n::get(
            "spells.terrify.faint_chance_middle",
            "% chance to also make affected ") ==
        "%几率让受影响的");
    REQUIRE(i18n::get("spells.terrify.faint_chance_suffix", " faint.") == "昏厥。");
    REQUIRE(i18n::get("spells.threat_projection.name", "Threat Projection") == "威胁投射");
    REQUIRE(
        i18n::get(
            "spells.threat_projection.descr",
            "Distorts the perception of the spell's victims, causing "
            "all other creatures to be misidentified as enemies.") ==
        "扭曲法术受害者的感知，使其把所有其他生物都误认为敌人。");
    REQUIRE(
        i18n::get(
            "spells.summon_tentacles.appear_msg",
            "Monstrous tentacles rise up from the ground!") == "巨大的触手从地面升起！");
    REQUIRE(i18n::get("spells.disease.name", "Disease") == "疾病");
    REQUIRE(
        i18n::get(
            "spells.disease.afflict_prefix",
            "A horrible disease is starting to afflict ") ==
        "一种可怕的疾病开始折磨");
    REQUIRE(i18n::get("spells.blind.name", "Blind") == "致盲");
    REQUIRE(i18n::get("spells.knockback.name", "Knockback") == "击退");
    REQUIRE(i18n::get("spells.healing.name", "Healing") == "治疗");
    REQUIRE(i18n::get("spells.heal_others.name", "Heal Others") == "治疗他者");
    REQUIRE(i18n::get("spells.healing.restore_prefix", "Restores ") == "恢复");
    REQUIRE(i18n::get("spells.healing.restore_suffix", " hit points.") == "点生命值。");
    REQUIRE(i18n::get("spells.healing.cures_basic", "Cures weakening and poisoning.") == "治愈虚弱和中毒。");
    REQUIRE(
        i18n::get(
            "spells.healing.cures_master",
            "Cures weakening, poisoning, infections, disease, blindness and deafness.") ==
        "治愈虚弱、中毒、感染、疾病、失明和失聪。");
    REQUIRE(i18n::get("spells.healing.heals_wound", "Heals one wound.") == "治疗一个伤口。");
    REQUIRE(
        i18n::get(
            "spells.healing.regen_prefix",
            "+1 hit point regenerated per turn, for ") ==
        "每回合恢复+1生命值，持续");
    REQUIRE(i18n::get("spells.healing.regen_suffix", " turns.") == "回合。");
    REQUIRE(i18n::get("spells.migo_hypnosis.name", "MiGo Hypnosis") == "米戈催眠");
    REQUIRE(i18n::get("spells.immolation.name", "Immolation") == "焚烧");
    REQUIRE(i18n::get("spells.deafen.name", "Deafen") == "致聋");
    REQUIRE(i18n::get("spells.transmutation.name", "Transmutation") == "变形术");
    REQUIRE(i18n::get("spells.transmutation.item_before_prefix", "The ") == "");
    REQUIRE(i18n::get("spells.transmutation.disappears_singular", "disappears") == "消失了");
    REQUIRE(i18n::get("spells.transmutation.disappears_plural", "disappear") == "消失了");
    REQUIRE(i18n::get("spells.transmutation.appears_singular", "appears") == "出现了");
    REQUIRE(i18n::get("spells.transmutation.appears_plural", "appear") == "出现了");
    REQUIRE(
        i18n::get(
            "spells.transmutation.descr_main",
            "Attempts to convert items (stand over an item when casting). "
            "On failure, the item is destroyed.") ==
        "尝试转化物品（施法时站在物品上）。失败时，该物品会被摧毁。");
    REQUIRE(i18n::get("spells.transmutation.potion_chance_prefix", "Converts Potions with ") == "以");
    REQUIRE(i18n::get("spells.transmutation.manuscript_chance_prefix", "Converts Manuscripts with ") == "以");
    REQUIRE(i18n::get("spells.transmutation.chance_suffix", "% chance.") == "%几率转化药水或手稿。");
    REQUIRE(
        i18n::get(
            "spells.transmutation.weapon_chance_prefix",
            "Melee weapons with at least +1 damage (not counting any "
            "damage bonus from skills) are converted to a Potion or "
            "Manuscript, with ") ==
        "至少有+1伤害的近战武器（不包括技能带来的伤害加成）会被转化为药水或手稿，+1武器的几率为");
    REQUIRE(i18n::get("spells.transmutation.weapon_chance_plus_one", "% chance for a +1 weapon, ") == "%，+2武器的几率为");
    REQUIRE(i18n::get("spells.transmutation.weapon_chance_plus_two", "% chance for a +2 weapon, ") == "%，+3武器的几率为");
    REQUIRE(i18n::get("spells.transmutation.weapon_chance_plus_three", "% chance for a +3 weapon, etc.") == "%，依此类推。");
    REQUIRE(i18n::get("spells.space", " ") == "");
    REQUIRE(i18n::get("spells.clairvoyance.name", "Clairvoyance") == "千里眼");
    REQUIRE(
        i18n::get(
            "spells.clairvoyance.descr",
            "Reveals the presence of doors, traps, stairs, and other "
            "locations of interest in the surrounding area.") ==
        "揭示周围区域中门、陷阱、楼梯和其他值得注意地点的存在。");
    REQUIRE(i18n::get("spells.clairvoyance.reveals_items", "Also reveals items.") == "还会揭示物品。");
    REQUIRE(
        i18n::get(
            "spells.clairvoyance.reveals_items_creatures",
            "Also reveals items and creatures.") ==
        "还会揭示物品和生物。");
    REQUIRE(i18n::get("spells.blood_tempering.name", "Blood Tempering") == "鲜血淬炼");
    REQUIRE(
        i18n::get(
            "spells.blood_tempering.descr",
            "Through ardous suffering, the caster tempers their body to "
            "resist physical force (cannot be harmed by normal attacks, "
            "however other forms of damage such as fire is still "
            "harmful).") ==
        "通过艰苦的痛苦，施法者淬炼自己的身体以抵抗物理力量（不会被普通攻击伤害，但火焰等其他形式的伤害仍然有害）。");
    REQUIRE(i18n::get("spells.thorns.name", "Thorns") == "荆棘");
    REQUIRE(i18n::get("spells.thorns.return_damage_prefix", "The spell returns ") == "该法术会向攻击者反弹");
    REQUIRE(i18n::get("spells.thorns.return_damage_suffix", " damage to the attacker.") == "点伤害。");
    REQUIRE(i18n::get("spells.crimson_passage.name", "Crimson Passage") == "猩红通道");
    REQUIRE(
        i18n::get(
            "spells.crimson_passage.infinite_steps",
            "An infinite number of steps may be taken, the spell "
            "is only limited by the number of hit points.") ==
        "可以踏出无限步数，该法术只受生命值数量限制。");
    REQUIRE(
        i18n::get(
            "spells.crimson_passage.steps_suffix",
            " steps may be taken before the effect ends.") ==
        "步后效果结束。");
    REQUIRE(
        i18n::get(
            "spells.crimson_passage.recast_cancels",
            "Casting the spell again while it is already active cancels "
            "the effect (this does not drain hit points or cause shock).") ==
        "在效果已激活时再次施放该法术会取消效果（这不会消耗生命值，也不会造成震撼）。");
    REQUIRE(i18n::get("spells.sacrifice_life.name", "Sacrifice Life") == "牺牲生命");
    REQUIRE(
        i18n::get(
            "spells.sacrifice_life.descr",
            "Sacrifices the life force of the caster in order to restore "
            "the spirit. The amount restored is proportional to the life "
            "lost. A maximum of 8 hit points may be sacrificed.") ==
        "牺牲施法者的生命力以恢复精神。恢复量与失去的生命成正比。最多可以牺牲8点生命值。");
    REQUIRE(
        i18n::get(
            "spells.sacrifice_life.spirit_point_prefix",
            "For each hit point sacrificed, ") ==
        "每牺牲1点生命值，就获得");
    REQUIRE(
        i18n::get(
            "spells.sacrifice_life.spirit_point_singular_suffix",
            " spirit point is gained.") ==
        "点精神。");
    REQUIRE(
        i18n::get(
            "spells.sacrifice_life.spirit_point_plural_suffix",
            " spirit points are gained.") ==
        "点精神。");
    REQUIRE(i18n::get("spells.shed_impurity.name", "Shed Impurity") == "蜕除污秽");
    REQUIRE(
        i18n::get(
            "spells.shed_impurity.descr",
            "Purifies the caster by carving away all that is extraneous, "
            "revealing the essential core of their being.") ==
        "通过剜除一切多余之物净化施法者，显露其存在的本质核心。");
    REQUIRE(
        i18n::get(
            "spells.shed_impurity.moribund_prefix",
            "Hit points are lowered to the limit where the Moribund effect is activated "
            "(bonuses for having low hit points). "
            "This limit is at ") == "生命值会降至触发濒死效果的阈值（低生命值加成）。该阈值为");
    REQUIRE(i18n::get("spells.shed_impurity.moribund_suffix", " hit points.") == "点生命值。");
    REQUIRE(i18n::get("spells.shed_impurity.bonus_prefix", "If at least ") == "如果至少失去");
    REQUIRE(i18n::get("spells.shed_impurity.bonus_middle", " hit points are lost, then ") == "点生命值，则");
    REQUIRE(i18n::get("spells.shed_impurity.cures_basic", "weakening and poisoning are cured.") == "治愈虚弱和中毒。");
    REQUIRE(
        i18n::get(
            "spells.shed_impurity.cures_expert",
            "weakening, poisoning, infection and disease are cured.") ==
        "治愈虚弱、中毒、感染和疾病。");
    REQUIRE(
        i18n::get(
            "spells.shed_impurity.cures_master",
            "weakening, poisoning, infection, disease and slowing are cured.") ==
        "治愈虚弱、中毒、感染、疾病和迟缓。");
    REQUIRE(
        i18n::get(
            "spells.shed_impurity.cures_transcendent_prefix",
            "weakening, poisoning, infection, disease and slowing are cured. "
            "The caster is also blessed for ") == "治愈虚弱、中毒、感染、疾病和迟缓。施法者还会获得祝福，持续");
    REQUIRE(i18n::get("spells.shed_impurity.cures_transcendent_suffix", " turns.") == "回合。");
    REQUIRE(i18n::get("spells.shed_impurity.current_removed_prefix", " Currently ") == "当前会移除");
    REQUIRE(i18n::get("spells.shed_impurity.current_removed_suffix", " hit points would be removed.") == "点生命值。");
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
    REQUIRE(i18n::get("spells.appears_suffix", " appears!") == "出现了！");
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
    REQUIRE(i18n::get("item_curse.lies_upon_prefix", "A curse lies upon ") == "诅咒降临在");
    REQUIRE(i18n::get("item_curse.exclaim", "!") == "！");
    REQUIRE(
        i18n::get(
            "item_curse.growing_attached_prefix",
            "I am growing very attached to ") == "我越来越离不开");
    REQUIRE(i18n::get("item_curse.period", ".") == "。");
    REQUIRE(
        i18n::get(
            "item_curse.hold_on_prefix",
            "I am starting to think that I should hold on to ") == "我开始觉得应该永远留着");
    REQUIRE(i18n::get("item_curse.forever_suffix", ", forever...") == "……");
    REQUIRE(i18n::get("item_curse.descr_prefix", "This item is cursed, ") == "这件物品被诅咒了，");
    REQUIRE(
        i18n::get(
            "item_curse.hit_chance_penalty_descr",
            "it makes the owner less accurate (-10% hit chance with melee "
            "and ranged attacks).") == "它会使持有者不那么准确（近战和远程攻击命中率 -10%）。");
    REQUIRE(
        i18n::get(
            "item_curse.increased_shock_descr",
            "it is a burden on the mind of the owner (+10% minimum shock).") ==
        "它会成为持有者精神上的负担（最低震惊值 +10%）。");
    REQUIRE(i18n::get("item_curse.heavy_descr", "it is inexplicably heavy for its size.") == "它的重量相对于大小而言莫名其妙地重。");
    REQUIRE(
        i18n::get(
            "item_curse.heavy_curse_msg_suffix",
            " suddenly feels much heavier to carry.") == "突然变得重得多。");
    REQUIRE(i18n::get("item_curse.shrieks_suffix", " shrieks...") == "发出尖叫...");
    REQUIRE(
        i18n::get(
            "item_curse.shriek_descr",
            "it occasionally emits a disembodied voice in a horrible "
            "shrieking tone.") == "它偶尔会发出可怕尖叫般的无形声音。");
    REQUIRE(
        i18n::get(
            "item_curse.teleport_discharge_prefix",
            "I somehow sense that a burst of energy is discharged "
            "from ") == "我莫名感觉一股能量从");
    REQUIRE(i18n::get("item_curse.being_teleported", "I am being teleported...") == "我正在被传送...");
    REQUIRE(i18n::get("item_curse.teleport_descr", "it occasionally teleports the wearer.") == "它偶尔会传送佩戴者。");
    REQUIRE(i18n::get("item_curse.loud_whistling", "There is a loud whistling sound.") == "响起一阵响亮的哨声。");
    REQUIRE(i18n::get("item_curse.faint_whistling_nearer", "I hear a faint whistling sound coming nearer...") == "我听到一阵微弱的哨声正在靠近...");
    REQUIRE(
        i18n::get(
            "item_curse.summon_descr",
            "it calls deadly interdimensional beings into the existence of "
            "the owner.") == "它会将致命的异维存在召唤到持有者所在的现实。");
    REQUIRE(i18n::get("item_curse.area_bursts_into_flames", "The surrounding area suddenly burst into flames!") == "周围区域突然燃起火焰！");
    REQUIRE(i18n::get("item_curse.fire_descr", "it spontaneously sets objects around the caster on fire.") == "它会自发点燃施法者周围的物体。");
    REQUIRE(i18n::get("item_curse.cannot_read_descr", "it prevents the owner from comprehending written language.") == "它会阻止持有者理解书面语言。");
    REQUIRE(i18n::get("item_curse.light_sensitive_descr", "the owner is harmed by light.") == "持有者会被光线伤害。");
    REQUIRE(i18n::get("item_potion.vitality_name", "Vitality") == "生命力");
    REQUIRE(
        i18n::get(
            "item_potion.vitality_descr",
            "This elixir fully restores all hit points, heals all "
            "wounds, and cures blindness, deafness, poisoning, "
            "infections, disease and weakening. "
            "Also, for some duration after consuming the potion, "
            "+1 extra hit point is healed per turn, and there is "
            "10% chance per turn to heal one wound.") ==
        "这种灵药会完全恢复所有生命值，治疗所有伤口，并治愈失明、失聪、中毒、感染、疾病和虚弱。此外，饮用药水后的一段时间内，每回合会额外恢复 1 点生命值，并且每回合有 10% 几率治疗一个伤口。");
    REQUIRE(i18n::get("item_potion.spirit_name", "Spirit") == "精神");
    REQUIRE(i18n::get("item_potion.spirit_descr", "Fully restores the spirit.") == "完全恢复精神。");
    REQUIRE(i18n::get("item_potion.blindness_name", "Blindness") == "失明");
    REQUIRE(i18n::get("item_potion.blindness_descr", "Causes temporary loss of vision.") == "导致暂时失去视觉。");
    REQUIRE(i18n::get("item_potion.paralyzation_name", "Paralyzation") == "麻痹");
    REQUIRE(i18n::get("item_potion.paralyzation_descr", "Causes paralysis.") == "导致麻痹。");
    REQUIRE(i18n::get("item_potion.disease_name", "Disease") == "疾病");
    REQUIRE(i18n::get("item_potion.disease_descr", "Causes disease.") == "导致疾病。");
    REQUIRE(i18n::get("item_potion.confusion_name", "Confusion") == "混乱");
    REQUIRE(i18n::get("item_potion.confusion_descr", "Causes confusion.") == "导致混乱。");
    REQUIRE(i18n::get("item_potion.fortitude_name", "Fortitude") == "坚韧");
    REQUIRE(
        i18n::get(
            "item_potion.fortitude_descr",
            "Gives the consumer complete peace and clarity of mind.") ==
        "给予饮用者完全的平静与清明。");
    REQUIRE(i18n::get("item_potion.poison_name", "Poison") == "毒");
    REQUIRE(i18n::get("item_potion.poison_descr", "A sinister brew.") == "一剂邪恶的药水。");
    REQUIRE(i18n::get("item_potion.insight_name", "Insight") == "洞察");
    REQUIRE(
        i18n::get(
            "item_potion.insight_descr",
            "This strange concoction causes a sudden flash of intuition.") ==
        "这种奇异的混合物会带来一阵突如其来的直觉闪现。");
    REQUIRE(i18n::get("item_potion.curing_name", "Curing") == "治疗");
    REQUIRE(
        i18n::get(
            "item_potion.curing_descr",
            "Restores 3 hit points, and cures blindness, deafness, "
            "poisoning, infections, disease and weakening.") ==
        "恢复 3 点生命值，并治愈失明、失聪、中毒、感染、疾病和虚弱。");
    REQUIRE(i18n::get("item_potion.resistance_name", "Resistance") == "抗性");
    REQUIRE(
        i18n::get(
            "item_potion.resistance_descr",
            "Completely protects the consumer from electricity, fire and poison - "
            "and also prevents paralysis.") ==
        "完全保护饮用者免受电击、火焰和毒素伤害，并且防止麻痹。");
    REQUIRE(i18n::get("item_potion.descent_name", "Descent") == "下沉");
    REQUIRE(
        i18n::get(
            "item_potion.descent_descr",
            "A bizarre liquid that causes the consumer to "
            "dematerialize and sink through the ground.") ==
        "一种奇异液体，会让饮用者非物质化并沉入地下。");
    REQUIRE(i18n::get("item_potion.skill_name", "Skill") == "技艺");
    REQUIRE(
        i18n::get(
            "item_potion.skill_descr",
            "The consumer becomes more skillful "
            "(+10% to hit chance, evasion, stealth, and searching).") ==
        "饮用者变得更加熟练（命中率、闪避、潜行和搜索 +10%）。");
    REQUIRE(i18n::get("item_potion.carapace_name", "Carapace") == "甲壳");
    REQUIRE(
        i18n::get(
            "item_potion.carapace_descr",
            "Causes a tough carapace to grow over the consumer's skin "
            "providing protection against physical attacks "
            "(+3 armor points), "
            "as well as some resistance against burning "
            "(+25% chance to resist burning).") ==
        "使坚硬的甲壳覆盖饮用者的皮肤，提供对物理攻击的防护（+3 护甲点），并获得一些燃烧抗性（+25% 抵抗燃烧几率）。");
    REQUIRE(i18n::get("item_potion.blinking_name", "Blinking") == "闪现");
    REQUIRE(
        i18n::get(
            "item_potion.blinking_descr",
            "Causes the consumer to rapidly fade away from existence, "
            "and reappear again at a nearby position "
            "of their choosing.") == "使饮用者快速从现实中淡出，并在自己选择的附近位置重新出现。");
    REQUIRE(i18n::get("item_potion.burrowing_name", "Burrowing") == "掘地");
    REQUIRE(
        i18n::get(
            "item_potion.burrowing_descr",
            "Infused with the alchemically treated blood of Chthonians, "
            "it grants the consumer the ability to burrow through earth and rock "
            "(can burrow through walls and rubble).") ==
        "注入了经过炼金处理的钻地魔血液，赋予饮用者穿过泥土和岩石掘进的能力（可以穿过墙壁和瓦砾）。");
    REQUIRE(i18n::get("item_potion.article_a_space", "a ") == "");
    REQUIRE(i18n::get("item_potion.article_an_space", "an ") == "");
    REQUIRE(i18n::get("item_potion.appearance_golden", "Golden") == "金色");
    REQUIRE(i18n::get("item_potion.appearance_yellow", "Yellow") == "黄色");
    REQUIRE(i18n::get("item_potion.appearance_dark", "Dark") == "暗色");
    REQUIRE(i18n::get("item_potion.appearance_black", "Black") == "黑色");
    REQUIRE(i18n::get("item_potion.appearance_oily", "Oily") == "油状");
    REQUIRE(i18n::get("item_potion.appearance_smoky", "Smoky") == "烟雾状");
    REQUIRE(i18n::get("item_potion.appearance_slimy", "Slimy") == "黏滑");
    REQUIRE(i18n::get("item_potion.appearance_green", "Green") == "绿色");
    REQUIRE(i18n::get("item_potion.appearance_fiery", "Fiery") == "火红");
    REQUIRE(i18n::get("item_potion.appearance_murky", "Murky") == "浑浊");
    REQUIRE(i18n::get("item_potion.appearance_muddy", "Muddy") == "泥色");
    REQUIRE(i18n::get("item_potion.appearance_violet", "Violet") == "紫罗兰色");
    REQUIRE(i18n::get("item_potion.appearance_orange", "Orange") == "橙色");
    REQUIRE(i18n::get("item_potion.appearance_watery", "Watery") == "水状");
    REQUIRE(i18n::get("item_potion.appearance_metallic", "Metallic") == "金属色");
    REQUIRE(i18n::get("item_potion.appearance_clear", "Clear") == "透明");
    REQUIRE(i18n::get("item_potion.appearance_misty", "Misty") == "雾状");
    REQUIRE(i18n::get("item_potion.appearance_bloody", "Bloody") == "血色");
    REQUIRE(i18n::get("item_potion.appearance_magenta", "Magenta") == "品红色");
    REQUIRE(i18n::get("item_potion.appearance_clotted", "Clotted") == "凝块状");
    REQUIRE(i18n::get("item_potion.appearance_moldy", "Moldy") == "霉斑");
    REQUIRE(i18n::get("item_potion.appearance_frothy", "Frothy") == "泡沫状");
    REQUIRE(i18n::get("item_potion.unidentified_suffix", " Potion") == "药水");
    REQUIRE(i18n::get("item_potion.unidentified_plural_suffix", " Potions") == "药水");
    REQUIRE(i18n::get("item_potion.real_name_prefix", "Potion of ") == "药水：");
    REQUIRE(i18n::get("item_potion.real_name_plural_prefix", "Potions of ") == "药水：");
    REQUIRE(i18n::get("item_potion.real_name_a_prefix", "a Potion of ") == "药水：");
    REQUIRE(i18n::get("item_potion.feel_more_at_ease", "I feel more at ease.") == "我感到安心多了。");
    REQUIRE(i18n::get("item_potion.feel_insightful", "I feel insightful.") == "我感到洞察力涌现。");
    REQUIRE(i18n::get("item_potion.feel_fine", "I feel fine.") == "我感觉很好。");
    REQUIRE(i18n::get("item_potion.sinking_sensation_disappears", "I feel a faint sinking sensation, but it soon disappears...") == "我感到一阵微弱的下沉感，但很快就消失了……");
    REQUIRE(i18n::get("item_potion.fade_out_and_reappear", "I fade out of existence and reappear.") == "我从现实中淡出，然后又重新出现。");
    REQUIRE(i18n::get("item_weapon.looks_shocked_suffix", " looks shocked!") == "看起来震惊了！");
    REQUIRE(i18n::get("item_weapon.bows_before_me_suffix", " bows before me.") == "向我俯首。");
    REQUIRE(i18n::get("item_weapon.assailed_by_dark_energy_suffix", " is assailed by dark energy.") == "被黑暗能量袭击了。");
    REQUIRE(i18n::get("item_weapon.energy_feed_prefix", "The ") == "这把");
    REQUIRE(i18n::get("item_weapon.energy_feed_suffix", " feeds on my energy!") == "汲取了我的能量！");
    REQUIRE(i18n::get("item_weapon.unseen_target_it", "It") == "它");
    REQUIRE(i18n::get("item.discovered_prefix", "I have discovered ") == "我发现了");
    REQUIRE(i18n::get("item.exclamation_mark", "!") == "！");
    REQUIRE(i18n::get("item.discovered_history_prefix", "Discovered ") == "发现了");
    REQUIRE(i18n::get("item.hit_suffix", " hit") == "命中");
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


    // Coverage restored from i18n-obsolete plus current missing locale keys.
    require_translation(
        "game_over_summary.none",
        "无");
    require_translation(
        "insanity.draws_nearer_prefix",
        "疯狂逼近……");
    require_translation(
        "insanity.reduce_xp_start",
        "多亏心智的仁慈，一些过去的经历被遗忘了（-25% 经验）。");
    require_translation(
        "insanity.scream_shriek",
        "我发出一声惊恐的尖叫。");
    require_translation(
        "insanity.scream_terror",
        "我恐惧地尖叫。");
    require_translation(
        "item_device.condition_breaking",
        "几乎坏掉了。");
    require_translation(
        "item_device.condition_fine",
        "状况良好。");
    require_translation(
        "item_device.condition_prefix",
        "它似乎");
    require_translation(
        "item_device.condition_shoddy",
        "状况粗劣。");
    require_translation(
        "item_device.descr_blaster",
        "启动后，这个装置会用地狱之力轰击一个可见的敌对生物。");
    require_translation(
        "item_device.descr_force_field",
        "启动后，这个装置会在使用者周围构建一道临时的不透明屏障，阻挡所有物质。屏障只能在空地中生成（即没有生物、墙壁等占据的空间）。");
    require_translation(
        "item_device.descr_rejuvenator",
        "启动后，这个装置会治愈所有伤口和身体疾病。然而，这个过程非常痛苦且侵入性很强，并会给使用者带来巨大的精神震惊。");
    require_translation(
        "item_device.descr_sentry_drone",
        "启动后，这个装置会“活过来”并守卫使用者。");
    require_translation(
        "item_device.descr_translocator",
        "启动后，这个装置会将所有可见敌人传送到不同位置。");
    require_translation(
        "item_device.name_info_breaking",
        "（将坏）");
    require_translation(
        "item_device.name_info_fine",
        "（良好）");
    require_translation(
        "item_device.name_info_shoddy",
        "（粗劣）");
    require_translation(
        "item_device.teleported_suffix",
        "被传送了。");
    require_translation(
        "item_explosive.throw_lit_dynamite",
        "我投掷了一根点燃的炸药。");
    require_translation(
        "item_explosive.throw_lit_flare",
        "我投掷了一支点燃的照明棒。");
    require_translation(
        "item_explosive.throw_lit_molotov",
        "我投掷了一瓶点燃的燃烧瓶。");
    require_translation(
        "item_explosive.throw_smoke_grenade",
        "我投掷了一枚烟雾手雷。");
    require_translation(
        "item_rod.a_rod_of_prefix",
        "魔杖：");
    require_translation(
        "item_rod.displacement_descr",
        "激活时，此装置会将使用者短距离移动。");
    require_translation(
        "item_rod.displacement_name",
        "置换");
    require_translation(
        "item_rod.ellipsis",
        "...");
    require_translation(
        "item_rod.look_chromium",
        "铬");
    require_translation(
        "item_rod.look_chromium_a",
        "铬");
    require_translation(
        "item_rod.look_cobalt",
        "钴");
    require_translation(
        "item_rod.look_cobalt_a",
        "钴");
    require_translation(
        "item_rod.look_copper",
        "铜");
    require_translation(
        "item_rod.look_copper_a",
        "铜");
    require_translation(
        "item_rod.look_gallium",
        "镓");
    require_translation(
        "item_rod.look_gallium_a",
        "镓");
    require_translation(
        "item_rod.look_golden",
        "金");
    require_translation(
        "item_rod.look_golden_a",
        "金");
    require_translation(
        "item_rod.look_iron",
        "铁");
    require_translation(
        "item_rod.look_iron_a",
        "铁");
    require_translation(
        "item_rod.look_lead",
        "铅");
    require_translation(
        "item_rod.look_lead_a",
        "铅");
    require_translation(
        "item_rod.look_lithium",
        "锂");
    require_translation(
        "item_rod.look_lithium_a",
        "锂");
    require_translation(
        "item_rod.look_magnesium",
        "镁");
    require_translation(
        "item_rod.look_magnesium_a",
        "镁");
    require_translation(
        "item_rod.look_nickel",
        "镍");
    require_translation(
        "item_rod.look_nickel_a",
        "镍");
    require_translation(
        "item_rod.look_platinum",
        "铂");
    require_translation(
        "item_rod.look_platinum_a",
        "铂");
    require_translation(
        "item_rod.look_silver",
        "银");
    require_translation(
        "item_rod.look_silver_a",
        "银");
    require_translation(
        "item_rod.look_tin",
        "锡");
    require_translation(
        "item_rod.look_tin_a",
        "锡");
    require_translation(
        "item_rod.look_titanium",
        "钛");
    require_translation(
        "item_rod.look_titanium_a",
        "钛");
    require_translation(
        "item_rod.look_tungsten",
        "钨");
    require_translation(
        "item_rod.look_tungsten_a",
        "钨");
    require_translation(
        "item_rod.look_zinc",
        "锌");
    require_translation(
        "item_rod.look_zinc_a",
        "锌");
    require_translation(
        "item_rod.look_zirconium",
        "锆");
    require_translation(
        "item_rod.look_zirconium_a",
        "锆");
    require_translation(
        "item_rod.rod_of_prefix",
        "魔杖：");
    require_translation(
        "item_rod.rod_suffix",
        "魔杖");
    require_translation(
        "item_rod.rods_of_prefix",
        "魔杖：");
    require_translation(
        "item_rod.rods_suffix",
        "魔杖");
    require_translation(
        "item_rod.tried",
        "（已试）");
    require_translation(
        "item_rod.turns_left_open",
        "（");
    require_translation(
        "item_rod.turns_left_suffix",
        "回合）");
    require_translation(
        "player_bon.background.exorcist.bonus_trait_levels_and_separator",
        "和");
    require_translation(
        "player_bon.background.exorcist.bonus_trait_levels_prefix",
        "在角色等级");
    require_translation(
        "player_bon.background.exorcist.bonus_trait_levels_separator",
        "、");
    require_translation(
        "player_bon.background.exorcist.bonus_trait_levels_suffix",
        "时获得一个额外特质。");
    require_translation(
        "player_bon.background.exorcist.destroy_sacred_tools_descr",
        "不能使用手稿、祭坛、独石或锣，而是通过摧毁它们获得经验和热忱（手稿会在拾取时被摧毁）。热忱可用于施放法术；当精神点不足以施放时，会自动消耗这些点数。");
    require_translation(
        "player_bon.background.exorcist.holy_symbol_descr",
        "开局携带一个圣符，可恢复精神点，并赋予对精神震惊和恐惧的抗性。");
    require_translation(
        "player_bon.background.flagellant.blood_spell_shock_descr",
        "施放已记忆的鲜血领域法术时，受到的精神震惊 -25%。");
    require_translation(
        "player_bon.background.flagellant.blood_upgrade_levels_and_separator",
        "和");
    require_translation(
        "player_bon.background.flagellant.blood_upgrade_levels_prefix",
        "专精于鲜血领域法术。在角色等级");
    require_translation(
        "player_bon.background.flagellant.blood_upgrade_levels_suffix",
        "时，该领域的所有法术都以更高技能等级施放。");
    require_translation(
        "player_bon.background.flagellant.moribund_descr",
        "因受到伤害而生命值降至 6 点或以下时，会获得濒死状态 5-7 回合（+3 近战伤害、+30% 近战命中率、+3 护甲点）。");
    require_translation(
        "player_bon.background.flagellant.no_damage_shock_descr",
        "因受伤而受到的精神震惊为零。");
    require_translation(
        "player_bon.background.flagellant.torture_collar_descr",
        "佩戴无法取下的苦刑项圈；行走需要额外回合，潜行和闪避降低 20%。然而，佩戴项圈会让鞭笞者对肉体痛苦更加麻木，护甲增加 3 点。");
    require_translation(
        "player_bon.background.ghoul.corpse_feeding_descr",
        "不会再生生命值，也不能使用医疗器械，而是通过吞食尸体来治疗（在尸体上等待即可吞食）。");
    require_translation(
        "player_bon.background.ghoul.darkness_shock_descr",
        "因看见怪物和站在黑暗中受到的精神震惊 -50%，但从光照获得的震惊降低效果也减半。");
    require_translation(
        "player_bon.background.ghoul.darkvision_descr",
        "可以在黑暗中视物。");
    require_translation(
        "player_bon.background.ghoul.disease_immunity_descr",
        "免疫疾病和感染。");
    require_translation(
        "player_bon.background.ghoul.frenzy_descr",
        "可以随意激发狂暴，并且狂暴结束时不会变得虚弱。");
    require_translation(
        "player_bon.background.ghoul.ghoul_allies_descr",
        "所有食尸鬼都是盟友。");
    require_translation(
        "player_bon.background.ghoul.hit_points_descr",
        "+8 点生命值。");
    require_translation(
        "player_bon.background.ghoul.ranged_penalty_descr",
        "使用火器和投掷武器时命中率 -15%。");
    require_translation(
        "player_bon.background.ghoul.sprain_immunity_descr",
        "不会扭伤。");
    require_translation(
        "player_bon.background.occultist.bone_charms_descr",
        "开局携带数个骨符，可用于获得法术抗性或驱散符印（地面上的“奇异形状”）。");
    require_translation(
        "player_bon.background.occultist.choose_domain_descr",
        "在角色创建时选择一个特定法术领域作为背景，这会决定起始法术。");
    require_translation(
        "player_bon.background.occultist.domain.channeling_prefix",
        "你曾涉猎暴烈能量的导引，");
    require_translation(
        "player_bon.background.occultist.domain.corruption_prefix",
        "你曾涉猎枯萎与腐化的法术，");
    require_translation(
        "player_bon.background.occultist.domain.illusion_prefix",
        "你曾涉猎幻象的施放，");
    require_translation(
        "player_bon.background.occultist.domain.knowledge_prefix",
        "并对");
    require_translation(
        "player_bon.background.occultist.domain.knowledge_suffix",
        "有基础了解。");
    require_translation(
        "player_bon.background.occultist.domain.mind_prefix",
        "你曾涉猎启示、预见与意志的学科，");
    require_translation(
        "player_bon.background.occultist.domain.time_prefix",
        "你曾涉猎时间与因果的操纵，");
    require_translation(
        "player_bon.background.occultist.domain.warding_prefix",
        "你曾涉猎防护魔法，");
    require_translation(
        "player_bon.background.occultist.spell_domain_traits_descr",
        "可以获得特质来提高各个法术领域的技能等级。");
    require_translation(
        "player_bon.background.occultist.spirit_points_descr",
        "+3 点精神（在“坚定精神”之外）。");
    require_translation(
        "player_bon.background.occultist.strange_item_shock_descr",
        "施放已记忆的法术，以及使用或鉴定药水、手稿等奇异物品时，受到的精神震惊 -50%（在“冷静”之外）。");
    require_translation(
        "player_bon.background.rogue.creature_awareness_descr",
        "对其他生物存在的感知保持更久。");
    require_translation(
        "player_bon.background.rogue.mind_cloud_artifact_descr",
        "已获得一件神器，可以蒙蔽所有敌人的心智，使它们忘记使用者的存在。");
    require_translation(
        "player_bon.background.rogue.passive_shock_descr",
        "随时间被动获得的精神震惊减少 25%。");
    require_translation(
        "player_bon.background.rogue.sense_uniques_descr",
        "可以感知独特怪物或强大神器的存在。");
    require_translation(
        "player_bon.background.rogue.spot_hidden_descr",
        "发现隐藏怪物、门和陷阱的几率 +10%。");
    require_translation(
        "player_bon.background.war_vet.armor_maintenance_descr",
        "护甲在损坏前可维持两倍时间。");
    require_translation(
        "player_bon.background.war_vet.flak_jacket_descr",
        "开局携带一件防弹背心。");
    require_translation(
        "player_bon.background.war_vet.instant_prepare_descr",
        "立即切换到预备武器。");
    require_translation(
        "player_bon.trait.absorption.descr",
        "每当法术护盾被敌对法术终止时，恢复1-6点精神（法术护盾由精神特质或法术护盾法术赋予）");
    require_translation(
        "player_bon.trait.absorption.title",
        "吸收");
    require_translation(
        "player_bon.trait.adept_marksman.descr",
        "使用火器和投掷武器时 +10% 命中率，并且最低伤害 +1（不能提高最大伤害）");
    require_translation(
        "player_bon.trait.adept_marksman.title",
        "熟练射手");
    require_translation(
        "player_bon.trait.adept_melee.descr",
        "+10% 命中率，近战攻击 +1 伤害");
    require_translation(
        "player_bon.trait.adept_melee.title",
        "熟练近战斗士");
    require_translation(
        "player_bon.trait.adept_of_channeling.descr",
        "专精于暴烈能量的导引。导能法术以更高技能等级施放。");
    require_translation(
        "player_bon.trait.adept_of_channeling.title",
        "导能熟手");
    require_translation(
        "player_bon.trait.adept_of_corruption.descr",
        "专精于腐化和枯萎。腐化法术以更高技能等级施放。");
    require_translation(
        "player_bon.trait.adept_of_corruption.title",
        "腐化熟手");
    require_translation(
        "player_bon.trait.adept_of_illusion.descr",
        "专精于施放幻象。幻象法术以更高技能等级施放。");
    require_translation(
        "player_bon.trait.adept_of_illusion.title",
        "幻象熟手");
    require_translation(
        "player_bon.trait.adept_of_the_mind.descr",
        "专精于知识、预见和意志。心灵法术以更高技能等级施放。");
    require_translation(
        "player_bon.trait.adept_of_the_mind.title",
        "心灵熟手");
    require_translation(
        "player_bon.trait.adept_of_time.descr",
        "专精于时间和因果的操纵。时间法术以更高技能等级施放。");
    require_translation(
        "player_bon.trait.adept_of_time.title",
        "时间熟手");
    require_translation(
        "player_bon.trait.adept_of_warding.descr",
        "专精于防护魔法。守护法术以更高技能等级施放。");
    require_translation(
        "player_bon.trait.adept_of_warding.title",
        "守护熟手");
    require_translation(
        "player_bon.trait.available_fp_prefix",
        "和");
    require_translation(
        "player_bon.trait.available_fp_suffix",
        "点热忱");
    require_translation(
        "player_bon.trait.available_sp_period",
        "。");
    require_translation(
        "player_bon.trait.available_sp_prefix",
        "你当前有");
    require_translation(
        "player_bon.trait.available_sp_suffix",
        "点精神");
    require_translation(
        "player_bon.trait.callous.title",
        "冷硬");
    require_translation(
        "player_bon.trait.cast_bless_i.title",
        "施放祝福");
    require_translation(
        "player_bon.trait.cast_bless_ii.title",
        "施放祝福 II");
    require_translation(
        "player_bon.trait.cast_cleansing_fire_i.title",
        "施放净化之火");
    require_translation(
        "player_bon.trait.cast_cleansing_fire_ii.title",
        "施放净化之火 II");
    require_translation(
        "player_bon.trait.cast_heal_i.title",
        "施放治疗");
    require_translation(
        "player_bon.trait.cast_heal_ii.title",
        "施放治疗 II");
    require_translation(
        "player_bon.trait.cast_light_i.title",
        "施放光亮");
    require_translation(
        "player_bon.trait.cast_light_ii.title",
        "施放光亮 II");
    require_translation(
        "player_bon.trait.cast_sanctuary_i.title",
        "施放庇护");
    require_translation(
        "player_bon.trait.cast_sanctuary_ii.title",
        "施放庇护 II");
    require_translation(
        "player_bon.trait.cast_see_invisible_i.title",
        "施放识破隐形");
    require_translation(
        "player_bon.trait.cast_see_invisible_ii.title",
        "施放识破隐形 II");
    require_translation(
        "player_bon.trait.cool_headed.descr",
        "+20% 精神震惊抗性");
    require_translation(
        "player_bon.trait.cool_headed.title",
        "冷静");
    require_translation(
        "player_bon.trait.courageous.title",
        "勇敢");
    require_translation(
        "player_bon.trait.crippling_strikes.descr",
        "你的近战攻击有 60% 几率使目标生物虚弱 2-3 回合（使其近战伤害减半）");
    require_translation(
        "player_bon.trait.crippling_strikes.title",
        "致残打击");
    require_translation(
        "player_bon.trait.dexterous.descr",
        "+25% 闪避攻击几率");
    require_translation(
        "player_bon.trait.dexterous.title",
        "灵巧");
    require_translation(
        "player_bon.trait.elec_incl.descr",
        "魔杖充能速度加倍，奇异装置更不容易故障或损坏，电提灯持续时间加倍，电击武器伤害+1");
    require_translation(
        "player_bon.trait.elec_incl.title",
        "电气亲和");
    require_translation(
        "player_bon.trait.elusive.descr",
        "生物记住你的时间只有正常持续时间的一半（向上取整）。");
    require_translation(
        "player_bon.trait.elusive.title",
        "难以捉摸");
    require_translation(
        "player_bon.trait.enthusiasm.descr",
        "使濒死效果的所有加成翻倍");
    require_translation(
        "player_bon.trait.enthusiasm.title",
        "狂热");
    require_translation(
        "player_bon.trait.expert_marksman.title",
        "专家射手");
    require_translation(
        "player_bon.trait.expert_melee.title",
        "专家近战斗士");
    require_translation(
        "player_bon.trait.fearless.descr",
        "你不会变得恐惧，+10% 精神震惊抗性");
    require_translation(
        "player_bon.trait.fearless.title",
        "无畏");
    require_translation(
        "player_bon.trait.foul.descr",
        "+1爪击伤害，用爪攻击时，恶毒蠕虫偶尔会从受害者尸体中爆出并攻击你的敌人");
    require_translation(
        "player_bon.trait.foul.title",
        "污秽");
    require_translation(
        "player_bon.trait.gain_cast_cost_prefix",
        "此法术消耗");
    require_translation(
        "player_bon.trait.gain_cast_cost_suffix",
        "点精神施放。");
    require_translation(
        "player_bon.trait.gain_cast_descr_separator",
        "；");
    require_translation(
        "player_bon.trait.gain_cast_line_separator",
        "");
    require_translation(
        "player_bon.trait.gain_cast_prefix",
        "获得施放“");
    require_translation(
        "player_bon.trait.gain_cast_skill_prefix",
        "，以");
    require_translation(
        "player_bon.trait.gain_cast_skill_suffix",
        "等级");
    require_translation(
        "player_bon.trait.gain_cast_suffix",
        "”的能力");
    require_translation(
        "player_bon.trait.galvanization.descr",
        "施放任何鲜血领域法术会赋予再生4-6回合（每回合额外再生+1生命值），前提是施法损失了生命值");
    require_translation(
        "player_bon.trait.galvanization.title",
        "激发");
    require_translation(
        "player_bon.trait.healer.descr",
        "使用医疗器械只需要一半的正常时间和资源");
    require_translation(
        "player_bon.trait.healer.title",
        "治疗者");
    require_translation(
        "player_bon.trait.imperceptible.title",
        "难以察觉");
    require_translation(
        "player_bon.trait.indomitable_fury.descr",
        "狂暴时，你免疫伤口，爪击会造成恐惧");
    require_translation(
        "player_bon.trait.indomitable_fury.title",
        "不屈狂怒");
    require_translation(
        "player_bon.trait.lithe.title",
        "轻盈");
    require_translation(
        "player_bon.trait.master_marksman.title",
        "大师射手");
    require_translation(
        "player_bon.trait.master_melee.title",
        "大师近战斗士");
    require_translation(
        "player_bon.trait.master_of_channeling.descr",
        "掌握暴烈能量的导引。导能法术以更高技能等级施放。");
    require_translation(
        "player_bon.trait.master_of_channeling.title",
        "导能大师");
    require_translation(
        "player_bon.trait.master_of_corruption.descr",
        "掌握腐化和枯萎。腐化法术以更高技能等级施放。");
    require_translation(
        "player_bon.trait.master_of_corruption.title",
        "腐化大师");
    require_translation(
        "player_bon.trait.master_of_illusion.descr",
        "掌握幻象的施放。幻象法术以更高技能等级施放。");
    require_translation(
        "player_bon.trait.master_of_illusion.title",
        "幻象大师");
    require_translation(
        "player_bon.trait.master_of_the_mind.descr",
        "掌握知识、预见和意志。心灵法术以更高技能等级施放，并且你还能感知物品和生物。");
    require_translation(
        "player_bon.trait.master_of_the_mind.title",
        "心灵大师");
    require_translation(
        "player_bon.trait.master_of_time.descr",
        "掌握时间和因果的操纵。时间法术以更高技能等级施放。");
    require_translation(
        "player_bon.trait.master_of_time.title",
        "时间大师");
    require_translation(
        "player_bon.trait.master_of_warding.descr",
        "掌握防护魔法。守护法术以更高技能等级施放。");
    require_translation(
        "player_bon.trait.master_of_warding.title",
        "守护大师");
    require_translation(
        "player_bon.trait.meditative.descr",
        "施加专注状态，使下一个法术无需花费回合即可施放，且施法消耗减少1点；施放一个法术后需要125-150回合恢复此状态");
    require_translation(
        "player_bon.trait.meditative.title",
        "冥想者");
    require_translation(
        "player_bon.trait.memento_mori.descr",
        "将濒死状态阈值提高到8点生命值，并将效果持续时间增加50%（向下取整）");
    require_translation(
        "player_bon.trait.memento_mori.title",
        "记住死亡");
    require_translation(
        "player_bon.trait.mighty_spirit.descr",
        "+2点精神，精神再生速度提高，阻挡一个法术后需要25-50回合恢复法术抗性");
    require_translation(
        "player_bon.trait.mighty_spirit.title",
        "强大精神");
    require_translation(
        "player_bon.trait.prolonged_life.descr",
        "受到任何致命伤害时改为消耗你的热忱点数");
    require_translation(
        "player_bon.trait.prolonged_life.title",
        "延续生命");
    require_translation(
        "player_bon.trait.rapid_recoverer.descr",
        "你每三个回合恢复1点生命值");
    require_translation(
        "player_bon.trait.rapid_recoverer.title",
        "快速恢复者");
    require_translation(
        "player_bon.trait.ravenous.descr",
        "用爪攻击时，你偶尔会吞食活体受害者");
    require_translation(
        "player_bon.trait.ravenous.title",
        "贪食");
    require_translation(
        "player_bon.trait.resistant.descr",
        "+25%抵抗燃烧、中毒和麻痹的几率，并且这些效果的持续时间减半");
    require_translation(
        "player_bon.trait.resistant.title",
        "抗性");
    require_translation(
        "player_bon.trait.rugged.title",
        "强健");
    require_translation(
        "player_bon.trait.ruthless.descr",
        "+100%背刺伤害");
    require_translation(
        "player_bon.trait.ruthless.title",
        "无情");
    require_translation(
        "player_bon.trait.sage.descr",
        "专注时，施放法术不消耗精神点，且恢复专注状态的时间缩短为75-100回合");
    require_translation(
        "player_bon.trait.sage.title",
        "贤者");
    require_translation(
        "player_bon.trait.self_aware.descr",
        "你不会变得混乱，并会显示状态效果的剩余回合数");
    require_translation(
        "player_bon.trait.self_aware.title",
        "自知");
    require_translation(
        "player_bon.trait.silent.descr",
        "你的所有近战攻击都是无声的（无论使用什么武器），打开或关闭门、涉水而过时也不会惊动生物");
    require_translation(
        "player_bon.trait.silent.title",
        "静默");
    require_translation(
        "player_bon.trait.steady_aimer.descr",
        "站立不动会让下一回合的远程攻击造成最大伤害并+10%命中率，除非受到伤害");
    require_translation(
        "player_bon.trait.steady_aimer.title",
        "稳定瞄准");
    require_translation(
        "player_bon.trait.stealthy.descr",
        "+45% 避免被视觉发现的几率");
    require_translation(
        "player_bon.trait.stealthy.title",
        "潜行");
    require_translation(
        "player_bon.trait.stout_spirit.descr",
        "+2点精神，精神再生速度提高，你可以抵抗有害法术（阻挡一个法术后需要125-150回合恢复法术抗性）");
    require_translation(
        "player_bon.trait.stout_spirit.title",
        "坚定精神");
    require_translation(
        "player_bon.trait.strong_backed.descr",
        "+50%负重上限");
    require_translation(
        "player_bon.trait.strong_backed.title",
        "强壮背负");
    require_translation(
        "player_bon.trait.strong_spirit.descr",
        "+2点精神，精神再生速度提高，阻挡一个法术后需要75-100回合恢复法术抗性");
    require_translation(
        "player_bon.trait.strong_spirit.title",
        "强韧精神");
    require_translation(
        "player_bon.trait.survivalist.descr",
        "你不会患病，并且只有一半伤口数计入惩罚（向下取整；也就是计算战斗、生命值和再生惩罚时伤口数减半，行走变慢从6处伤口而不是3处开始，死亡从10处伤口而不是5处开始）");
    require_translation(
        "player_bon.trait.survivalist.title",
        "生存专家");
    require_translation(
        "player_bon.trait.thick_skinned.descr",
        "+1点护甲（物理伤害减少1点）");
    require_translation(
        "player_bon.trait.thick_skinned.title",
        "厚皮");
    require_translation(
        "player_bon.trait.tough.descr",
        "+6点生命值，+10%抵抗燃烧、中毒和麻痹的几率，踢击时更不容易扭伤，更容易成功完成需要力量的物体互动（例如撞开东西）");
    require_translation(
        "player_bon.trait.tough.title",
        "坚韧");
    require_translation(
        "player_bon.trait.toxic.descr",
        "+1爪击伤害，你免疫中毒，并且用爪攻击经常会使受害者中毒");
    require_translation(
        "player_bon.trait.toxic.title",
        "剧毒");
    require_translation(
        "player_bon.trait.treasure_hunter.descr",
        "你往往会找到更多物品");
    require_translation(
        "player_bon.trait.treasure_hunter.title",
        "寻宝者");
    require_translation(
        "player_bon.trait.unbreakable.title",
        "不屈");
    require_translation(
        "player_bon.trait.undead_bane.descr",
        "对所有亡灵怪物的近战和远程攻击伤害+2，对以太亡灵怪物的命中率+50%");
    require_translation(
        "player_bon.trait.undead_bane.title",
        "亡灵克星");
    require_translation(
        "player_bon.trait.vicious.descr",
        "+100%背刺伤害（在普通+50%之外）");
    require_translation(
        "player_bon.trait.vicious.title",
        "凶狠");
    require_translation(
        "player_bon.trait.vigilant.descr",
        "你总能察觉附近的生物");
    require_translation(
        "player_bon.trait.vigilant.title",
        "警觉");
    require_translation(
        "property.all_wounds_healed",
        "我所有的伤口都愈合了！");
    require_translation(
        "property.crave_astral_opium",
        "我渴望星界鸦片！！");
    require_translation(
        "property.infection_getting_worse",
        "我的感染正在恶化！");
    require_translation(
        "property.mon_struggles_in_pain_suffix",
        "痛苦地挣扎！");
    require_translation(
        "property.mon_struggles_pull_free_suffix",
        "挣扎着想拔脱。");
    require_translation(
        "property.mon_struggles_tear_free_suffix",
        "挣扎着想挣脱。");
    require_translation(
        "property.one_wound_healed",
        "一道伤口愈合了。");
    require_translation(
        "property.resist_electric_player",
        "我感到一阵微弱的刺痛。");
    require_translation(
        "property.resist_fire_player",
        "我感到温暖。");
    require_translation(
        "property.resist_physical_player",
        "我抵抗了伤害。");
    require_translation(
        "property.resist_seems_unaffected",
        "{}似乎不受影响。");
    require_translation(
        "property.resist_seems_unharmed",
        "{}似乎没有受伤。");
    require_translation(
        "property.struggle_tear_out_spike",
        "我挣扎着想拔出尖刺！");
    require_translation(
        "spells.exclamation",
        "！");
    require_translation(
        "terrain.pick_up_query_prefix",
        "捡起");
    require_translation(
        "terrain_door.attempt_to_open_it",
        "要尝试打开它吗？");
    require_translation(
        "terrain_door.blocked_prefix",
        "那扇");
    require_translation(
        "terrain_door.blocked_suffix",
        "被挡住了。");
    require_translation(
        "terrain_door.break_open",
        "打开");
    require_translation(
        "terrain_door.break_to_floor",
        "到地上");
    require_translation(
        "terrain_door.currently_being_opened_cannot_close",
        "门正在被打开，无法关闭。");
    require_translation(
        "terrain_door.fail_close_suffix",
        "，但没能关上。");
    require_translation(
        "terrain_door.fumble_blindly_close_prefix",
        "我盲目地摸索着一扇");
    require_translation(
        "terrain_door.fumbles_blindly_fail_open_a",
        "盲目地摸索着，但没能打开一扇");
    require_translation(
        "terrain_door.hear_door_crashing_open",
        "我听到一扇门被撞开！");
    require_translation(
        "terrain_door.legend",
        "门");
    require_translation(
        "terrain_door.legend_metal",
        "门（金属）");
    require_translation(
        "terrain_door.legend_warded",
        "门（受守护）");
    require_translation(
        "terrain_door.name_article_a",
        "一扇");
    require_translation(
        "terrain_door.name_article_an",
        "一扇");
    require_translation(
        "terrain_door.name_article_the",
        "那扇");
    require_translation(
        "terrain_door.name_barred_gate",
        "铁栅门");
    require_translation(
        "terrain_door.name_metal_door",
        "金属门");
    require_translation(
        "terrain_door.name_modifier_burning",
        "燃烧的");
    require_translation(
        "terrain_door.name_modifier_open",
        "打开的");
    require_translation(
        "terrain_door.name_modifier_stuck",
        "卡住的");
    require_translation(
        "terrain_door.name_short_door",
        "门");
    require_translation(
        "terrain_door.name_unwarded_door",
        "失去守护的门");
    require_translation(
        "terrain_door.name_warded_door",
        "受守护的门");
    require_translation(
        "terrain_door.name_wooden_door",
        "木门");
    require_translation(
        "terrain_door.open_query_prefix",
        "打开");
    require_translation(
        "terrain_door.query_suffix",
        "？");
    require_translation(
        "terrain_door.shotgun_blown_to_pieces_suffix",
        "被轰成碎片！");
    require_translation(
        "terrain_door.something_blocking_prefix",
        "有东西挡住了");
    require_translation(
        "throwing.creature_hit",
        "有生物被击中了。");
    require_translation(
        "throwing.is_hit_suffix",
        "被击中了。");
    require_translation(
        "throwing.monster_throws",
        "投掷了");
    require_translation(
        "throwing.period",
        "。");
    require_translation(
        "throwing.player_throw_prefix",
        "我投掷了");
    require_translation(
        "throwing.unseen_creature",
        "有个看不见的生物");

    const auto input_mode_descr =
        i18n::get("option.input_mode.descr", "");
    REQUIRE(input_mode_descr.find("\\n") == std::string::npos);
    REQUIRE(input_mode_descr.find("\n\n{COLOR_LIGHT_WHITE}默认：") != std::string::npos);

    const auto path = messages::resolved_path("menu_quotes.txt");

    REQUIRE(path.find("locale/zh_CN/messages/menu_quotes.txt") != std::string::npos);
    REQUIRE(std::filesystem::exists(path));

    const auto manual_path = i18n::localized_file("manual.txt", "manual.txt");

    REQUIRE(manual_path.find("locale/zh_CN/manual.txt") != std::string::npos);
    REQUIRE(std::filesystem::exists(manual_path));

    std::ifstream manual_file(manual_path);
    REQUIRE(manual_file.is_open());

    std::string first_line;
    std::getline(manual_file, first_line);
    REQUIRE(first_line == "--------------------------------------------------------------------------------");

    std::string title_line;
    std::getline(manual_file, title_line);
    REQUIRE(title_line == "游戏命令");
}

TEST_CASE("Monster XML player-facing text has i18n keys")
{
    config::set_language("zh_CN");
    i18n::reload();

    const std::vector<std::string> tags = {
        "name_a",
        "name_the",
        "corpse_name_a",
        "corpse_name_the",
        "description",
        "wary_message",
        "smell_message",
        "aware_message_seen",
        "aware_message_hidden",
        "spell_message_sound",
        "spell_message_visual",
        "death_message"};

    std::ifstream file(paths::data_dir() + "monsters.xml");
    REQUIRE(file.is_open());

    int nr_keyed_entries = 0;
    std::string line;

    while (std::getline(file, line)) {
        for (const auto& tag : tags) {
            const std::regex text_tag_regex(
                "^\\s*<" + tag + "([^>]*)>(.*)</" + tag + ">$");

            std::smatch match;

            if (!std::regex_match(line, match, text_tag_regex)) {
                continue;
            }

            if (match[2].str().empty()) {
                break;
            }

            const std::string attrs = match[1].str();
            const std::regex key_regex("i18n_key=\"([^\"]+)\"");
            std::smatch key_match;

            INFO("monster XML line: " << line);
            REQUIRE(std::regex_search(attrs, key_match, key_regex));

            const std::string key = key_match[1].str();

            INFO("i18n key: " << key);
            REQUIRE(i18n::get(key, "__missing_translation__") != "__missing_translation__");

            ++nr_keyed_entries;
            break;
        }
    }

    REQUIRE(nr_keyed_entries > 0);
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

    REQUIRE(i18n::localized_file(unique_rel_path, base_path) == base_path);

    std::filesystem::remove(base_path);
}
