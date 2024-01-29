#include <functional>
#include <termios.h>
#include <unistd.h>

#include "cmd-registry.hxx"
#include "log.hxx"
#include "process.hxx"
#include "signal_exit.hxx"
#include "tty.hxx"

namespace TermHub {

Tty::Tty(boost::asio::io_service &asio, HubPtr h, DutPtr d)
    : dut_(d), hub_(h), asio_(asio), input_(asio_, ::dup(STDIN_FILENO)),
      output_(asio_, ::dup(STDOUT_FILENO)),
      key_tokenizer(
          std::bind(&Tty::handle_read_token, this, std::placeholders::_1),
          std::bind(&Tty::handle_read_data, this, std::placeholders::_1,
                    std::placeholders::_2)),
      process_(nullptr) {
    tcgetattr(STDIN_FILENO, &old_tio);
    new_tio = old_tio;
    cfmakeraw(&new_tio);
    tcsetattr(STDIN_FILENO, TCSANOW, &new_tio);
}

Tty::~Tty() {}

bool Tty::tty_cmd_add(const char *&b, const char *e) {
    LOG("TTY-CMD-ADD: " << std::string(b, e));

    Fmt::Literal quit("quit", true, true);
    Fmt::Literal send_break("break", true, true);
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

    } else if (parse_group(b, e, key_, send_break) && b == e) {
        LOG("TTY-CMD: " << key_ << "(" << actions.size() << ") -> BREAK");

        key_tokenizer.key_add(key, actions.size());
        actions.emplace_back(std::make_shared<ActionBreak>());
        return true;

    } else if (parse_group(b, e, key_, inject_lit, inject_)) {
        LOG("TTY-CMD: " << key_ << " -> INJECT(" << inject_ << ")");

        key_tokenizer.key_add(key, actions.size());
        actions.emplace_back(std::make_shared<ActionInject>(inject));
        return true;

    } else if (parse_group(b, e, key_, spawn_lit, inject_)) {
        LOG("TTY-CMD: " << key_ << " -> SPAWN(" << inject_ << ")");

        key_tokenizer.key_add(key, actions.size());
        actions.emplace_back(std::make_shared<ActionSpawn>(asio_, hub_, inject,
                                                           shared_from_this()));
        return true;
    } else {
        LOG("Did not understand command");
    }

    return false;
}

void Tty::ActionQuit::exec(TtyPtr tty, DutPtr dut) {
    LOG("Call signal_exit");
    signal_exit();
}

void Tty::ActionBreak::exec(TtyPtr tty, DutPtr dut) {
    LOG("Call break");
    dut->send_break();
}

void Tty::ActionInject::exec(TtyPtr tty, DutPtr dut) {
    LOG("Injecting data: " << Fmt::EscapedString(data));
    dut->inject(data.c_str(), data.size());
}

void Tty::action_spawn_completed(int pid, int flags, int status) {
    if (flags)
        std::cout << "\r\n<Pid: " << pid << " was killed"
                  << ">\r\n";
    else
        std::cout << "\r\n<Pid: " << pid << " exited: " << status << ">\r\n";

    process_.reset();
    input_enable();
}

void Tty::ActionSpawn::exec(TtyPtr tty, DutPtr dut) {
    tty->input_disable();
    auto x = std::bind(&Tty::action_spawn_completed, tty, std::placeholders::_1,
                       std::placeholders::_2, std::placeholders::_3);
    tty->process_ = Process::create(asio_, hub_, dut, app_, x);
    std::cout << "\r\n<Spawn: " << app_ << " pid: " << tty->process_->get_id()
              << ">\r\n";
}

bool Tty::tty_cmd_del(const char *&b, const char *e) { return false; }

void Tty::write_start() {
    if (write_in_progress_ || shutting_down_) {
        return;
    }

    size_t length = 0;
    const char *data = tx_buf_.get_data_buf(&length);
    if (length == 0) {
        return;
    }

    write_in_progress_ = true;
    stat_tx_request(length);
    auto x = std::bind(&Tty::write_completion, shared_from_this(),
                       std::placeholders::_1, std::placeholders::_2);
    boost::asio::async_write(output_, boost::asio::buffer(data, length),
                             boost::asio::transfer_at_least(1), x);
}

void Tty::write_completion(const boost::system::error_code &error,
                           size_t length) {
    write_in_progress_ = false;
    if (shutting_down_)
        return;

    if (error) {
        boost::system::error_code e;
        std::cout << "Write error, clearning socket!\r\n";
        stat_tx_complete(0);
        tx_buf_.clear();
        return;
    }

    stat_tx_complete(length);
    tx_buf_.consume(length);
    write_start();
}

void Tty::inject(const char *p, size_t l) {
    l -= tx_buf_.push(p, l);
    stat_tx_drop_inc(l);
    write_start();
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
    LOG("tty read ptr:" << (void *)dut_ << " data:" << Fmt::EscapedString(str));
    stat_rx_inc(s);
    dut_->inject(c, s);
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
    tcsetattr(STDIN_FILENO, TCSANOW, &old_tio);
}

void Tty::handle_read(const boost::system::error_code &error, size_t length) {
    if (shutting_down_)
        return;

    if (error) {
        LOG("Error: " << error.message());
        return;
    }

    if (input_disable_) {
        for (size_t i = 0; i < length; ++i) {
            if (buf_[i] == 3 && process_) {
                process_->kill();
            } else {
                if (process_)
                    std::cout << "\r\n<Input is suspended!>\r\n";
                else
                    std::cout << "\r\n<Input is suspended while extern process "
                                 "is "
                                 "running! Press ctrl-c to kill it>\r\n";
            }
        }
    } else {
        // TODO, start timer
        key_tokenizer.put(&buf_[0], length);
    }

    start();
}

void Tty::status_dump(std::stringstream &ss, const now_t &base_time) {
    ss << "tty() {\n";
    stat.pr(ss, base_time);
    ss << "}\n\n";
}
} // namespace TermHub
