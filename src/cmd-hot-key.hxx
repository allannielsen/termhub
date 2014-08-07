#ifndef __TERMHUB_CMD_HOT_KEY_HXX__
#define __TERMHUB_CMD_HOT_KEY_HXX__

#include <string>
#include "cmd-base.hxx"

namespace TermHub {
struct CmdHotKey : public CmdBase {
    enum Type {
        Void,
        Quit,
        Inject,
    };

    bool process(const std::string &s);

    Type type = Void;
    std::string str1;
    std::string str2;
};
}

#endif

