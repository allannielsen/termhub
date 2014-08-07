#ifndef __TERMHUB_CMD_REGISTRY__
#define __TERMHUB_CMD_REGISTRY__

#include <map>
#include <string>
#include <functional>

namespace TermHub {

struct CmdRegistry {
    typedef std::function<bool(const char *&b, const char *e)> cmd_t;

    bool cmd_add(const std::string &name, cmd_t c);
    bool cmd_del(const std::string &name);

    bool exec(const char *&b, const char *e);

  private:
    std::map<std::string, cmd_t> registry;
};

bool global_cmd_add(std::string name, CmdRegistry::cmd_t c);
bool global_cmd_del(std::string name);
bool global_cmd_exec(const std::string &name);
}

#endif  // __TERMHUB_CMD_REGISTRY__
