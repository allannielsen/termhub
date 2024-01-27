#ifndef __TERMHUB_TCP_CLIENT_HXX__
#define __TERMHUB_TCP_CLIENT_HXX__

#include <boost/asio.hpp>
#include <iostream>
#include <memory>
#include <string>

#include "dut.hxx"
#include "hub.hxx"
#include "iobase.hxx"
#include "ringbuf.hxx"
#include "signal_exit.hxx"

namespace TermHub {
struct TcpClient : public Dut, std::enable_shared_from_this<TcpClient> {
    static DutPtr create(boost::asio::io_service &asio, HubPtr h,
                         std::string host, std::string port);

    ~TcpClient();

    void shutdown();

    void start();

    void reconnect_timeout();

    void handle_reconnect_timeout(const boost::system::error_code &e);

    void connect();

    void handle_read(const boost::system::error_code &error, size_t length);

    void write_start();

    void write_completion(const boost::system::error_code &error,
                          size_t length);

    void inject(const char *p, size_t l);

    void status_dump(std::stringstream &ss, const now_t &base_time);

  private:
    TcpClient(boost::asio::io_service &asio, HubPtr h, std::string host,
              std::string port);

    boost::asio::io_service &asio_;
    boost::asio::deadline_timer timer_;
    HubPtr hub_;
    std::array<char, 32> buf_;
    RingBuf<1024 * 1024 * 4> tx_buf_;
    boost::asio::ip::tcp::socket socket_;
    bool shutting_down_ = false;
    size_t write_in_progress_ = 0;
    std::string host_, port_;
};
} // namespace TermHub

#endif
