#ifndef GAME_COMMANDS_HPP
#define GAME_COMMANDS_HPP

struct InputData;

enum class GameCmd
{
        undefined,

        right,
        down,
        left,
        up,
        up_right,
        down_right,
        down_left,
        up_left,
        wait,
        wait_long,
        reload,
        kick,
        close,
        unload,
        fire,
        get,
        inventory,
        apply_item,
        drop_item,
        swap_weapon,
        auto_move,
        throw_item,
        look,
        auto_melee,
        cast_spell,
        make_noise,
        disarm,
        char_descr,
        minimap,
        msg_history,
        manual,
        options,
        game_menu,
        quit,
        f2,
        f3,

        // Debug commands
#ifndef NDEBUG
        f4,
        f5,
        f6,
        f7,
        f8,
        f9,
#endif  // NDEBUG
};

namespace game_commands
{

// NOTE: This is a pure function, except for reading the options
GameCmd to_cmd(const InputData& input);

void handle(const GameCmd cmd);

}

#endif // GAME_COMMANDS_HPP
