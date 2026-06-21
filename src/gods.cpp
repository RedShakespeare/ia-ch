// =============================================================================
// Copyright Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "gods.hpp"

#include <vector>

#include "i18n.hpp"
#include "random.hpp"

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------
struct GodDef
{
    std::string name;
    std::string descr_key;
    std::string descr_fallback;
};

static const std::vector<GodDef> s_god_list = {
    {"Abholos", "gods.abholos.descr", "The Devourer in the Mist"},
    {"Alala", "gods.alala.descr", "The Herald of S'glhuo"},
    {"Ammutseba", "gods.ammutseba.descr", "The Devourer of Stars"},
    {"Aphoom-Zhah", "gods.aphoom_zhah.descr", "The Cold Flame"},
    {"Apocolothoth", "gods.apocolothoth.descr", "The Moon-God"},
    {"Atlach-Nacha", "gods.atlach_nacha.descr", "The Spider God"},
    {"Ayiig", "gods.ayiig.descr", "The Serpent Goddess"},
    {"Aylith", "gods.aylith.descr", "The Widow in the Woods"},
    {"Baoht Zuqqa-Mogg", "gods.baoht_zuqqa_mogg.descr", "The Bringer of Pestilence"},
    {"Basatan", "gods.basatan.descr", "The Master of the Crabs"},
    {"Bgnu-Thun", "gods.bgnu_thun.descr", "The Soul-Chilling Ice-God"},
    {"Bokrug", "gods.bokrug.descr", "The Great Water Lizard"},
    {"Bugg-Shash", "gods.bugg_shash.descr", "The Black One"},
    {"Cthaat", "gods.cthaat.descr", "The Dark Water God"},
    {"Cthugha", "gods.cthugha.descr", "The Living Flame"},
    {"Cthylla", "gods.cthylla.descr", "The Secret Daughter of Cthulhu"},
    {"Ctoggha", "gods.ctoggha.descr", "The Dream-Daemon"},
    {"Cyaegha", "gods.cyaegha.descr", "The Destroying Eye"},
    {"Dygra", "gods.dygra.descr", "The Stone-Thing"},
    {"Dythalla", "gods.dythalla.descr", "The Lord of Lizards"},
    {"Eihort", "gods.eihort.descr", "The God of the Labyrinth"},
    {"Ghisguth", "gods.ghisguth.descr", "The Sound of the Deep Waters"},
    {"Glaaki", "gods.glaaki.descr", "The Lord of Dead Dreams"},
    {"Gleeth", "gods.gleeth.descr", "The Blind God of the Moon"},
    {"Gloon", "gods.gloon.descr", "The Corrupter of Flesh"},
    {"Gog-Hoor", "gods.gog_hoor.descr", "The Eater on the Insane"},
    {"Gol-goroth", "gods.gol_goroth.descr", "The God of the Black Stone"},
    {"Groth-Golka", "gods.groth_golka.descr", "The Demon Bird-God"},
    {"Gurathnaka", "gods.gurathnaka.descr", "The Eater of Dreams"},
    {"Han", "gods.han.descr", "The Dark One"},
    {"Hastur", "gods.hastur.descr", "The King in Yellow"},
    {"Hchtelegoth", "gods.hchtelegoth.descr", "The Great Tentacled God"},
    {"Hziulquoigmnzhah", "gods.hziulquoigmnzhah.descr", "The God of Cykranosh"},
    {"Inpesca", "gods.inpesca.descr", "The Sea Horror"},
    {"Iod", "gods.iod.descr", "The Shining Hunter"},
    {"Istasha", "gods.istasha.descr", "The Mistress of Darkness"},
    {"Ithaqua", "gods.ithaqua.descr", "The God of the Cold White Silence"},
    {"Janaingo", "gods.janaingo.descr", "The Guardian of the Watery Gates"},
    {"Kaalut", "gods.kaalut.descr", "The Ravenous One"},
    {"Kassogtha", "gods.kassogtha.descr", "The Leviathan of Disease"},
    {"Kaunuzoth", "gods.kaunuzoth.descr", "The Great One"},
    {"Lam", "gods.lam.descr", "The Grey"},
    {"Lythalia", "gods.lythalia.descr", "The Forest-Goddess"},
    {"Mnomquah", "gods.mnomquah.descr", "The Lord of the Black Lake"},
    {"Mordiggian", "gods.mordiggian.descr", "The Great Ghoul"},
    {"Mynoghra", "gods.mynoghra.descr", "The She-Daemon of the Shadows"},
    {"Ngirrthlu", "gods.ngirrthlu.descr", "The Wolf-Thing"},
    {"Northot", "gods.northot.descr", "The Forgotten God"},
    {"Nssu-Ghahnb", "gods.nssu_ghahnb.descr", "The Leech of the Aeons"},
    {"Nycrama", "gods.nycrama.descr", "The Zombifying Essence"},
    {"Nyogtha", "gods.nyogtha.descr", "The Haunter of the Red Abyss"},
    {"Obmbu", "gods.obmbu.descr", "The Shatterer"},
    {"Othuum", "gods.othuum.descr", "The Oceanic Horror"},
    {"Othuyeg", "gods.othuyeg.descr", "The Doom-Walker"},
    {"Psuchawrl", "gods.psuchawrl.descr", "The Elder One"},
    {"Quyagen", "gods.quyagen.descr", "He Who Dwells Beneath Our Feet"},
    {"Rhan-Tegoth", "gods.rhan_tegoth.descr", "He of the Ivory Throne"},
    {"Rlim Shaikorth", "gods.rlim_shaikorth.descr", "The White Worm"},
    {"Ruhtra Dyoll", "gods.ruhtra_dyoll.descr", "The Fire God"},
    {"Sebek", "gods.sebek.descr", "The Crocodile God"},
    {"Sedmelluq", "gods.sedmelluq.descr", "The Great Manipulator"},
    {"Sfatlicllp", "gods.sfatlicllp.descr", "The Fallen Wisdom"},
    {"Shaklatal", "gods.shaklatal.descr", "The Eye of Wicked Sight"},
    {"Sheb-Teth", "gods.sheb_teth.descr", "The Devourer of Souls"},
    {"Shterot", "gods.shterot.descr", "The Tenebrous One"},
    {"Shudde M'ell", "gods.shudde_mell.descr", "The Burrower Beneath"},
    {"Shuy-Nihl", "gods.shuy_nihl.descr", "The Devourer in the Earth"},
    {"Sthanee", "gods.sthanee.descr", "The Lost One"},
    {"Summanus", "gods.summanus.descr", "The Monarch of Night"},
    {"Thanaroa", "gods.thanaroa.descr", "The Shining One"},
    {"Tharapithia", "gods.tharapithia.descr", "The Shadow in the Crimson Light"},
    {"Thog", "gods.thog.descr", "The Demon-God of Xuthal"},
    {"Thrygh", "gods.thrygh.descr", "The Godbeast"},
    {"Tsathoggua", "gods.tsathoggua.descr", "The Toad-God"},
    {"Tulushuggua", "gods.tulushuggua.descr", "The Watery Dweller Beneath"},
    {"Vibur", "gods.vibur.descr", "The Thing from Beyond"},
    {"Volgna-Gath", "gods.volgna_gath.descr", "The Keeper of the Secrets"},
    {"Vthyarilops", "gods.vthyarilops.descr", "The Starfish God"},
    {"Xalafu", "gods.xalafu.descr", "The Dread One"},
    {"Xcthol", "gods.xcthol.descr", "The Goat God"},
    {"Xinlurgash", "gods.xinlurgash.descr", "The Ever-Consuming"},
    {"Xirdneth", "gods.xirdneth.descr", "The Maker of Illusions"},
    {"Xotli", "gods.xotli.descr", "The Lord of Terror"},
    {"Yegg-Ha", "gods.yegg_ha.descr", "The Faceless One"},
    {"Ygolonac", "gods.ygolonac.descr", "The Defiler"},
    {"Yig", "gods.yig.descr", "The Father of Serpents"},
    {"Ymnar", "gods.ymnar.descr", "The Dark Stalker"},
    {"Yog-Sapha", "gods.yog_sapha.descr", "The Dweller in the Depths"},
    {"Yorith", "gods.yorith.descr", "The Oldest Dreamer"},
    {"Ythogtha", "gods.ythogtha.descr", "The Thing in the Pit"},
    {"Yug-Siturath", "gods.yug_siturath.descr", "The All-Consuming Fog"},
    {"Zathog", "gods.zathog.descr", "The Black Lord of Whirling Vortices"},
    {"Zindarak", "gods.zindarak.descr", "The Fiery Messenger"},
    {"Zushakon", "gods.zushakon.descr", "The Dark Silent One"},
    {"Zvilpogghua", "gods.zvilpogghua.descr", "The Feaster from the Stars"},
    {"Gozer", "gods.gozer.descr", "The Destroyer"}};

static int s_current_god_idx = 0;

static God s_current_god;

// -----------------------------------------------------------------------------
// gods
// -----------------------------------------------------------------------------
namespace gods
{
const God& current_god()
{
    const GodDef& god = s_god_list[s_current_god_idx];

    s_current_god = {
        god.name,
        i18n::get(god.descr_key, god.descr_fallback)};

    return s_current_god;
}

void set_random_god()
{
    const int nr_gods = (int)s_god_list.size();

    s_current_god_idx = rnd::range(0, nr_gods - 1);
}

}  // namespace gods
