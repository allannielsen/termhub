#ifndef __TERMHUB_CMD_HXX__
#define __TERMHUB_CMD_HXX__

#include "dut.hxx"
#include "hub.hxx"
#include "tcp_server.hxx"
#include <boost/asio.hpp>
#include <unistd.h>

namespace TermHub {

class UnixSocketServer : public std::enable_shared_from_this<UnixSocketServer> {
  public:
    static std::shared_ptr<UnixSocketServer>
    create(boost::asio::io_service &asio, const std::string &file, HubPtr hub,
           DutPtr dut, TcpServerPtr tcp) {
        ::unlink(file.c_str());
        return std::shared_ptr<UnixSocketServer>(
            new UnixSocketServer(asio, file, hub, dut, tcp));
    }

    ~UnixSocketServer() {}

  private:
    UnixSocketServer(boost::asio::io_context &io_context,
                     const std::string &file_name, HubPtr hub, DutPtr dut,
                     TcpServerPtr tcp)
        : hub_(hub), dut_(dut), tcp_(tcp),
          acceptor_(io_context,
                    boost::asio::local::stream_protocol::endpoint(file_name)) {
        do_accept();
    }

    void do_accept();

    void status_msg(std::stringstream &ss);

    HubPtr hub_;
    DutPtr dut_;
    TcpServerPtr tcp_;
    boost::asio::local::stream_protocol::acceptor acceptor_;
};

} // namespace TermHub

#endif
