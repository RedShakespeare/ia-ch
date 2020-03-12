// =============================================================================
// Copyright 2011-2020 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "sdl_base.hpp"

#include <iostream>

#include "SDL_image.h"
#include "SDL_mixer.h"

#include "audio.hpp"
#include "config.hpp"
#include "game_time.hpp"
#include "init.hpp"
#include "version.hpp"


static bool s_is_inited = false;


namespace sdl_base
{

void init()
{
        TRACE_FUNC_BEGIN;

        cleanup();

        s_is_inited = true;

        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS) == -1)
        {
                TRACE_ERROR_RELEASE << "Failed to init SDL"
                                    << std::endl
                                    << SDL_GetError()
                                    << std::endl;

                PANIC;
        }

        if (IMG_Init(IMG_INIT_PNG) == -1)
        {
                TRACE_ERROR_RELEASE << "Failed to init SDL_image"
                                    << std::endl
                                    << SDL_GetError()
                                    << std::endl;

                PANIC;
        }

        const int audio_freq = 44100;
        const Uint16 audio_format = MIX_DEFAULT_FORMAT;
        const int audio_channels = 2;
        const int audio_buffers = 1024;

        const int result =
                Mix_OpenAudio(
                        audio_freq,
                        audio_format,
                        audio_channels,
                        audio_buffers);

        if (result == -1)
        {
                TRACE_ERROR_RELEASE << "Failed to init SDL_mixer"
                                    << std::endl
                                    << SDL_GetError()
                                    << std::endl;

                ASSERT(false);
        }

        Mix_AllocateChannels(audio::g_allocated_channels);

        TRACE_FUNC_END;
}

void cleanup()
{
        if (!s_is_inited)
        {
                return;
        }

        s_is_inited = false;

        IMG_Quit();

        Mix_AllocateChannels(0);

        Mix_CloseAudio();

        SDL_Quit();
}

void sleep(const uint32_t duration)
{
        if (config::is_bot_playing())
        {
                return;
        }

        if (duration == 1)
        {
                SDL_Delay(duration);
        }
        else
        {
                // Duration longer than 1 ms
                const Uint32 wait_until = SDL_GetTicks() + duration;

                while (SDL_GetTicks() < wait_until)
                {
                        SDL_PumpEvents();
                }
        }
}

std::string sdl_pref_dir()
{
        std::string version_str;

        if (version_info::g_version_str.empty())
        {
                version_str = version_info::read_git_sha1_str_from_file();
        }
        else
        {
                version_str = version_info::g_version_str;
        }

        const auto path_ptr =
                // NOTE: This is somewhat of a hack, see the function arguments
                SDL_GetPrefPath(
                        "infra_arcana",         // "Organization"
                        version_str.c_str());   // "Application"

        const std::string path_str = path_ptr;

        SDL_free(path_ptr);

        TRACE << "User data directory: " << path_str << std::endl;

        return path_str;
}

} // namespace sdl_base
