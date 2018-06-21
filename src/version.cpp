#include "version.hpp"

#define STRINGIFY(x) #x
#define TO_STRING(x) STRINGIFY(x)

namespace version_info
{

// This should be set when (and only when) building a tagged release. Use the
// format "vMAJOR.MINOR".
const std::string version_str = "";

const std::string copyright_str = "(c) 2011-2018 Martin Tornqvist";

const std::string date_str = __DATE__;

const std::string git_commit_hash_str = TO_STRING(GIT_COMMIT_HASH);

} // version_info
