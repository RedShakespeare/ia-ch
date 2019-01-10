// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
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

static bool is_inited = false;

namespace sdl_base
{

void init()
{
        TRACE_FUNC_BEGIN;

        cleanup();

        is_inited = true;

        if (SDL_Init(SDL_INIT_EVERYTHING) == -1)
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
                Mix_OpenAudio(audio_freq,
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

        Mix_AllocateChannels(audio::allocated_channels);

        TRACE_FUNC_END;
}

void cleanup()
{
        if (!is_inited)
        {
                return;
        }

        is_inited = false;

        IMG_Quit();

        Mix_AllocateChannels(0);

        Mix_CloseAudio();

        SDL_Quit();
}

void sleep(const Uint32 duration)
{
        if (config::is_bot_playing())
        {
                return;
        }

        if (duration == 1)
        {
                SDL_Delay(duration);
        }
        else // Duration longer than 1 ms
        {
                const Uint32 wait_until = SDL_GetTicks() + duration;

                while (SDL_GetTicks() < wait_until)
                {
                        SDL_PumpEvents();
                }
        }
}

} // sdl_base
