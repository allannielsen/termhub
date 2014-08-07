#include <unistd.h>
#include <termios.h>
#include <functional>

#include "tty.hxx"
#include "log.hxx"
#include "process.hxx"
#include "signal_exit.hxx"
#include "cmd-registry.hxx"

namespace TermHub {

Tty::Tty(boost::asio::io_service &asio, HubPtr h, IoPtr d)
        : dut_(d),
          hub_(h),
          asio_(asio),
          input_(asio_, ::dup(STDIN_FILENO)),
          output_(asio_, ::dup(STDOUT_FILENO)),
          key_tokenizer(
              std::bind(&Tty::handle_read_token, this, std::placeholders::_1),
              std::bind(&Tty::handle_read_data, this, std::placeholders::_1,
                        std::placeholders::_2)) {
    tcgetattr(STDIN_FILENO, &old_tio);
    new_tio = old_tio;
    cfmakeraw(&new_tio);
    int res = tcsetattr(STDIN_FILENO, TCSANOW, &new_tio);
    assert(res == 0);
}

Tty::~Tty() { tcsetattr(STDIN_FILENO, TCSANOW, &old_tio); }

bool Tty::tty_cmd_add(const char *&b, const char *e) {
    LOG("TTY-CMD-ADD: " << std::string(b, e));

    Fmt::Literal quit("quit", true, true);
    Fmt::Literal inject_lit("inject", true, true);
    Fmt::Literal spawn_lit("spawn", true, true);

    std::string key;
    std::string inject;

    Fmt::EscapedString key_(key);
    Fmt::EscapedString inject_(inject);

    if (parse_group(b, e, key_, quit) && b == e) {
        LOG("TTY-CMD: " << key_ << "(" << actions.size() << ") -> QUIT");

        key_tokenizer.key_add(key, actions.size());
        actions.emplace_back(std::make_shared<ActionQuit>());
        return true;

    } else if (parse_group(b, e, key_, inject_lit, inject_)) {
        LOG("TTY-CMD: " << key_ << " -> INJECT(" << inject_ << ")");

        key_tokenizer.key_add(key, actions.size());
        actions.emplace_back(std::make_shared<ActionInject>(inject));
        return true;

    } else if (parse_group(b, e, key_, spawn_lit, inject_)) {
        LOG("TTY-CMD: " << key_ << " -> SPAWN(" << inject_ << ")");

        key_tokenizer.key_add(key, actions.size());
        actions.emplace_back(
            std::make_shared<ActionSpawn>(asio_, hub_, inject));
        return true;
    } else {
        LOG("Did not understand command");
    }

    return false;
}

void Tty::ActionQuit::exec(TtyPtr tty, IoPtr dut) {
    LOG("Call signal_exit");
    signal_exit();
}

void Tty::ActionInject::exec(TtyPtr tty, IoPtr dut) {
    LOG("Injecting data: " << Fmt::EscapedString(data));
    dut->inject(data);
}

void Tty::action_spawn_completed(int pid, int flags, int status) {
    if (flags)
        std::cout << "\r\n<Pid: " << pid << " was killed"
                  << ">\r\n";
    else
        std::cout << "\r\n<Pid: " << pid << " exited: " << status << ">\r\n";

    input_enable();
}

void Tty::ActionSpawn::exec(TtyPtr tty, IoPtr dut) {
    tty->input_disable();
    auto x = std::bind(&Tty::action_spawn_completed, tty, std::placeholders::_1,
                       std::placeholders::_2, std::placeholders::_3);
    auto p = Process::create(asio_, hub_, dut, app_, x);

    std::cout << "\r\n<Spawn: " << app_ << " pid: " << p->get_id() << ">\r\n";
}

bool Tty::tty_cmd_del(const char *&b, const char *e) { return false; }

void Tty::inject(const std::string &s) {
    write(output_, boost::asio::buffer(s));
}

void Tty::init() {
    global_cmd_add("tty-cmd-add",
                   std::bind(&Tty::tty_cmd_add, shared_from_this(),
                             std::placeholders::_1, std::placeholders::_2));

    global_cmd_add("tty-cmd-del",
                   std::bind(&Tty::tty_cmd_del, shared_from_this(),
                             std::placeholders::_1, std::placeholders::_2));
}

void Tty::start() {
    auto x = std::bind(&Tty::handle_read, shared_from_this(),
                       std::placeholders::_1, std::placeholders::_2);
    boost::asio::async_read(input_, boost::asio::buffer(&buf_[0], buf_.size()),
                            boost::asio::transfer_at_least(1), x);
}

void Tty::handle_read_data(const char *c, size_t s) {
    std::string str(c, s);
    LOG("tty read ptr:" << (void *)dut_.get()
                        << " data:" << Fmt::EscapedString(str));
    dut_->inject(str);
}

void Tty::handle_read_token(uint32_t t) {
    LOG("Got token: " << t);

    if (t > actions.size()) {
        LOG("No action found for token: " << t);
        return;
    }

    actions[t]->exec(shared_from_this(), dut_);
}

void Tty::shutdown() {
    std::cout << "\r\n";
    LOG("Shutting down TTY");
    global_cmd_del("tty-cmd-add");
    global_cmd_del("tty-cmd-del");
    shutting_down_ = true;
    input_.close();
}

void Tty::handle_read(const boost::system::error_code &error, size_t length) {
    if (shutting_down_) return;

    if (error) {
        LOG("Error: " << error.message());
        return;
    }

    if (input_disable_) {
        std::cout << "\r\n<input is suspended!>\r\n";
    } else {
        // TODO, start timer
        key_tokenizer.put(&buf_[0], length);
    }

    start();
}
}  // namespace TermHub

