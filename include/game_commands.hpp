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

        // Debug commands
#ifndef NDEBUG
        debug_f2,
        debug_f3,
        debug_f4,
        debug_f5,
        debug_f6,
        debug_f7,
        debug_f8,
        debug_f9,
#endif  // NDEBUG
};

namespace game_commands
{

GameCmd to_cmd(const InputData& input);

void handle(const GameCmd cmd);

}

#endif // GAME_COMMANDS_HPP
