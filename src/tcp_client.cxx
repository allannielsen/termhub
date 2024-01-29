#include "tcp_client.hxx"
#include "log.hxx"

namespace TermHub {

TcpClient::TcpClient(boost::asio::io_service &asio, HubPtr h, std::string host,
                     std::string port)
    : Dut(asio, h, "TCP"), asio_(asio), socket_(asio), host_(host),
      port_(port) {}

TcpClient::~TcpClient() { LOG("tcp-client destruct" << (void *)this); }

void TcpClient::child_close() {
    socket_.cancel();
    socket_.close();
}

void TcpClient::child_async_read() {
    auto x = std::bind(&Dut::read_handler, this, std::placeholders::_1,
                       std::placeholders::_2);
    boost::asio::async_read(
        socket_, boost::asio::buffer(&read_buf_[0], read_buf_.size()),
        boost::asio::transfer_at_least(1), x);
}

std::string TcpClient::child_connect() {
    std::string ep;
    using boost::asio::ip::tcp;

    tcp::resolver resolver(asio_);
    boost::asio::connect(socket_, resolver.resolve({host_, port_}));

    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;

    int fd = socket_.native_handle();
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

    boost::asio::socket_base::keep_alive option(true);
    socket_.set_option(option);

    ep.append(host_);
    ep.append(":");
    ep.append(port_);
    return ep;
}

void TcpClient::child_async_write(size_t length, const char *data) {
    auto x = std::bind(&Dut::write_handler, this, std::placeholders::_1,
                       std::placeholders::_2);
    boost::asio::async_write(socket_, boost::asio::buffer(data, length),
                             boost::asio::transfer_at_least(1), x);
}

void TcpClient::status_dump(std::stringstream &ss, const now_t &base_time) {
    ss << "dut-tcp-client(" << host_ << ":" << port_ << ") {\n";
    stat.pr(ss, base_time);
    ss << "}";
}

} // namespace TermHub
