// =============================================================================
// Copyright Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef SOUND_HPP
#define SOUND_HPP

#include <memory>
#include <string>

#include "audio_data.hpp"
#include "pos.hpp"

namespace actor
{
class Actor;
}  // namespace actor

enum class SndVol
{
    low,
    high,
    global
};

enum class AlertsMon
{
    no,
    yes
};

// This can be used for configuring sounds so that the sound message is ignored
// if the source is seen (typically because printing the sound message would be
// redundant or not make sense, e.g. the player can see the Cultist firing a
// pistol so there is no need to state that they hear it).
//
// If a creature is associated with the sound, then this creature is the source,
// otherwise the source is the origin location.
//
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
// SndSpec
// -----------------------------------------------------------------------------
// Parameter object for constructing a Snd without a long positional argument
// list. Build via chainable setters, then pass to Snd(std::string, SndSpec).
// Defaults mirror the in-class defaults on Snd's private members, so a
// default-constructed SndSpec matches a default-constructed Snd.
class SndSpec
{
public:
    SndSpec() = default;

    SndSpec& sfx(audio::SfxId v) { m_sfx = v; return *this; }

    SndSpec& ignore_msg_if_origin_seen(IgnoreMsgIfOriginSeen v)
    {
        m_is_msg_ignored_if_origin_seen = v;
        return *this;
    }

    SndSpec& origin(const P& v) { m_origin = v; return *this; }

    SndSpec& actor(actor::Actor* v) { m_actor_who_made_sound = v; return *this; }

    SndSpec& vol(SndVol v) { m_vol = v; return *this; }

    SndSpec& alerts(AlertsMon v) { m_is_alerting_mon = v; return *this; }

    SndSpec& heard_effect(std::shared_ptr<SndHeardEffect> v)
    {
        m_snd_heard_effect = std::move(v);
        return *this;
    }

    audio::SfxId sfx() const { return m_sfx; }

    IgnoreMsgIfOriginSeen ignore_msg_if_origin_seen() const
    {
        return m_is_msg_ignored_if_origin_seen;
    }

    const P& origin() const { return m_origin; }

    actor::Actor* actor() const { return m_actor_who_made_sound; }

    SndVol vol() const { return m_vol; }

    AlertsMon alerts() const { return m_is_alerting_mon; }

    const std::shared_ptr<SndHeardEffect>& heard_effect() const
    {
        return m_snd_heard_effect;
    }

private:
    audio::SfxId m_sfx = audio::SfxId::END;
    IgnoreMsgIfOriginSeen m_is_msg_ignored_if_origin_seen = IgnoreMsgIfOriginSeen::no;
    P m_origin {};
    actor::Actor* m_actor_who_made_sound = nullptr;
    SndVol m_vol = SndVol::low;
    AlertsMon m_is_alerting_mon = AlertsMon::no;
    std::shared_ptr<SndHeardEffect> m_snd_heard_effect;
};

// -----------------------------------------------------------------------------
// Sound
// -----------------------------------------------------------------------------
class Snd
{
public:
    // Construct from a message and a SndSpec parameter object.
    Snd(std::string msg, const SndSpec& spec);

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
        return m_is_msg_ignored_if_origin_seen == IgnoreMsgIfOriginSeen::yes;
    }

    bool is_alerting_mon() const
    {
        return m_is_alerting_mon == AlertsMon::yes;
    }

    void set_alerts_mon(AlertsMon alerts)
    {
        m_is_alerting_mon = alerts;
    }

    P origin() const
    {
        return m_origin;
    }

    actor::Actor* actor_who_made_sound() const
    {
        return m_actor_who_made_sound;
    }

    SndVol volume() const
    {
        return m_vol;
    }

    void add_string(const std::string& str)
    {
        m_msg += str;
    }

    void on_heard(actor::Actor& actor);

    bool did_player_hear_sound() const
    {
        return m_did_player_hear_sound;
    }

private:
    std::string m_msg;
    audio::SfxId m_sfx {audio::SfxId::END};
    IgnoreMsgIfOriginSeen m_is_msg_ignored_if_origin_seen {IgnoreMsgIfOriginSeen::no};
    P m_origin;
    actor::Actor* m_actor_who_made_sound {nullptr};
    SndVol m_vol {SndVol::low};
    AlertsMon m_is_alerting_mon {AlertsMon::no};
    std::shared_ptr<SndHeardEffect> m_snd_heard_effect;
    bool m_did_player_hear_sound {false};
};

// -----------------------------------------------------------------------------
// Sound emitting
// -----------------------------------------------------------------------------
namespace snd_emit
{
void run(Snd& snd);

}  // namespace snd_emit

#endif  // SOUND_HPP
