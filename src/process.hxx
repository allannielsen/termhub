#ifndef __TERMHUB_PROCESS_HXX__
#define __TERMHUB_PROCESS_HXX__

#include <boost/asio.hpp>
#include <boost/process.hpp>
#include <functional>

#include "iobase.hxx"
#include "log.hxx"
#include "ringbuf.hxx"
#include "signal_exit.hxx"

namespace TermHub {

class Process : public Iobase, public std::enable_shared_from_this<Process> {
  public:
    typedef std::function<void(int, int, int)> cb_t;
    static std::shared_ptr<Process> create(boost::asio::io_service &asio,
                                           HubPtr h, IoPtr d, std::string s,
                                           cb_t cb);

    ~Process();

    void inject(const char *p, size_t l);
    void start();
    void shutdown();
    void kill();
    int get_id() { return child.get_id(); }

    void status_dump(std::stringstream &ss, const now_t &base_time);

  private:
    Process(boost::asio::io_service &asio, HubPtr h, IoPtr d,
            boost::process::context cxt, std::string app, cb_t cb);

    void read_out();
    void read_err();
    void clean_up();

    void handle_read_out(const boost::system::error_code &error,
                         std::size_t length);
    void handle_read_err(const boost::system::error_code &error,
                         std::size_t length);

    void write_start();
    void write_completion(const boost::system::error_code &error,
                          size_t length);

    IoPtr dut_;
    HubPtr hub_;
    bool dead_ = false;
    bool exiting_ = false;
    bool error_err = false;
    bool error_out = false;
    size_t write_in_progress_ = 0;

    std::array<char, 4096> buf_out;
    std::array<char, 4096> buf_err;
    RingBuf<4096> tx_buf_;
    boost::process::child child;
    boost::asio::posix::stream_descriptor in;
    boost::asio::posix::stream_descriptor out;
    boost::asio::posix::stream_descriptor err;

    cb_t call_back;
};

} // namespace TermHub

#endif
