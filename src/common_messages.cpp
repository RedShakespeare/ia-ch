#include "common_messages.hpp"

namespace common_messages
{

const std::string info_screen_tip =
        "[space/esc] to exit";

const std::string info_screen_tip_scrollable =
        "[2/8, down/up, j/k] to scroll " +
        info_screen_tip;

const std::string cancel_info =
        "[space/esc] to cancel";

const std::string confirm_info =
        "[space/esc/enter] to continue";

const std::string any_key_info =
        "[Any key] to continue";

const std::string disarm_no_trap =
        "I find nothing there to disarm.";

const std::string mon_prevent_cmd =
        "Not while an enemy is near.";

const std::string fire_prevent_cmd =
        "Fire is spreading!";

const std::string mon_disappear =
        " suddenly disappears!";

} // common_messages
