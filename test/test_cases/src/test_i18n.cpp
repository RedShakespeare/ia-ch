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
    REQUIRE(i18n::get("insanity.draws_nearer_prefix", "Insanity draws nearer... ") == "疯狂逼近……");
    REQUIRE(
        i18n::get(
            "insanity.reduce_xp_start",
            "Thanks to the mercy of the mind, some past experiences are "
            "forgotten (-25% XP).") == "多亏心智的仁慈，一些过去的经历被遗忘了（-25% 经验）。");
    REQUIRE(i18n::get("insanity.scream_shriek", "I let out a terrified shriek.") == "我发出一声惊恐的尖叫。");
    REQUIRE(i18n::get("insanity.scream_terror", "I scream in terror.") == "我恐惧地尖叫。");
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
    REQUIRE(
        i18n::get(
            "item_explosive.throw_lit_dynamite",
            "I throw a lit dynamite stick.") == "我投掷了一根点燃的炸药。");
    REQUIRE(
        i18n::get(
            "item_explosive.throw_lit_molotov",
            "I throw a lit Molotov Cocktail.") == "我投掷了一瓶点燃的燃烧瓶。");
    REQUIRE(
        i18n::get(
            "item_explosive.throw_lit_flare",
            "I throw a lit flare.") == "我投掷了一支点燃的照明棒。");
    REQUIRE(
        i18n::get(
            "item_explosive.throw_smoke_grenade",
            "I throw a smoke grenade.") == "我投掷了一枚烟雾手雷。");
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
    REQUIRE(i18n::get("map.item_legend", "Item") == "物品");
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
    REQUIRE(i18n::get("attack_melee.hear_fighting", "I hear fighting.") == "我听到打斗声。");
    REQUIRE(i18n::get("attack_melee.attack_terrain_prefix", "Attacking ") == "攻击");
    REQUIRE(i18n::get("attack_melee.attack_terrain_with", " with ") == "，使用");
    REQUIRE(
        i18n::get("attack_melee.attack_terrain_suffix", " would be useless.") ==
        "会毫无用处。");
    REQUIRE(
        i18n::get("attack_melee.stopped_at_boundary_suffix", " is stopped at the boundary.") ==
        "被挡在边界处。");
    REQUIRE(i18n::get("bash.attack_middle", " ") == "");
    REQUIRE(i18n::get("terrain_pylon.space", " ") == "");
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
