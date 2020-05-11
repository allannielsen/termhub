#include "cmd-registry.hxx"
#include "log.hxx"
#include "fmt.hxx"

namespace TermHub {

static CmdRegistry global_cmd_registry;

bool global_cmd_add(std::string name, CmdRegistry::cmd_t c) {
    return global_cmd_registry.cmd_add(name, c);
}

bool global_cmd_del(std::string name) {
    return global_cmd_registry.cmd_del(name);
}

bool global_cmd_exec(const std::string &c, const std::string &a) {
    return global_cmd_registry.exec(c, a);
}

bool CmdRegistry::cmd_add(const std::string &name, cmd_t c) {
    if (registry.find(name) != registry.end()) {
        LOG("Could not register command as it is already present. Name:"
            << name);
        return false;
    }

    registry.emplace(name, c);
    LOG("CMD-ADD: " << name);
    return true;
}

bool CmdRegistry::cmd_del(const std::string &name) {
    auto i = registry.find(name);
    if (i == registry.end()) {
        LOG("No such command: " << name);
        return false;
    }

    registry.erase(i);
    LOG("CMD-DEL: " << name);

    return true;
}

bool CmdRegistry::exec(const std::string &cmd, const std::string &args) {
    auto i = registry.find(cmd);
    if (i == registry.end()) {
        LOG("No such command: \"" << cmd << "\"");
        std::cout << "No such command: " << cmd << "\r\n";
        return false;
    }

    const char *b = args.c_str();
    const char *e = b + args.size();

    // call delegate
    return i->second(b, e);
}

};  // namespace TermHub

