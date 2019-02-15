#include "process.hxx"
#include "log.hxx"
#include "fmt.hxx"

namespace TermHub {

extern int listen_port_number;
std::shared_ptr<Process> Process::create(boost::asio::io_service& asio,
                                         HubPtr h, IoPtr d, std::string s,
                                         cb_t cb) {
    boost::process::context ctx;
    ctx.stdin_behavior = boost::process::capture_stream();
    ctx.stdout_behavior = boost::process::capture_stream();
    ctx.stderr_behavior = boost::process::capture_stream();
    if (listen_port_number != -1)
        ctx.environment.emplace("TERMHUB_PORT",
                                std::to_string(listen_port_number));
    auto p = std::shared_ptr<Process>(new Process(asio, h, d, ctx, s, cb));

    h->connect(p);
    p->start();

    return p;
}

Process::Process(boost::asio::io_service& asio, HubPtr h, IoPtr d,
                 boost::process::context ctx, std::string app, cb_t cb)
    : dut_(d),
      hub_(h),
      child(boost::process::launch_shell(app, ctx)),
      in(asio),
      out(asio),
      err(asio),
      call_back(cb) {

    boost::process::postream& os = child.get_stdin();
    in.assign(os.handle().release());

    boost::process::pistream& is = child.get_stdout();
    out.assign(is.handle().release());

    boost::process::pistream& es = child.get_stderr();
    err.assign(es.handle().release());
}

Process::~Process() { LOG("Destructing process"); }

void Process::inject(const std::string& s) {
    LOG("Process inject: " << Fmt::EscapedString(const_cast<std::string &>(s)));
    try {
        write(in, boost::asio::buffer(s));
    } catch(...) {}
    LOG("Process inject - ended");
}

void Process::start() {
    read_out();
    read_err();
}

void Process::read_out() {
    auto x = std::bind(&Process::handle_read_out, shared_from_this(),
                       std::placeholders::_1, std::placeholders::_2);

    boost::asio::async_read(out,
                            boost::asio::buffer(&buf_out[0], buf_out.size()),
                            boost::asio::transfer_at_least(1), x);
}

void Process::read_err() {
    auto x = std::bind(&Process::handle_read_err, shared_from_this(),
                       std::placeholders::_1, std::placeholders::_2);

    boost::asio::async_read(err,
                            boost::asio::buffer(&buf_err[0], buf_err.size()),
                            boost::asio::transfer_at_least(1), x);
}

void Process::shutdown() {
    exiting_ = true;
    if (dead_) return;

    child.terminate(true);
    clean_up();
}

void Process::kill() {
    child.terminate(true);
    clean_up();
}

void Process::handle_read_out(const boost::system::error_code& error,
                              std::size_t length) {
    if (exiting_) return;

    if (error) {
        LOG("Error: " << error.message());
        error_out = true;
        if (error_err && error_out) clean_up();
        return;
    }

    std::string s(buf_out.begin(), length);
    LOG("Process read: " << Fmt::EscapedString(s));

    // Hack... But we need a way to send a break signal - now it is <F12>
    if (s == std::string("\x1b[24~"))
        dut_->send_break();
    else if (s.size() < 2 && s[0] == 0xff && s[1] == 0xf3) // telnet break
        dut_->send_break();
    else
        dut_->inject(s);

    read_out();
    LOG("Process read ended");
}

void Process::handle_read_err(const boost::system::error_code& error,
                              std::size_t length) {
    if (exiting_) {
        return;
    }

    if (error) {
        LOG("Error: " << error.message());
        error_err = true;
        if (error_err && error_out) clean_up();
        return;
    }

    if (buf_out.size() < 1) {
        LOG("Got empty buffer");
        read_err();
        return;
    }

    std::cout << "PID-" << child.get_id() << " ";
    std::cout.write(buf_err.begin(), length);
    std::cout << "\r\n";
    std::cout.flush();
    read_err();
}

void Process::clean_up() {
    if (dead_) return;

    dead_ = true;
    auto id = child.wait();

    if (id.exited())
        call_back(child.get_id(), 0, id.exit_status());
    else
        call_back(child.get_id(), 1, 0);

    hub_->disconnect(shared_from_this());
}
}

