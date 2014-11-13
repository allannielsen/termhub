#ifndef __TERMHUB_PROCESS_HXX__
#define __TERMHUB_PROCESS_HXX__

#include <functional>
#include <boost/asio.hpp>
#include <boost/process.hpp>

#include "log.hxx"
#include "iobase.hxx"
#include "signal_exit.hxx"

namespace TermHub {

class Process : public Iobase, public std::enable_shared_from_this<Process> {
  public:
    typedef std::function<void(int, int, int)> cb_t;
    static std::shared_ptr<Process> create(boost::asio::io_service& asio,
                                           HubPtr h, IoPtr d, std::string s,
                                           cb_t cb);

    ~Process();

    void inject(const std::string& s);
    void start();
    void shutdown();
    void kill();
    int get_id() { return child.get_id(); }

  private:
    Process(boost::asio::io_service& asio, HubPtr h, IoPtr d,
            boost::process::context cxt, std::string app, cb_t cb);

    void read_out();
    void read_err();
    void clean_up();

    void handle_read_out(const boost::system::error_code& error,
                         std::size_t length);
    void handle_read_err(const boost::system::error_code& error,
                         std::size_t length);


    IoPtr dut_;
    HubPtr hub_;
    bool dead_ = false;
    bool exiting_ = false;
    bool error_err = false;
    bool error_out = false;

    std::array<char, 32> buf_out;
    std::array<char, 32> buf_err;
    boost::process::child child;
    boost::asio::posix::stream_descriptor in;
    boost::asio::posix::stream_descriptor out;
    boost::asio::posix::stream_descriptor err;

    cb_t call_back;
};

}  // namespace TermHub

#endif

