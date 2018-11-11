#include "common_text.hpp"

namespace common_text
{

const std::string next_page_up_hint =
        "[page up, <] more";

const std::string next_page_down_hint =
        "[page down, >] more";

const std::string screen_exit_hint =
        "[space, esc] to exit";

const std::string scrollable_info_screen_hint =
        "[2/8, down/up] to scroll " +
        screen_exit_hint;

const std::string cancel_hint =
        "[space, esc] to cancel";

const std::string confirm_hint =
        "[space, esc, enter] to continue";

const std::string any_key_hint =
        "[any key] to continue";

const std::string yes_or_no_hint =
        "[y/n]";

const std::string disarm_no_trap =
        "I find nothing there to disarm.";

const std::string mon_prevent_cmd =
        "Not while an enemy is near.";

const std::string fire_prevent_cmd =
        "Fire is spreading!";

const std::string mon_disappear =
        "suddenly disappears!";

} // common_text
