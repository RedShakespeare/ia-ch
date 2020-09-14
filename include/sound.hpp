// =============================================================================
// Copyright 2011-2020 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef SOUND_HPP
#define SOUND_HPP

#include <string>

#include "item_data.hpp"
#include "msg_log.hpp"

namespace actor
{
class Actor;
}  // namespace actor

enum class SndVol
{
        low,
        high
};

enum class AlertsMon
{
        no,
        yes
};

enum class IgnoreMsgIfOriginSeen
{
        no,
        yes
};

class SndHeardEffect
{
public:
        SndHeardEffect() = default;

        virtual ~SndHeardEffect() = default;

        virtual void run(actor::Actor& actor) const = 0;
};

// -----------------------------------------------------------------------------
// Sound
// -----------------------------------------------------------------------------
class Snd
{
public:
        Snd(std::string msg,
            audio::SfxId sfx,
            IgnoreMsgIfOriginSeen ignore_msg_if_origin_seen,
            const P& origin,
            actor::Actor* actor_who_made_sound,
            SndVol vol,
            AlertsMon alerting_mon,
            MorePromptOnMsg add_more_prompt_on_msg = MorePromptOnMsg::no,
            std::shared_ptr<SndHeardEffect> snd_heard_effect = nullptr);

        Snd() = default;

        ~Snd();

        void run();

        const std::string& msg() const
        {
                return m_msg;
        }

        void clear_msg()
        {
                m_msg = "";
        }

        audio::SfxId sfx() const
        {
                return m_sfx;
        }

        void clear_sfx()
        {
                m_sfx = audio::SfxId::END;
        }

        bool is_msg_ignored_if_origin_seen() const
        {
                return m_is_msg_ignored_if_origin_seen ==
                        IgnoreMsgIfOriginSeen::yes;
        }

        bool is_alerting_mon() const
        {
                return m_is_alerting_mon == AlertsMon::yes;
        }

        void set_alerts_mon(AlertsMon alerts)
        {
                m_is_alerting_mon = alerts;
        }

        MorePromptOnMsg should_add_more_prompt_on_msg() const
        {
                return m_add_more_prompt_on_msg;
        }

        P origin() const
        {
                return m_origin;
        }

        actor::Actor* actor_who_made_sound() const
        {
                return m_actor_who_made_sound;
        }

        int is_loud() const
        {
                return m_vol == SndVol::high;
        }

        void add_string(const std::string& str)
        {
                m_msg += str;
        }

        void on_heard(actor::Actor& actor) const;

private:
        std::string m_msg;
        audio::SfxId m_sfx;
        IgnoreMsgIfOriginSeen m_is_msg_ignored_if_origin_seen;
        P m_origin;
        actor::Actor* m_actor_who_made_sound;
        SndVol m_vol;
        AlertsMon m_is_alerting_mon;
        MorePromptOnMsg m_add_more_prompt_on_msg;
        std::shared_ptr<SndHeardEffect> m_snd_heard_effect;
};

// -----------------------------------------------------------------------------
// Sound emitting
// -----------------------------------------------------------------------------
namespace snd_emit
{
void run(Snd snd);

void reset_nr_snd_msg_printed_current_turn();

}  // namespace snd_emit

#endif  // SOUND_HPP
