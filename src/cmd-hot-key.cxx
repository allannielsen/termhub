#include "cmd-hot-key.hxx"
#include "fmt.hxx"


namespace TermHub {
bool CmdHotKey::process(const std::string &s) {
    Fmt::Literal hotkey("hot-key", true, true);
    Fmt::Literal quit("quit", true, true);
    Fmt::Literal inject_lit("inject", true, true);

    type = Void;
    str1.clear();
    str2.clear();

    Fmt::EscapedString seq(str1);
    Fmt::EscapedString inject(str2);

    if (parse_group(s, hotkey, seq, quit)) {
        type = Quit;
        return true;

    } else if (parse_group(s, hotkey, seq, inject_lit, inject)) {
        type = Inject;
        return true;
    }

    return false;
}
}
