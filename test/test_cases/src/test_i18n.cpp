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
    REQUIRE(
        i18n::get(
            "player_bon.background.exorcist.destroy_sacred_tools_descr",
            "Cannot use manuscripts, altars, monoliths, or gongs, "
            "but instead gains experience and fervor for destroying "
            "these (manuscripts are destroyed when picking them up). "
            "Fervor can be used for casting spells - these points "
            "are used automatically when there is not enough "
            "spirit points to cast from.") ==
        "不能使用手稿、祭坛、独石或锣，而是通过摧毁它们获得经验和热忱（手稿会在拾取时被摧毁）。热忱可用于施放法术；当精神点不足以施放时，会自动消耗这些点数。");
    REQUIRE(
        i18n::get(
            "player_bon.background.exorcist.holy_symbol_descr",
            "Starts with a Holy Symbol, which can restore "
            "spirit points and grant resistance against "
            "mental shock and fear.") ==
        "开局携带一个圣符，可恢复精神点，并赋予对精神震惊和恐惧的抗性。");
    REQUIRE(
        i18n::get(
            "player_bon.background.exorcist.bonus_trait_levels_prefix",
            "Gains a bonus trait at character levels ") == "在角色等级");
    REQUIRE(i18n::get("player_bon.background.exorcist.bonus_trait_levels_separator", ", ") == "、");
    REQUIRE(i18n::get("player_bon.background.exorcist.bonus_trait_levels_and_separator", ", and ") == "和");
    REQUIRE(i18n::get("player_bon.background.exorcist.bonus_trait_levels_suffix", ".") == "时获得一个额外特质。");
    REQUIRE(
        i18n::get(
            "player_bon.background.flagellant.no_damage_shock_descr",
            "No mental shock received for taking damage.") == "因受伤而受到的精神震惊为零。");
    REQUIRE(
        i18n::get(
            "player_bon.background.flagellant.moribund_descr",
            "If health is reduced to 6 hit points or below when taking damage, "
            "the moribund status is applied for 5-7 turns "
            "(+3 melee damage, +30% melee hit chance, +3 armor points).") ==
        "因受到伤害而生命值降至 6 点或以下时，会获得濒死状态 5-7 回合（+3 近战伤害、+30% 近战命中率、+3 护甲点）。");
    REQUIRE(
        i18n::get(
            "player_bon.background.flagellant.torture_collar_descr",
            "Wears a torture collar which cannot be taken off; "
            "walking requires extra turns, and stealth and evasion "
            "are reduced by 20%. However, wearing the collar hardens "
            "the Flagellant against physical suffering, armor is "
            "increased by 3 points.") ==
        "佩戴无法取下的苦刑项圈；行走需要额外回合，潜行和闪避降低 20%。然而，佩戴项圈会让鞭笞者对肉体痛苦更加麻木，护甲增加 3 点。");
    REQUIRE(
        i18n::get(
            "player_bon.background.flagellant.blood_upgrade_levels_prefix",
            "Specializes in spells belonging to the Blood domain. "
            "At character levels ") == "专精于鲜血领域法术。在角色等级");
    REQUIRE(i18n::get("player_bon.background.flagellant.blood_upgrade_levels_and_separator", " and ") == "和");
    REQUIRE(
        i18n::get(
            "player_bon.background.flagellant.blood_upgrade_levels_suffix",
            ", all spells belonging to this domain are cast at "
            "a higher skill level.") == "时，该领域的所有法术都以更高技能等级施放。");
    REQUIRE(
        i18n::get(
            "player_bon.background.flagellant.blood_spell_shock_descr",
            "-25% mental shock taken from casting memorized spells "
            "from the Blood domain.") == "施放已记忆的鲜血领域法术时，受到的精神震惊 -25%。");
    REQUIRE(
        i18n::get(
            "player_bon.background.ghoul.darkness_shock_descr",
            "-50% mental shock taken from seeing monsters and "
            "standing in darkness - "
            "but also only gains halved shock reduction from light.") ==
        "因看见怪物和站在黑暗中受到的精神震惊 -50%，但从光照获得的震惊降低效果也减半。");
    REQUIRE(
        i18n::get(
            "player_bon.background.ghoul.corpse_feeding_descr",
            "Does not regenerate hit points and cannot use medical equipment - "
            "instead heals by feeding on corpses "
            "(feeding is done by waiting on a corpse).") ==
        "不会再生生命值，也不能使用医疗器械，而是通过吞食尸体来治疗（在尸体上等待即可吞食）。");
    REQUIRE(
        i18n::get(
            "player_bon.background.ghoul.frenzy_descr",
            "Can incite frenzy at will, and does not become weakened "
            "when frenzy ends.") == "可以随意激发狂暴，并且狂暴结束时不会变得虚弱。");
    REQUIRE(i18n::get("player_bon.background.ghoul.hit_points_descr", "+8 hit points.") == "+8 点生命值。");
    REQUIRE(
        i18n::get("player_bon.background.ghoul.disease_immunity_descr", "Is immune to disease and infections.") ==
        "免疫疾病和感染。");
    REQUIRE(i18n::get("player_bon.background.ghoul.sprain_immunity_descr", "Does not get sprains.") == "不会扭伤。");
    REQUIRE(i18n::get("player_bon.background.ghoul.darkvision_descr", "Can see in darkness.") == "可以在黑暗中视物。");
    REQUIRE(
        i18n::get("player_bon.background.ghoul.ranged_penalty_descr", "-15% hit chance with firearms and thrown weapons.") ==
        "使用火器和投掷武器时命中率 -15%。");
    REQUIRE(i18n::get("player_bon.background.ghoul.ghoul_allies_descr", "All ghouls are allied.") == "所有食尸鬼都是盟友。");
    REQUIRE(
        i18n::get(
            "player_bon.background.occultist.strange_item_shock_descr",
            "-50% mental shock taken from casting memorized spells "
            "and from using or identifying strange items such as "
            "potions or manuscripts "
            "(in addition to \"Cool-headed\").") ==
        "施放已记忆的法术，以及使用或鉴定药水、手稿等奇异物品时，受到的精神震惊 -50%（在“冷静”之外）。");
    REQUIRE(
        i18n::get(
            "player_bon.background.occultist.spell_domain_traits_descr",
            "Can gain traits to increase skill level in various spell domains.") ==
        "可以获得特质来提高各个法术领域的技能等级。");
    REQUIRE(
        i18n::get(
            "player_bon.background.occultist.choose_domain_descr",
            "Chooses background in a specific spell domain at character creation, "
            "which determines starting spells.") == "在角色创建时选择一个特定法术领域作为背景，这会决定起始法术。");
    REQUIRE(
        i18n::get(
            "player_bon.background.occultist.spirit_points_descr",
            "+3 spirit points (in addition to \"Stout Spirit\").") == "+3 点精神（在“坚定精神”之外）。");
    REQUIRE(
        i18n::get(
            "player_bon.background.occultist.bone_charms_descr",
            "Starts with several Bone Charms, that can be used for "
            "gaining spell resistance or dispelling sigils "
            "(\"strange shape\" on the floor).") == "开局携带数个骨符，可用于获得法术抗性或驱散符印（地面上的“奇异形状”）。");
    REQUIRE(
        i18n::get(
            "player_bon.background.occultist.domain.channeling_prefix",
            "You have previously dabbled in the channeling of violent energy, ") ==
        "你曾涉猎暴烈能量的导引，");
    REQUIRE(
        i18n::get(
            "player_bon.background.occultist.domain.corruption_prefix",
            "You have previously dabbled in spells that wither and corrupt, ") ==
        "你曾涉猎枯萎与腐化的法术，");
    REQUIRE(
        i18n::get(
            "player_bon.background.occultist.domain.illusion_prefix",
            "You have previously dabbled in the casting of illusions, ") ==
        "你曾涉猎幻象的施放，");
    REQUIRE(
        i18n::get(
            "player_bon.background.occultist.domain.mind_prefix",
            "You have previously dabbled in disciplines of revelation, foresight, "
            "and will, ") == "你曾涉猎启示、预见与意志的学科，");
    REQUIRE(
        i18n::get(
            "player_bon.background.occultist.domain.time_prefix",
            "You have previously dabbled in the manipulation of time and causality, ") ==
        "你曾涉猎时间与因果的操纵，");
    REQUIRE(
        i18n::get(
            "player_bon.background.occultist.domain.warding_prefix",
            "You have previously dabbled in protective magic, ") == "你曾涉猎防护魔法，");
    REQUIRE(i18n::get("player_bon.background.occultist.domain.knowledge_prefix", "and have basic knowledge of ") == "并对");
    REQUIRE(i18n::get("player_bon.background.occultist.domain.knowledge_suffix", ".") == "有基础了解。");
    REQUIRE(
        i18n::get(
            "player_bon.background.rogue.passive_shock_descr",
            "Mental shock received passively over time is reduced by 25%.") == "随时间被动获得的精神震惊减少 25%。");
    REQUIRE(
        i18n::get(
            "player_bon.background.rogue.spot_hidden_descr",
            "+10% chance to spot hidden monsters, doors, and traps.") == "发现隐藏怪物、门和陷阱的几率 +10%。");
    REQUIRE(
        i18n::get(
            "player_bon.background.rogue.creature_awareness_descr",
            "Remains aware of the presence of other creatures longer.") == "对其他生物存在的感知保持更久。");
    REQUIRE(
        i18n::get(
            "player_bon.background.rogue.sense_uniques_descr",
            "Can sense the presence of unique monsters or powerful "
            "artifacts.") == "可以感知独特怪物或强大神器的存在。");
    REQUIRE(
        i18n::get(
            "player_bon.background.rogue.mind_cloud_artifact_descr",
            "Has acquired an artifact which can cloud the minds of all "
            "enemies, causing them to forget the presence of the "
            "user.") == "已获得一件神器，可以蒙蔽所有敌人的心智，使它们忘记使用者的存在。");
    REQUIRE(
        i18n::get("player_bon.background.war_vet.instant_prepare_descr", "Switches to prepared weapon instantly.") ==
        "立即切换到预备武器。");
    REQUIRE(i18n::get("player_bon.background.war_vet.flak_jacket_descr", "Starts with a Flak Jacket.") == "开局携带一件防弹背心。");
    REQUIRE(
        i18n::get(
            "player_bon.background.war_vet.armor_maintenance_descr",
            "Maintains armor twice as long before it breaks.") == "护甲在损坏前可维持两倍时间。");
    REQUIRE(i18n::get("player_bon.trait.adept_melee.title", "Adept Melee Fighter") == "熟练近战斗士");
    REQUIRE(
        i18n::get(
            "player_bon.trait.adept_melee.descr",
            "+10% hit chance and +1 damage with melee attacks") ==
        "+10% 命中率，近战攻击 +1 伤害");
    REQUIRE(i18n::get("player_bon.trait.expert_melee.title", "Expert Melee Fighter") == "专家近战斗士");
    REQUIRE(i18n::get("player_bon.trait.master_melee.title", "Master Melee Fighter") == "大师近战斗士");
    REQUIRE(i18n::get("player_bon.trait.adept_marksman.title", "Adept Marksman") == "熟练射手");
    REQUIRE(
        i18n::get(
            "player_bon.trait.adept_marksman.descr",
            "+10% hit chance and +1 minimum damage with firearms and thrown weapons "
            "(cannot raise maximum damage)") == "使用火器和投掷武器时 +10% 命中率，并且最低伤害 +1（不能提高最大伤害）");
    REQUIRE(i18n::get("player_bon.trait.expert_marksman.title", "Expert Marksman") == "专家射手");
    REQUIRE(i18n::get("player_bon.trait.master_marksman.title", "Master Marksman") == "大师射手");
    REQUIRE(i18n::get("player_bon.trait.cool_headed.title", "Cool-headed") == "冷静");
    REQUIRE(
        i18n::get(
            "player_bon.trait.cool_headed.descr",
            "+20% mental shock resistance") == "+20% 精神震惊抗性");
    REQUIRE(i18n::get("player_bon.trait.courageous.title", "Courageous") == "勇敢");
    REQUIRE(i18n::get("player_bon.trait.dexterous.title", "Dexterous") == "灵巧");
    REQUIRE(
        i18n::get(
            "player_bon.trait.dexterous.descr",
            "+25% chance to evade attacks") == "+25% 闪避攻击几率");
    REQUIRE(i18n::get("player_bon.trait.lithe.title", "Lithe") == "轻盈");
    REQUIRE(i18n::get("player_bon.trait.crippling_strikes.title", "Crippling Strikes") == "致残打击");
    REQUIRE(
        i18n::get(
            "player_bon.trait.crippling_strikes.descr",
            "Your melee attacks have 60% chance to weaken the target "
            "creature for 2-3 turns (reducing their melee damage by half)") ==
        "你的近战攻击有 60% 几率使目标生物虚弱 2-3 回合（使其近战伤害减半）");
    REQUIRE(i18n::get("player_bon.trait.fearless.title", "Fearless") == "无畏");
    REQUIRE(
        i18n::get(
            "player_bon.trait.fearless.descr",
            "You cannot become terrified, +10% mental shock resistance") ==
        "你不会变得恐惧，+10% 精神震惊抗性");
    REQUIRE(i18n::get("player_bon.trait.stealthy.title", "Stealthy") == "潜行");
    REQUIRE(
        i18n::get(
            "player_bon.trait.stealthy.descr",
            "+45% chance to avoid detection by sight") == "+45% 避免被视觉发现的几率");
    REQUIRE(i18n::get("player_bon.trait.imperceptible.title", "Imperceptible") == "难以察觉");
    REQUIRE(i18n::get("player_bon.trait.silent.title", "Silent") == "静默");
    REQUIRE(
        i18n::get(
            "player_bon.trait.silent.descr",
            "All your melee attacks are silent (regardless of the weapon), "
            "and creatures are not alerted when you open or close doors, "
            "or wade through water") == "你的所有近战攻击都是无声的（无论使用什么武器），打开或关闭门、涉水而过时也不会惊动生物");
    REQUIRE(i18n::get("player_bon.trait.vigilant.title", "Vigilant") == "警觉");
    REQUIRE(
        i18n::get(
            "player_bon.trait.vigilant.descr",
            "You are always aware of nearby creatures") == "你总能察觉附近的生物");
    REQUIRE(i18n::get("player_bon.trait.treasure_hunter.title", "Treasure Hunter") == "寻宝者");
    REQUIRE(
        i18n::get(
            "player_bon.trait.treasure_hunter.descr",
            "You tend to find more items") == "你往往会找到更多物品");
    REQUIRE(i18n::get("player_bon.trait.self_aware.title", "Self-aware") == "自知");
    REQUIRE(
        i18n::get(
            "player_bon.trait.self_aware.descr",
            "You cannot become confused, the number of remaining turns "
            "for status effects are displayed") == "你不会变得混乱，并会显示状态效果的剩余回合数");
    REQUIRE(i18n::get("player_bon.trait.healer.title", "Healer") == "治疗者");
    REQUIRE(
        i18n::get(
            "player_bon.trait.healer.descr",
            "Using medical equipment requires only half the normal time "
            "and resources") == "使用医疗器械只需要一半的正常时间和资源");
    REQUIRE(i18n::get("player_bon.trait.rapid_recoverer.title", "Rapid Recoverer") == "快速恢复者");
    REQUIRE(
        i18n::get(
            "player_bon.trait.rapid_recoverer.descr",
            "You regenerate 1 hit point every third turn") == "你每三个回合恢复1点生命值");
    REQUIRE(i18n::get("player_bon.trait.survivalist.title", "Survivalist") == "生存专家");
    REQUIRE(
        i18n::get(
            "player_bon.trait.survivalist.descr",
            "You cannot become diseased, "
            "only half your wounds count, "
            "rounded down "
            "(i.e. number of wounds are halved when calculating "
            "combat, hit point and regeneration penalties, "
            "slower walking speed happens at 6 wounds instead of 3, "
            "and you die from 10 wounds instead of 5)") ==
        "你不会患病，并且只有一半伤口数计入惩罚（向下取整；也就是计算战斗、生命值和再生惩罚时伤口数减半，行走变慢从6处伤口而不是3处开始，死亡从10处伤口而不是5处开始）");
    REQUIRE(i18n::get("player_bon.trait.stout_spirit.title", "Stout Spirit") == "坚定精神");
    REQUIRE(
        i18n::get(
            "player_bon.trait.stout_spirit.descr",
            "+2 spirit points, increased spirit regeneration rate, you "
            "can defy harmful spells (it takes 125-150 turns to regain "
            "spell resistance after a spell is blocked)") == "+2点精神，精神再生速度提高，你可以抵抗有害法术（阻挡一个法术后需要125-150回合恢复法术抗性）");
    REQUIRE(i18n::get("player_bon.trait.strong_spirit.title", "Strong Spirit") == "强韧精神");
    REQUIRE(
        i18n::get(
            "player_bon.trait.strong_spirit.descr",
            "+2 spirit points, increased spirit regeneration rate, it "
            "takes 75-100 turns to regain spell resistance after a spell "
            "is blocked") == "+2点精神，精神再生速度提高，阻挡一个法术后需要75-100回合恢复法术抗性");
    REQUIRE(i18n::get("player_bon.trait.mighty_spirit.title", "Mighty Spirit") == "强大精神");
    REQUIRE(
        i18n::get(
            "player_bon.trait.mighty_spirit.descr",
            "+2 spirit points, increased spirit regeneration rate, it "
            "takes 25-50 turns to regain spell resistance after a spell "
            "is blocked") == "+2点精神，精神再生速度提高，阻挡一个法术后需要25-50回合恢复法术抗性");
    REQUIRE(i18n::get("player_bon.trait.meditative.title", "Meditative") == "冥想者");
    REQUIRE(
        i18n::get(
            "player_bon.trait.meditative.descr",
            "Applies a focused state which allows the next spell to be "
            "cast without spending a turn, and with the casting cost "
            "reduced by 1 point - it takes 125-150 turns to regain this "
            "state after a spell is cast") == "施加专注状态，使下一个法术无需花费回合即可施放，且施法消耗减少1点；施放一个法术后需要125-150回合恢复此状态");
    REQUIRE(i18n::get("player_bon.trait.sage.title", "Sage") == "贤者");
    REQUIRE(
        i18n::get(
            "player_bon.trait.sage.descr",
            "When focused, spells are cast without spending spirit points, "
            "and the duration to regain the focused state is reduced to "
            "75-100 turns") == "专注时，施放法术不消耗精神点，且恢复专注状态的时间缩短为75-100回合");
    REQUIRE(i18n::get("player_bon.trait.absorption.title", "Absorption") == "吸收");
    REQUIRE(
        i18n::get(
            "player_bon.trait.absorption.descr",
            "1-6 spirit points are restored each time Spell Shield is ended "
            "by a hostile spell "
            "(Spell Shield is granted by spirit traits or the Spell Shield spell)") == "每当法术护盾被敌对法术终止时，恢复1-6点精神（法术护盾由精神特质或法术护盾法术赋予）");
    REQUIRE(i18n::get("player_bon.trait.tough.title", "Tough") == "坚韧");
    REQUIRE(
        i18n::get(
            "player_bon.trait.tough.descr",
            "+6 hit points, "
            "+10% chance to resist burning, poisoning and paralysis, "
            "less likely to sprain when kicking, more likely to "
            "succeed with object interactions requiring strength (e.g. "
            "bashing things open)") ==
        "+6点生命值，+10%抵抗燃烧、中毒和麻痹的几率，踢击时更不容易扭伤，更容易成功完成需要力量的物体互动（例如撞开东西）");
    REQUIRE(i18n::get("player_bon.trait.rugged.title", "Rugged") == "强健");
    REQUIRE(i18n::get("player_bon.trait.unbreakable.title", "Unbreakable") == "不屈");
    REQUIRE(i18n::get("player_bon.trait.thick_skinned.title", "Thick Skinned") == "厚皮");
    REQUIRE(
        i18n::get(
            "player_bon.trait.thick_skinned.descr",
            "+1 armor point (physical damage reduced by 1 point)") == "+1点护甲（物理伤害减少1点）");
    REQUIRE(i18n::get("player_bon.trait.callous.title", "Callous") == "冷硬");
    REQUIRE(i18n::get("player_bon.trait.resistant.title", "Resistant") == "抗性");
    REQUIRE(
        i18n::get(
            "player_bon.trait.resistant.descr",
            "+25% chance to resist burning, poisoning and paralysis - "
            "and the duration of those effects is halved") == "+25%抵抗燃烧、中毒和麻痹的几率，并且这些效果的持续时间减半");
    REQUIRE(i18n::get("player_bon.trait.strong_backed.title", "Strong-backed") == "强壮背负");
    REQUIRE(i18n::get("player_bon.trait.strong_backed.descr", "+50% carry weight limit") == "+50%负重上限");
    REQUIRE(i18n::get("player_bon.trait.undead_bane.title", "Bane of the Undead") == "亡灵克星");
    REQUIRE(
        i18n::get(
            "player_bon.trait.undead_bane.descr",
            "+2 melee and ranged attack damage against all undead "
            "monsters, +50% hit chance against ethereal undead monsters") ==
        "对所有亡灵怪物的近战和远程攻击伤害+2，对以太亡灵怪物的命中率+50%");
    REQUIRE(i18n::get("player_bon.trait.elec_incl.title", "Electrically Inclined") == "电气亲和");
    REQUIRE(
        i18n::get(
            "player_bon.trait.elec_incl.descr",
            "Rods recharge twice as fast, strange devices are less likely "
            "to malfunction or break, electric lanterns last twice as "
            "long, +1 damage with electricity weapons") == "魔杖充能速度加倍，奇异装置更不容易故障或损坏，电提灯持续时间加倍，电击武器伤害+1");
    REQUIRE(i18n::get("player_bon.trait.adept_of_channeling.title", "Adept of Channeling") == "导能熟手");
    REQUIRE(
        i18n::get(
            "player_bon.trait.adept_of_channeling.descr",
            "Specialize in the channeling of violent energy. "
            "Channeling spells are cast at a higher skill level.") == "专精于暴烈能量的导引。导能法术以更高技能等级施放。");
    REQUIRE(i18n::get("player_bon.trait.master_of_channeling.title", "Master of Channeling") == "导能大师");
    REQUIRE(
        i18n::get(
            "player_bon.trait.master_of_channeling.descr",
            "Attain mastery over the channeling of violent energy. "
            "Channeling spells are cast at a higher skill level.") == "掌握暴烈能量的导引。导能法术以更高技能等级施放。");
    REQUIRE(i18n::get("player_bon.trait.adept_of_corruption.title", "Adept of Corruption") == "腐化熟手");
    REQUIRE(
        i18n::get(
            "player_bon.trait.adept_of_corruption.descr",
            "Specialize in corruption and withering. "
            "Corruption spells are cast at a higher skill level.") == "专精于腐化和枯萎。腐化法术以更高技能等级施放。");
    REQUIRE(i18n::get("player_bon.trait.master_of_corruption.title", "Master of Corruption") == "腐化大师");
    REQUIRE(
        i18n::get(
            "player_bon.trait.master_of_corruption.descr",
            "Attain mastery over corruption and withering. "
            "Corruption spells are cast at a higher skill level.") == "掌握腐化和枯萎。腐化法术以更高技能等级施放。");
    REQUIRE(i18n::get("player_bon.trait.adept_of_illusion.title", "Adept of Illusion") == "幻象熟手");
    REQUIRE(
        i18n::get(
            "player_bon.trait.adept_of_illusion.descr",
            "Specialize in the casting of illusions. "
            "Illusion spells are cast at a higher skill level.") == "专精于施放幻象。幻象法术以更高技能等级施放。");
    REQUIRE(i18n::get("player_bon.trait.master_of_illusion.title", "Master of Illusion") == "幻象大师");
    REQUIRE(
        i18n::get(
            "player_bon.trait.master_of_illusion.descr",
            "Attain mastery over the casting of illusions. "
            "Illusion spells are cast at a higher skill level.") == "掌握幻象的施放。幻象法术以更高技能等级施放。");
    REQUIRE(i18n::get("player_bon.trait.adept_of_the_mind.title", "Adept of the Mind") == "心灵熟手");
    REQUIRE(
        i18n::get(
            "player_bon.trait.adept_of_the_mind.descr",
            "Specialize in knowledge, foresight, and will. "
            "Mind spells are cast at a higher skill level.") == "专精于知识、预见和意志。心灵法术以更高技能等级施放。");
    REQUIRE(i18n::get("player_bon.trait.master_of_the_mind.title", "Master of the Mind") == "心灵大师");
    REQUIRE(
        i18n::get(
            "player_bon.trait.master_of_the_mind.descr",
            "Attain mastery over knowledge, foresight, and will. "
            "Mind spells are cast at a higher skill level, "
            "and you also sense items and creatures.") == "掌握知识、预见和意志。心灵法术以更高技能等级施放，并且你还能感知物品和生物。");
    REQUIRE(i18n::get("player_bon.trait.adept_of_time.title", "Adept of Time") == "时间熟手");
    REQUIRE(
        i18n::get(
            "player_bon.trait.adept_of_time.descr",
            "Specialize in the manipulation of time and causality. "
            "Time spells are cast at a higher skill level.") == "专精于时间和因果的操纵。时间法术以更高技能等级施放。");
    REQUIRE(i18n::get("player_bon.trait.master_of_time.title", "Master of Time") == "时间大师");
    REQUIRE(
        i18n::get(
            "player_bon.trait.master_of_time.descr",
            "Attain mastery over the manipulation of time and causality. "
            "Time spells are cast at a higher skill level.") == "掌握时间和因果的操纵。时间法术以更高技能等级施放。");
    REQUIRE(i18n::get("player_bon.trait.adept_of_warding.title", "Adept of Warding") == "守护熟手");
    REQUIRE(
        i18n::get(
            "player_bon.trait.adept_of_warding.descr",
            "Specialize in protective magic. "
            "Warding spells are cast at a higher skill level.") == "专精于防护魔法。守护法术以更高技能等级施放。");
    REQUIRE(i18n::get("player_bon.trait.master_of_warding.title", "Master of Warding") == "守护大师");
    REQUIRE(
        i18n::get(
            "player_bon.trait.master_of_warding.descr",
            "Attain mastery over protective magic. "
            "Warding spells are cast at a higher skill level.") == "掌握防护魔法。守护法术以更高技能等级施放。");
    REQUIRE(i18n::get("player_bon.trait.gain_cast_prefix", "Gain the ability to cast \"") == "获得施放“");
    REQUIRE(i18n::get("player_bon.trait.gain_cast_suffix", "\"") == "”的能力");
    REQUIRE(i18n::get("player_bon.trait.gain_cast_skill_prefix", " at ") == "，以");
    REQUIRE(i18n::get("player_bon.trait.gain_cast_skill_suffix", " level") == "等级");
    REQUIRE(i18n::get("player_bon.trait.gain_cast_descr_separator", " -") == "；");
    REQUIRE(i18n::get("player_bon.trait.gain_cast_line_separator", " ") == "");
    REQUIRE(i18n::get("player_bon.trait.gain_cast_cost_prefix", " This spell costs ") == "此法术消耗");
    REQUIRE(i18n::get("player_bon.trait.gain_cast_cost_suffix", " spirit to cast.") == "点精神施放。");
    REQUIRE(i18n::get("player_bon.trait.available_sp_prefix", "You currently have ") == "你当前有");
    REQUIRE(i18n::get("player_bon.trait.available_sp_suffix", " spirit") == "点精神");
    REQUIRE(i18n::get("player_bon.trait.available_fp_prefix", " and ") == "和");
    REQUIRE(i18n::get("player_bon.trait.available_fp_suffix", " fervor") == "点热忱");
    REQUIRE(i18n::get("player_bon.trait.available_sp_period", ".") == "。");
    REQUIRE(i18n::get("player_bon.trait.cast_bless_i.title", "Cast Bless") == "施放祝福");
    REQUIRE(i18n::get("player_bon.trait.cast_bless_ii.title", "Cast Bless II") == "施放祝福 II");
    REQUIRE(i18n::get("player_bon.trait.cast_cleansing_fire_i.title", "Cast Cleansing Fire") == "施放净化之火");
    REQUIRE(i18n::get("player_bon.trait.cast_cleansing_fire_ii.title", "Cast Cleansing Fire II") == "施放净化之火 II");
    REQUIRE(i18n::get("player_bon.trait.cast_heal_i.title", "Cast Heal") == "施放治疗");
    REQUIRE(i18n::get("player_bon.trait.cast_heal_ii.title", "Cast Heal II") == "施放治疗 II");
    REQUIRE(i18n::get("player_bon.trait.cast_light_i.title", "Cast Light") == "施放光亮");
    REQUIRE(i18n::get("player_bon.trait.cast_light_ii.title", "Cast Light II") == "施放光亮 II");
    REQUIRE(i18n::get("player_bon.trait.cast_sanctuary_i.title", "Cast Sanctuary") == "施放庇护");
    REQUIRE(i18n::get("player_bon.trait.cast_sanctuary_ii.title", "Cast Sanctuary II") == "施放庇护 II");
    REQUIRE(i18n::get("player_bon.trait.cast_see_invisible_i.title", "Cast See Invisible") == "施放识破隐形");
    REQUIRE(i18n::get("player_bon.trait.cast_see_invisible_ii.title", "Cast See Invisible II") == "施放识破隐形 II");
    REQUIRE(i18n::get("player_bon.trait.prolonged_life.title", "Prolonged Life") == "延续生命");
    REQUIRE(
        i18n::get(
            "player_bon.trait.prolonged_life.descr",
            "Any fatal damage received is instead drained from your fervor points") == "受到任何致命伤害时改为消耗你的热忱点数");
    REQUIRE(i18n::get("player_bon.trait.ravenous.title", "Ravenous") == "贪食");
    REQUIRE(
        i18n::get(
            "player_bon.trait.ravenous.descr",
            "You occasionally feed on living victims when attacking with claws") == "用爪攻击时，你偶尔会吞食活体受害者");
    REQUIRE(i18n::get("player_bon.trait.foul.title", "Foul") == "污秽");
    REQUIRE(
        i18n::get(
            "player_bon.trait.foul.descr",
            "+1 claw damage, when attacking with claws, vicious worms "
            "occasionally burst out from the corpses of your victims to "
            "attack your enemies") == "+1爪击伤害，用爪攻击时，恶毒蠕虫偶尔会从受害者尸体中爆出并攻击你的敌人");
    REQUIRE(i18n::get("player_bon.trait.toxic.title", "Toxic") == "剧毒");
    REQUIRE(
        i18n::get(
            "player_bon.trait.toxic.descr",
            "+1 claw damage, you are immune to poison, and attacks with "
            "your claws often poisons your victims") == "+1爪击伤害，你免疫中毒，并且用爪攻击经常会使受害者中毒");
    REQUIRE(i18n::get("player_bon.trait.indomitable_fury.title", "Indomitable Fury") == "不屈狂怒");
    REQUIRE(
        i18n::get(
            "player_bon.trait.indomitable_fury.descr",
            "While frenzied, you are immune to wounds, and your claw attacks cause fear") == "狂暴时，你免疫伤口，爪击会造成恐惧");
    REQUIRE(i18n::get("player_bon.trait.elusive.title", "Elusive") == "难以捉摸");
    REQUIRE(
        i18n::get(
            "player_bon.trait.elusive.descr",
            "Creatures only remember you for half the normal duration (rounded up).") == "生物记住你的时间只有正常持续时间的一半（向上取整）。");
    REQUIRE(i18n::get("player_bon.trait.vicious.title", "Vicious") == "凶狠");
    REQUIRE(
        i18n::get(
            "player_bon.trait.vicious.descr",
            "+100% backstab damage (in addition to the normal +50%)") == "+100%背刺伤害（在普通+50%之外）");
    REQUIRE(i18n::get("player_bon.trait.ruthless.title", "Ruthless") == "无情");
    REQUIRE(i18n::get("player_bon.trait.ruthless.descr", "+100% backstab damage") == "+100%背刺伤害");
    REQUIRE(i18n::get("player_bon.trait.steady_aimer.title", "Steady Aimer") == "稳定瞄准");
    REQUIRE(
        i18n::get(
            "player_bon.trait.steady_aimer.descr",
            "Standing still gives ranged attacks maximum damage and +10% "
            "hit chance on the following turn, unless damage is taken") == "站立不动会让下一回合的远程攻击造成最大伤害并+10%命中率，除非受到伤害");
    REQUIRE(i18n::get("player_bon.trait.galvanization.title", "Galvanization") == "激发");
    REQUIRE(
        i18n::get(
            "player_bon.trait.galvanization.descr",
            "Casting any spell from the Blood domain grants "
            "Regeneration for 4-6 turns "
            "(+1 extra hit point regenerated per turn), if "
            "hit points are lost from casting the spell") == "施放任何鲜血领域法术会赋予再生4-6回合（每回合额外再生+1生命值），前提是施法损失了生命值");
    REQUIRE(i18n::get("player_bon.trait.enthusiasm.title", "Enthusiasm") == "狂热");
    REQUIRE(i18n::get("player_bon.trait.enthusiasm.descr", "Doubles all bonuses for the moribund effect") == "使濒死效果的所有加成翻倍");
    REQUIRE(i18n::get("player_bon.trait.memento_mori.title", "Memento Mori") == "记住死亡");
    REQUIRE(
        i18n::get(
            "player_bon.trait.memento_mori.descr",
            "Raises the threshold of the moribund status to 8 hit points, "
            "and increases the duration of the effect by 50% (rounded down)") == "将濒死状态阈值提高到8点生命值，并将效果持续时间增加50%（向下取整）");
    REQUIRE(
        i18n::get(
            "property.mon_struggles_tear_free_suffix",
            " struggles to tear free.") == "挣扎着想挣脱。");
    REQUIRE(
        i18n::get(
            "property.infection_getting_worse",
            "My infection is getting worse!") == "我的感染正在恶化！");
    REQUIRE(
        i18n::get(
            "property.mon_struggles_pull_free_suffix",
            " struggles to pull free.") == "挣扎着想拔脱。");
    REQUIRE(
        i18n::get(
            "property.struggle_tear_out_spike",
            "I struggle to tear out the spike!") == "我挣扎着想拔出尖刺！");
    REQUIRE(
        i18n::get(
            "property.mon_struggles_in_pain_suffix",
            " struggles in pain!") == "痛苦地挣扎！");
    REQUIRE(i18n::get("property.one_wound_healed", "A wound is healed.") == "一道伤口愈合了。");
    REQUIRE(i18n::get("property.all_wounds_healed", "All my wounds are healed!") == "我所有的伤口都愈合了！");
    REQUIRE(i18n::get("property.crave_astral_opium", "I crave Astral Opium!!") == "我渴望星界鸦片！！");
    REQUIRE(i18n::get("property.resist_electric_player", "I feel a faint tingle.") == "我感到一阵微弱的刺痛。");
    REQUIRE(i18n::get("property.resist_seems_unaffected", "{} seems unaffected.") == "{}似乎不受影响。");
    REQUIRE(i18n::get("property.resist_physical_player", "I resist harm.") == "我抵抗了伤害。");
    REQUIRE(i18n::get("property.resist_seems_unharmed", "{} seems unharmed.") == "{}似乎没有受伤。");
    REQUIRE(i18n::get("property.resist_fire_player", "I feel warm.") == "我感到温暖。");
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
    REQUIRE(i18n::get("item_device.condition_prefix", "It seems ") == "它似乎");
    REQUIRE(i18n::get("item_device.condition_fine", "to be in fine condition.") == "状况良好。");
    REQUIRE(i18n::get("item_device.condition_shoddy", "to be in shoddy condition.") == "状况粗劣。");
    REQUIRE(i18n::get("item_device.condition_breaking", "almost broken.") == "几乎坏掉了。");
    REQUIRE(i18n::get("item_device.name_info_breaking", "(breaking)") == "（将坏）");
    REQUIRE(i18n::get("item_device.name_info_shoddy", "(shoddy)") == "（粗劣）");
    REQUIRE(i18n::get("item_device.name_info_fine", "(fine)") == "（良好）");
    REQUIRE(
        i18n::get(
            "item_device.descr_blaster",
            "When activated, this device blasts one visible hostile "
            "creature with infernal power.") == "启动后，这个装置会用地狱之力轰击一个可见的敌对生物。");
    REQUIRE(
        i18n::get(
            "item_device.descr_rejuvenator",
            "When activated, this device heals all wounds and physical "
            "maladies. The procedure is very painful and invasive "
            "however, and causes great shock to the user.") ==
        "启动后，这个装置会治愈所有伤口和身体疾病。然而，这个过程非常痛苦且侵入性很强，并会给使用者带来巨大的精神震惊。");
    REQUIRE(i18n::get("item_device.teleported_suffix", " is teleported.") == "被传送了。");
    REQUIRE(
        i18n::get(
            "item_device.descr_translocator",
            "When activated, this device teleports all visible enemies "
            "to different locations.") == "启动后，这个装置会将所有可见敌人传送到不同位置。");
    REQUIRE(
        i18n::get(
            "item_device.descr_sentry_drone",
            "When activated, this device will \"come alive\" and "
            "guard the user.") == "启动后，这个装置会“活过来”并守卫使用者。");
    REQUIRE(
        i18n::get(
            "item_device.descr_force_field",
            "When activated, this device constructs a temporary opaque "
            "barrier around the user, blocking all physical matter. "
            "The barrier can only be created in empty spaces "
            "(i.e. not in spaces occupied by creatures, walls, etc).") ==
        "启动后，这个装置会在使用者周围构建一道临时的不透明屏障，阻挡所有物质。屏障只能在空地中生成（即没有生物、墙壁等占据的空间）。");
    REQUIRE(i18n::get("item_rod.look_iron", "Iron") == "铁");
    REQUIRE(i18n::get("item_rod.look_iron_a", "an Iron") == "铁");
    REQUIRE(i18n::get("item_rod.look_zinc", "Zinc") == "锌");
    REQUIRE(i18n::get("item_rod.look_zinc_a", "a Zinc") == "锌");
    REQUIRE(i18n::get("item_rod.look_chromium", "Chromium") == "铬");
    REQUIRE(i18n::get("item_rod.look_chromium_a", "a Chromium") == "铬");
    REQUIRE(i18n::get("item_rod.look_tin", "Tin") == "锡");
    REQUIRE(i18n::get("item_rod.look_tin_a", "a Tin") == "锡");
    REQUIRE(i18n::get("item_rod.look_silver", "Silver") == "银");
    REQUIRE(i18n::get("item_rod.look_silver_a", "a Silver") == "银");
    REQUIRE(i18n::get("item_rod.look_golden", "Golden") == "金");
    REQUIRE(i18n::get("item_rod.look_golden_a", "a Golden") == "金");
    REQUIRE(i18n::get("item_rod.look_nickel", "Nickel") == "镍");
    REQUIRE(i18n::get("item_rod.look_nickel_a", "a Nickel") == "镍");
    REQUIRE(i18n::get("item_rod.look_copper", "Copper") == "铜");
    REQUIRE(i18n::get("item_rod.look_copper_a", "a Copper") == "铜");
    REQUIRE(i18n::get("item_rod.look_lead", "Lead") == "铅");
    REQUIRE(i18n::get("item_rod.look_lead_a", "a Lead") == "铅");
    REQUIRE(i18n::get("item_rod.look_tungsten", "Tungsten") == "钨");
    REQUIRE(i18n::get("item_rod.look_tungsten_a", "a Tungsten") == "钨");
    REQUIRE(i18n::get("item_rod.look_platinum", "Platinum") == "铂");
    REQUIRE(i18n::get("item_rod.look_platinum_a", "a Platinum") == "铂");
    REQUIRE(i18n::get("item_rod.look_lithium", "Lithium") == "锂");
    REQUIRE(i18n::get("item_rod.look_lithium_a", "a Lithium") == "锂");
    REQUIRE(i18n::get("item_rod.look_zirconium", "Zirconium") == "锆");
    REQUIRE(i18n::get("item_rod.look_zirconium_a", "a Zirconium") == "锆");
    REQUIRE(i18n::get("item_rod.look_gallium", "Gallium") == "镓");
    REQUIRE(i18n::get("item_rod.look_gallium_a", "a Gallium") == "镓");
    REQUIRE(i18n::get("item_rod.look_cobalt", "Cobalt") == "钴");
    REQUIRE(i18n::get("item_rod.look_cobalt_a", "a Cobalt") == "钴");
    REQUIRE(i18n::get("item_rod.look_titanium", "Titanium") == "钛");
    REQUIRE(i18n::get("item_rod.look_titanium_a", "a Titanium") == "钛");
    REQUIRE(i18n::get("item_rod.look_magnesium", "Magnesium") == "镁");
    REQUIRE(i18n::get("item_rod.look_magnesium_a", "a Magnesium") == "镁");
    REQUIRE(i18n::get("item_rod.rod_suffix", " Rod") == "魔杖");
    REQUIRE(i18n::get("item_rod.rods_suffix", " Rods") == "魔杖");
    REQUIRE(i18n::get("item_rod.rod_of_prefix", "Rod of ") == "魔杖：");
    REQUIRE(i18n::get("item_rod.rods_of_prefix", "Rods of ") == "魔杖：");
    REQUIRE(i18n::get("item_rod.a_rod_of_prefix", "a Rod of ") == "魔杖：");
    REQUIRE(i18n::get("item_rod.turns_left_open", "(") == "（");
    REQUIRE(i18n::get("item_rod.turns_left_suffix", " turns)") == "回合）");
    REQUIRE(i18n::get("item_rod.tried", "(Tried)") == "（已试）");
    REQUIRE(i18n::get("item_rod.displacement_name", "Displacement") == "置换");
    REQUIRE(
        i18n::get(
            "item_rod.displacement_descr",
            "When activated, this device moves the user a short distance.") ==
        "激活时，此装置会将使用者短距离移动。");
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
    REQUIRE(i18n::get("terrain.pick_up_query_prefix", "Pick up ") == "捡起");
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
    REQUIRE(i18n::get("terrain_door.attempt_to_open_it", "Attempt to open it?") == "要尝试打开它吗？");
    REQUIRE(i18n::get("terrain_door.open_query_prefix", "Open ") == "打开");
    REQUIRE(i18n::get("terrain_door.query_suffix", "?") == "？");
    REQUIRE(
        i18n::get(
            "terrain_door.currently_being_opened_cannot_close",
            "The door is currently being opened, "
            "and cannot be closed.") == "门正在被打开，无法关闭。");
    REQUIRE(i18n::get("terrain_door.blocked_prefix", "The ") == "那扇");
    REQUIRE(i18n::get("terrain_door.blocked_suffix", " is blocked.") == "被挡住了。");
    REQUIRE(i18n::get("terrain_door.break_open", "open") == "打开");
    REQUIRE(i18n::get("terrain_door.break_to_floor", "to the floor") == "到地上");
    REQUIRE(
        i18n::get(
            "terrain_door.something_blocking_prefix",
            "Something is blocking the ") == "有东西挡住了");
    REQUIRE(
        i18n::get(
            "terrain_door.shotgun_blown_to_pieces_suffix",
            " is blown to pieces!") == "被轰成碎片！");
    REQUIRE(
        i18n::get(
            "terrain_door.hear_door_crashing_open",
            "I hear a door crashing open!") == "我听到一扇门被撞开！");
    REQUIRE(
        i18n::get(
            "terrain_door.fumbles_blindly_fail_open_a",
            " fumbles blindly, and fails to open a ") == "盲目地摸索着，但没能打开一扇");
    REQUIRE(
        i18n::get(
            "terrain_door.fumble_blindly_close_prefix",
            "I fumble blindly with a ") == "我盲目地摸索着一扇");
    REQUIRE(i18n::get("terrain_door.fail_close_suffix", ", and fail to close it.") == "，但没能关上。");
    REQUIRE(i18n::get("terrain_door.legend_metal", "Door (metal)") == "门（金属）");
    REQUIRE(i18n::get("terrain_door.legend_warded", "Door (warded)") == "门（受守护）");
    REQUIRE(i18n::get("terrain_door.legend", "Door") == "门");
    REQUIRE(i18n::get("terrain_door.name_article_a", "a ") == "一扇");
    REQUIRE(i18n::get("terrain_door.name_article_an", "an ") == "一扇");
    REQUIRE(i18n::get("terrain_door.name_article_the", "the ") == "那扇");
    REQUIRE(i18n::get("terrain_door.name_modifier_burning", "burning ") == "燃烧的");
    REQUIRE(i18n::get("terrain_door.name_modifier_open", "open ") == "打开的");
    REQUIRE(i18n::get("terrain_door.name_modifier_stuck", "stuck ") == "卡住的");
    REQUIRE(i18n::get("terrain_door.name_wooden_door", "wooden door") == "木门");
    REQUIRE(i18n::get("terrain_door.name_warded_door", "warded door") == "受守护的门");
    REQUIRE(i18n::get("terrain_door.name_unwarded_door", "unwarded door") == "失去守护的门");
    REQUIRE(i18n::get("terrain_door.name_metal_door", "metal door") == "金属门");
    REQUIRE(i18n::get("terrain_door.name_barred_gate", "barred gate") == "铁栅门");
    REQUIRE(i18n::get("terrain_door.name_short_door", "door") == "门");
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
    REQUIRE(i18n::get("property.wounded_open", "Wounded(") == "受伤（");
    REQUIRE(i18n::get("property.close_paren", ")") == "）");
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
