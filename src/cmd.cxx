#include "cmd.hxx"
#include "dut.hxx"
#include <chrono>

namespace TermHub {

void UnixSocketServer::status_msg(std::stringstream &ss) {
    now_t now = std::chrono::system_clock::now();
    ss << "Termhub is alive and kicking!!!\n\n";

    ss << "Dut status\n";
    ss << "==========\n";
    dut_->status_dump(ss, now);
    ss << "(Dut end)\n\n";

    ss << "TCP Server\n";
    ss << "==========\n";
    if (tcp_) {
        tcp_->status_dump(ss, now);
    } else {
        ss << "No TCP Server enabled\n";
    }
    ss << "(tcp server end)\n\n";

    ss << "Hub status\n";
    ss << "==========\n";
    hub_->status_dump(ss, now);
    ss << "(Hub end)\n\n";

    ss << "End of status dump!\n";
}

void UnixSocketServer::do_accept() {
    acceptor_.async_accept(
        [this](boost::system::error_code ec,
               boost::asio::local::stream_protocol::socket socket) {
            if (!ec) {
                std::stringstream ss;
                status_msg(ss);
                boost::asio::write(socket, boost::asio::buffer(ss.str()));
                socket.close();
            }

            do_accept();
        });
}

} // namespace TermHub
