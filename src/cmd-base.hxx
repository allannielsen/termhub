#ifndef __TERMHUB_CMD_BASE_HXX__
#define __TERMHUB_CMD_BASE_HXX__

#include <string>

namespace TermHub {
struct CmdBase {
    virtual bool process(const std::string &s) = 0;
};
}

#endif
