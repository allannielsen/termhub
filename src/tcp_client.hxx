#ifndef __TERMHUB_TCP_CLIENT_HXX__
#define __TERMHUB_TCP_CLIENT_HXX__

#include <boost/asio.hpp>
#include <string>

#include "dut.hxx"
#include "hub.hxx"
#include "iobase.hxx"
#include "signal_exit.hxx"

namespace TermHub {
struct TcpClient : public Dut {
    TcpClient(boost::asio::io_service &asio, HubPtr h, std::string host,
              std::string port);

    ~TcpClient();

    void child_close() override;
    std::string child_connect() override;
    void child_async_read() override;
    void child_async_write(size_t length, const char *data) override;

    void status_dump(std::stringstream &ss, const now_t &base_time) override;

  private:
    boost::asio::io_service &asio_;
    boost::asio::ip::tcp::socket socket_;
    size_t write_in_progress_ = 0;
    std::string host_, port_;
};
} // namespace TermHub

#endif
