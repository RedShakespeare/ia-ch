// =============================================================================
// Copyright Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef INSANITY_HPP
#define INSANITY_HPP

#include "i18n.hpp"

#include <string>
#include <vector>

namespace actor
{
class Actor;
}  // namespace actor

namespace insanity_i18n
{
inline std::string get(const std::string& key, const std::string& fallback)
{
    return i18n::get("insanity." + key, fallback);
}
}  // namespace insanity_i18n

enum class InsSymptId
{
    reduce_xp,
    scream,
    babbling,
    faint,
    laugh,
    phobia_rat,
    phobia_spider,
    phobia_reptile_and_amph,
    phobia_canine,
    phobia_dead,
    phobia_deep,
    phobia_dark,
    sadism,
    shadows,
    paranoia,  // Invisible stalker spawned
    confusion,
    frenzy,
    strange_sensation,
    END
};

enum class InsSymptType
{
    phobia,
    misc
};

class InsSympt
{
public:
    InsSympt() = default;

    virtual ~InsSympt() = default;

    virtual InsSymptId id() const = 0;

    virtual InsSymptType type() const = 0;

    virtual void save() const {}

    virtual void load() {}

    virtual bool is_permanent() const = 0;

    virtual bool is_allowed() const
    {
        return true;
    }

    void on_start();

    void on_end();

    virtual void on_new_player_turn(
        const std::vector<actor::Actor*>& seen_actors)
    {
        (void)seen_actors;
    }

    virtual void on_permanent_rfear() {}

    virtual std::string char_descr_msg() const
    {
        return "";
    }

    virtual std::string game_over_summary_msg() const
    {
        return "";
    }

protected:
    virtual void on_start_hook() {}

    virtual std::string start_msg() const = 0;
    virtual std::string start_heading() const = 0;

    virtual std::string end_msg() const
    {
        return "";
    }

    virtual std::string history_msg() const = 0;

    virtual std::string history_msg_end() const
    {
        return "";
    }
};

class InsReduceXp : public InsSympt
{
public:
    InsReduceXp() = default;

    InsSymptId id() const override
    {
        return InsSymptId::reduce_xp;
    }

    InsSymptType type() const override
    {
        return InsSymptType::misc;
    }

    bool is_permanent() const override
    {
        return false;
    }

    bool is_allowed() const override;

protected:
    void on_start_hook() override;

    std::string start_msg() const override;

    std::string start_heading() const override
    {
        return insanity_i18n::get(
            "reduce_xp.start_heading",
            "Experiences erased!");
    }

    std::string history_msg() const override
    {
        return insanity_i18n::get(
            "reduce_xp.history",
            "Experiences were erased from memory");
    }
};

class InsScream : public InsSympt
{
public:
    InsScream() = default;

    InsSymptId id() const override
    {
        return InsSymptId::scream;
    }

    InsSymptType type() const override
    {
        return InsSymptType::misc;
    }

    bool is_permanent() const override
    {
        return false;
    }

    bool is_allowed() const override;

protected:
    void on_start_hook() override;

    std::string start_msg() const override;

    std::string start_heading() const override
    {
        return insanity_i18n::get("scream.start_heading", "Screaming!");
    }

    std::string history_msg() const override
    {
        return insanity_i18n::get("scream.history", "Screamed in terror");
    }
};

class InsBabbling : public InsSympt
{
public:
    InsBabbling() = default;

    InsSymptId id() const override
    {
        return InsSymptId::babbling;
    }

    InsSymptType type() const override
    {
        return InsSymptType::misc;
    }

    bool is_permanent() const override
    {
        return true;
    }

    void on_new_player_turn(
        const std::vector<actor::Actor*>& seen_actors) override;

    std::string char_descr_msg() const override
    {
        return insanity_i18n::get("babbling.char_descr", "Babbling");
    }

    std::string game_over_summary_msg() const override
    {
        return insanity_i18n::get(
            "babbling.summary",
            "Had a tendency to babble");
    }

    void babble() const;

protected:
    void on_start_hook() override;

    std::string start_msg() const override
    {
        return insanity_i18n::get(
            "babbling.start",
            "I find myself babbling incoherently.");
    }

    std::string start_heading() const override
    {
        return insanity_i18n::get("babbling.start_heading", "Babbling!");
    }

    std::string end_msg() const override
    {
        return insanity_i18n::get(
            "babbling.end",
            "I feel in control of my speech.");
    }

    std::string history_msg() const override
    {
        return insanity_i18n::get(
            "babbling.history",
            "Started babbling incoherently");
    }

    std::string history_msg_end() const override
    {
        return insanity_i18n::get(
            "babbling.history_end",
            "My strange babbling was cured");
    }
};

class InsFaint : public InsSympt
{
public:
    InsFaint() = default;

    InsSymptId id() const override
    {
        return InsSymptId::faint;
    }

    InsSymptType type() const override
    {
        return InsSymptType::misc;
    }

    bool is_permanent() const override
    {
        return false;
    }

    bool is_allowed() const override;

protected:
    void on_start_hook() override;

    std::string start_msg() const override
    {
        return insanity_i18n::get("faint.start", "Everything is blacking out.");
    }

    std::string start_heading() const override
    {
        return insanity_i18n::get("faint.start_heading", "Fainting!");
    }

    std::string history_msg() const override
    {
        return insanity_i18n::get("faint.history", "Fainted");
    }
};

class InsLaugh : public InsSympt
{
public:
    InsLaugh() = default;

    InsSymptId id() const override
    {
        return InsSymptId::laugh;
    }

    InsSymptType type() const override
    {
        return InsSymptType::misc;
    }

    bool is_permanent() const override
    {
        return false;
    }

protected:
    void on_start_hook() override;

    std::string start_msg() const override
    {
        return insanity_i18n::get("laugh.start", "I laugh maniacally.");
    }

    std::string start_heading() const override
    {
        return insanity_i18n::get("laugh.start_heading", "HAHAHA!");
    }

    std::string history_msg() const override
    {
        return insanity_i18n::get("laugh.history", "Laughed maniacally");
    }
};

class InsPhobiaRat : public InsSympt
{
public:
    InsPhobiaRat() = default;

    InsSymptId id() const override
    {
        return InsSymptId::phobia_rat;
    }

    InsSymptType type() const override
    {
        return InsSymptType::phobia;
    }

    bool is_permanent() const override
    {
        return true;
    }

    void on_new_player_turn(
        const std::vector<actor::Actor*>& seen_actors) override;

    void on_permanent_rfear() override;

    bool is_allowed() const override;

    std::string char_descr_msg() const override
    {
        return insanity_i18n::get("phobia_rat.char_descr", "Phobia of rats");
    }

    std::string game_over_summary_msg() const override
    {
        return insanity_i18n::get(
            "phobia_rat.summary",
            "Had a phobia of rats");
    }

protected:
    std::string start_msg() const override
    {
        return insanity_i18n::get(
            "phobia_rat.start",
            "Rats suddenly seem terrifying.");
    }

    std::string start_heading() const override
    {
        return insanity_i18n::get("phobia_rat.start_heading", "Murophobia!");
    }

    std::string end_msg() const override
    {
        return insanity_i18n::get(
            "phobia_rat.end",
            "I am no longer terrified of rats.");
    }

    std::string history_msg() const override
    {
        return insanity_i18n::get(
            "phobia_rat.history",
            "Gained a phobia of rats");
    }

    std::string history_msg_end() const override
    {
        return insanity_i18n::get(
            "phobia_rat.history_end",
            "My phobia of rats was cured");
    }
};

class InsPhobiaSpider : public InsSympt
{
public:
    InsPhobiaSpider() = default;

    InsSymptId id() const override
    {
        return InsSymptId::phobia_spider;
    }

    InsSymptType type() const override
    {
        return InsSymptType::phobia;
    }

    bool is_permanent() const override
    {
        return true;
    }

    void on_new_player_turn(
        const std::vector<actor::Actor*>& seen_actors) override;

    void on_permanent_rfear() override;

    bool is_allowed() const override;

    std::string char_descr_msg() const override
    {
        return insanity_i18n::get(
            "phobia_spider.char_descr",
            "Phobia of spiders");
    }

    std::string game_over_summary_msg() const override
    {
        return insanity_i18n::get(
            "phobia_spider.summary",
            "Had a phobia of spiders");
    }

protected:
    std::string start_msg() const override
    {
        return insanity_i18n::get(
            "phobia_spider.start",
            "Spiders suddenly seem terrifying.");
    }

    std::string start_heading() const override
    {
        return insanity_i18n::get(
            "phobia_spider.start_heading",
            "Arachnophobia!");
    }

    std::string end_msg() const override
    {
        return insanity_i18n::get(
            "phobia_spider.end",
            "I am no longer terrified of spiders.");
    }

    std::string history_msg() const override
    {
        return insanity_i18n::get(
            "phobia_spider.history",
            "Gained a phobia of spiders");
    }

    std::string history_msg_end() const override
    {
        return insanity_i18n::get(
            "phobia_spider.history_end",
            "My phobia of spiders was cured");
    }
};

class InsPhobiaReptileAndAmph : public InsSympt
{
public:
    InsPhobiaReptileAndAmph() = default;

    InsSymptId id() const override
    {
        return InsSymptId::phobia_reptile_and_amph;
    }

    InsSymptType type() const override
    {
        return InsSymptType::phobia;
    }

    bool is_permanent() const override
    {
        return true;
    }

    void on_new_player_turn(
        const std::vector<actor::Actor*>& seen_actors) override;

    void on_permanent_rfear() override;

    bool is_allowed() const override;

    std::string char_descr_msg() const override
    {
        return insanity_i18n::get(
            "phobia_reptile_and_amph.char_descr",
            "Phobia of reptiles and amphibians");
    }

    std::string game_over_summary_msg() const override
    {
        return insanity_i18n::get(
            "phobia_reptile_and_amph.summary",
            "Had a phobia of reptiles and amphibians");
    }

protected:
    std::string start_msg() const override
    {
        return insanity_i18n::get(
            "phobia_reptile_and_amph.start",
            "Reptiles and amphibians suddenly seem terrifying.");
    }

    std::string start_heading() const override
    {
        return insanity_i18n::get(
            "phobia_reptile_and_amph.start_heading",
            "Herpetophobia!");
    }

    std::string end_msg() const override
    {
        return insanity_i18n::get(
            "phobia_reptile_and_amph.end",
            "I am no longer terrified of reptiles and amphibians.");
    }

    std::string history_msg() const override
    {
        return insanity_i18n::get(
            "phobia_reptile_and_amph.history",
            "Gained a phobia of reptiles and amphibians");
    }

    std::string history_msg_end() const override
    {
        return insanity_i18n::get(
            "phobia_reptile_and_amph.history_end",
            "My phobia of reptiles and amphibians was cured");
    }
};

class InsPhobiaCanine : public InsSympt
{
public:
    InsPhobiaCanine() = default;

    InsSymptId id() const override
    {
        return InsSymptId::phobia_canine;
    }

    InsSymptType type() const override
    {
        return InsSymptType::phobia;
    }

    bool is_permanent() const override
    {
        return true;
    }

    void on_new_player_turn(
        const std::vector<actor::Actor*>& seen_actors) override;

    void on_permanent_rfear() override;

    bool is_allowed() const override;

    std::string char_descr_msg() const override
    {
        return insanity_i18n::get(
            "phobia_canine.char_descr",
            "Phobia of canines");
    }

    std::string game_over_summary_msg() const override
    {
        return insanity_i18n::get(
            "phobia_canine.summary",
            "Had a phobia of canines");
    }

protected:
    std::string start_msg() const override
    {
        return insanity_i18n::get(
            "phobia_canine.start",
            "Canines suddenly seem terrifying.");
    }

    std::string start_heading() const override
    {
        return insanity_i18n::get(
            "phobia_canine.start_heading",
            "Cynophobia!");
    }

    std::string end_msg() const override
    {
        return insanity_i18n::get(
            "phobia_canine.end",
            "I am no longer terrified of canines.");
    }

    std::string history_msg() const override
    {
        return insanity_i18n::get(
            "phobia_canine.history",
            "Gained a phobia of canines");
    }

    std::string history_msg_end() const override
    {
        return insanity_i18n::get(
            "phobia_canine.history_end",
            "My phobia of canines was cured");
    }
};

class InsPhobiaDead : public InsSympt
{
public:
    InsPhobiaDead() = default;

    InsSymptId id() const override
    {
        return InsSymptId::phobia_dead;
    }

    InsSymptType type() const override
    {
        return InsSymptType::phobia;
    }

    bool is_permanent() const override
    {
        return true;
    }

    void on_new_player_turn(
        const std::vector<actor::Actor*>& seen_actors) override;

    void on_permanent_rfear() override;

    bool is_allowed() const override;

    std::string char_descr_msg() const override
    {
        return insanity_i18n::get(
            "phobia_dead.char_descr",
            "Phobia of the dead");
    }

    std::string game_over_summary_msg() const override
    {
        return insanity_i18n::get(
            "phobia_dead.summary",
            "Had a phobia of the dead");
    }

protected:
    std::string start_msg() const override
    {
        return insanity_i18n::get(
            "phobia_dead.start",
            "The dead suddenly seem far more terrifying.");
    }

    std::string start_heading() const override
    {
        return insanity_i18n::get("phobia_dead.start_heading", "Necrophobia!");
    }

    std::string end_msg() const override
    {
        return insanity_i18n::get(
            "phobia_dead.end",
            "I am no longer terrified of the dead.");
    }

    std::string history_msg() const override
    {
        return insanity_i18n::get(
            "phobia_dead.history",
            "Gained a phobia of the dead");
    }

    std::string history_msg_end() const override
    {
        return insanity_i18n::get(
            "phobia_dead.history_end",
            "My phobia of the dead was cured");
    }
};

class InsPhobiaDeep : public InsSympt
{
public:
    InsPhobiaDeep() = default;

    InsSymptId id() const override
    {
        return InsSymptId::phobia_deep;
    }

    InsSymptType type() const override
    {
        return InsSymptType::phobia;
    }

    bool is_permanent() const override
    {
        return true;
    }

    void on_new_player_turn(
        const std::vector<actor::Actor*>& seen_actors) override;

    void on_permanent_rfear() override;

    bool is_allowed() const override;

    std::string char_descr_msg() const override
    {
        return insanity_i18n::get(
            "phobia_deep.char_descr",
            "Phobia of deep places");
    }

    std::string game_over_summary_msg() const override
    {
        return insanity_i18n::get(
            "phobia_deep.summary",
            "Had a phobia of deep places");
    }

protected:
    std::string start_msg() const override
    {
        return insanity_i18n::get(
            "phobia_deep.start",
            "It suddenly seems far more terrifying to delve deeper.");
    }

    std::string start_heading() const override
    {
        return insanity_i18n::get("phobia_deep.start_heading", "Bathophobia!");
    }

    std::string end_msg() const override
    {
        return insanity_i18n::get(
            "phobia_deep.end",
            "I am no longer terrified of deep places.");
    }

    std::string history_msg() const override
    {
        return insanity_i18n::get(
            "phobia_deep.history",
            "Gained a phobia of deep places");
    }

    std::string history_msg_end() const override
    {
        return insanity_i18n::get(
            "phobia_deep.history_end",
            "My phobia of deep places was cured");
    }
};

class InsPhobiaDark : public InsSympt
{
public:
    InsPhobiaDark() = default;

    InsSymptId id() const override
    {
        return InsSymptId::phobia_dark;
    }

    InsSymptType type() const override
    {
        return InsSymptType::phobia;
    }

    bool is_permanent() const override
    {
        return true;
    }

    void on_new_player_turn(
        const std::vector<actor::Actor*>& seen_actors) override;

    void on_permanent_rfear() override;

    bool is_allowed() const override;

    std::string char_descr_msg() const override
    {
        return insanity_i18n::get(
            "phobia_dark.char_descr",
            "Phobia of darkness");
    }

    std::string game_over_summary_msg() const override
    {
        return insanity_i18n::get(
            "phobia_dark.summary",
            "Had a phobia of darkness");
    }

protected:
    std::string start_msg() const override
    {
        return insanity_i18n::get(
            "phobia_dark.start",
            "Darkness suddenly seems far more terrifying.");
    }

    std::string start_heading() const override
    {
        return insanity_i18n::get("phobia_dark.start_heading", "Nyctophobia!");
    }

    std::string end_msg() const override
    {
        return insanity_i18n::get(
            "phobia_dark.end",
            "I am no longer terrified of darkness.");
    }

    std::string history_msg() const override
    {
        return insanity_i18n::get(
            "phobia_dark.history",
            "Gained a phobia of darkness");
    }

    std::string history_msg_end() const override
    {
        return insanity_i18n::get(
            "phobia_dark.history_end",
            "My phobia of darkness was cured");
    }
};

class InsSadism : public InsSympt
{
public:
    InsSadism() = default;

    InsSymptId id() const override
    {
        return InsSymptId::sadism;
    }

    InsSymptType type() const override
    {
        return InsSymptType::misc;
    }

    bool is_permanent() const override
    {
        return true;
    }

    bool is_allowed() const override;

    std::string char_descr_msg() const override
    {
        return insanity_i18n::get("sadism.char_descr", "Sadistic obsession");
    }

    std::string game_over_summary_msg() const override
    {
        return insanity_i18n::get(
            "sadism.summary",
            "Had a sadistic obsession");
    }

protected:
    std::string start_msg() const override
    {
        return insanity_i18n::get(
            "sadism.start",
            "To my alarm, I find myself encouraged by the pain I "
            "cause in others. For every significant life I take, I "
            "find a little relief. However, my depraved mind will "
            "never find complete peace.");
    }

    std::string start_heading() const override
    {
        return insanity_i18n::get(
            "sadism.start_heading",
            "Sadistic obsession!");
    }

    std::string end_msg() const override
    {
        return insanity_i18n::get(
            "sadism.end",
            "I am cured of my sadistic obsession.");
    }

    std::string history_msg() const override
    {
        return insanity_i18n::get(
            "sadism.history",
            "Gained a sadistic obsession");
    }

    std::string history_msg_end() const override
    {
        return insanity_i18n::get(
            "sadism.history_end",
            "My sadistic obsession was cured");
    }
};

class InsShadows : public InsSympt
{
public:
    InsShadows() = default;

    InsSymptId id() const override
    {
        return InsSymptId::shadows;
    }

    InsSymptType type() const override
    {
        return InsSymptType::misc;
    }

    bool is_permanent() const override
    {
        return false;
    }

protected:
    void on_start_hook() override;

    std::string start_msg() const override
    {
        return insanity_i18n::get(
            "shadows.start",
            "The shadows are closing in on me!");
    }

    std::string start_heading() const override
    {
        return insanity_i18n::get(
            "shadows.start_heading",
            "Haunted by shadows!");
    }

    std::string history_msg() const override
    {
        return insanity_i18n::get(
            "shadows.history",
            "Was haunted by shadows");
    }
};

class InsParanoia : public InsSympt
{
public:
    InsParanoia() = default;

    InsSymptId id() const override
    {
        return InsSymptId::paranoia;
    }

    InsSymptType type() const override
    {
        return InsSymptType::misc;
    }

    bool is_permanent() const override
    {
        return false;
    }

protected:
    void on_start_hook() override;

    std::string start_msg() const override
    {
        return insanity_i18n::get(
            "paranoia.start",
            "Is there someone following me? Or is it panic taking "
            "over?");
    }

    std::string start_heading() const override
    {
        return insanity_i18n::get("paranoia.start_heading", "Paranoia!");
    }

    std::string history_msg() const override
    {
        return insanity_i18n::get(
            "paranoia.history",
            "Had a strong sensation of being followed");
    }
};

class InsConfusion : public InsSympt
{
public:
    InsConfusion() = default;

    InsSymptId id() const override
    {
        return InsSymptId::confusion;
    }

    InsSymptType type() const override
    {
        return InsSymptType::misc;
    }

    bool is_permanent() const override
    {
        return false;
    }

    bool is_allowed() const override;

protected:
    void on_start_hook() override;

    std::string start_msg() const override
    {
        return insanity_i18n::get(
            "confusion.start",
            "I find myself in a peculiar trance. I struggle to "
            "recall where I am, and what is happening.");
    }

    std::string start_heading() const override
    {
        return insanity_i18n::get("confusion.start_heading", "Confusion!");
    }

    std::string history_msg() const override
    {
        return insanity_i18n::get(
            "confusion.history",
            "Suddenly felt deeply confused for no reason");
    }
};

class InsFrenzy : public InsSympt
{
public:
    InsFrenzy() = default;

    InsSymptId id() const override
    {
        return InsSymptId::frenzy;
    }

    InsSymptType type() const override
    {
        return InsSymptType::misc;
    }

    bool is_permanent() const override
    {
        return false;
    }

    bool is_allowed() const override;

protected:
    void on_start_hook() override;

    std::string start_msg() const override
    {
        return insanity_i18n::get(
            "frenzy.start",
            "I fall into an uncontrollable rage!");
    }

    std::string start_heading() const override
    {
        return insanity_i18n::get("frenzy.start_heading", "Frenzy!");
    }

    std::string history_msg() const override
    {
        return insanity_i18n::get(
            "frenzy.history",
            "Fell into an uncontrollable rage");
    }
};

class InsStrangeSensation : public InsSympt
{
public:
    InsStrangeSensation() = default;

    InsSymptId id() const override
    {
        return InsSymptId::strange_sensation;
    }

    InsSymptType type() const override
    {
        return InsSymptType::misc;
    }

    bool is_permanent() const override
    {
        return false;
    }

protected:
    std::string start_msg() const override
    {
        return insanity_i18n::get(
            "strange_sensation.start",
            "There is a strange itch, as if something is crawling "
            "on the back of my neck.");
    }

    std::string start_heading() const override
    {
        return insanity_i18n::get(
            "strange_sensation.start_heading",
            "Strange sensation!");
    }

    std::string history_msg() const override
    {
        return insanity_i18n::get(
            "strange_sensation.history",
            "Had a sensation of something crawling on my neck");
    }
};

namespace insanity
{
void init();
void cleanup();

void save();
void load();

void run_sympt();

bool has_sympt(InsSymptId id);

bool has_sympt_type(InsSymptType type);

std::vector<const InsSympt*> active_sympts();

void on_new_player_turn(const std::vector<actor::Actor*>& seen_actors);

void on_permanent_rfear();

void end_sympt(InsSymptId id);

}  // namespace insanity

#endif  // INSANITY_HPP
