#include "tcp_client.hxx"
#include "log.hxx"

namespace TermHub {

DutPtr TcpClient::create(boost::asio::io_service &asio, HubPtr h,
                         std::string host, std::string port) {
    std::shared_ptr<TcpClient> p(new TcpClient(asio, h, host, port));
    // LOG("create " << (void *)p.get());
    signal_exit_reg(std::bind(&TcpClient::shutdown, p));
    p->start();
    return p;
}

TcpClient::~TcpClient() { LOG("tcp-client destruct" << (void *)this); }

void TcpClient::shutdown() {
    LOG("Shutting down TcpClient");
    shutting_down_ = true;
    socket_.cancel();
    socket_.close();
}

void TcpClient::start() {
    LOG("start");
    auto x = std::bind(&TcpClient::handle_read, shared_from_this(),
                       std::placeholders::_1, std::placeholders::_2);
    boost::asio::async_read(socket_, boost::asio::buffer(&buf_[0], buf_.size()),
                            boost::asio::transfer_at_least(1), x);
}

void TcpClient::reconnect_timeout() {
    auto x = std::bind(&TcpClient::handle_reconnect_timeout, shared_from_this(),
                       std::placeholders::_1);
    timer_.expires_from_now(boost::posix_time::seconds(1));
    timer_.async_wait(x);
}

void TcpClient::handle_reconnect_timeout(const boost::system::error_code &e) {
    bool ok = false;
    try {
        connect();
        ok = true;
    } catch (...) {
        ok = false;
    }

    if (ok) {
        std::cout << "Reconected :-)\r\n";
        start();
        return;
    } else {
        reconnect_timeout();
        return;
    }
}

void TcpClient::connect() {
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
}

void TcpClient::handle_read(const boost::system::error_code &error,
                            size_t length) {
    LOG("handle_read" << (void *)this);
    if (shutting_down_)
        return;

    if (error) {
        std::cout << "\r\nError: " << error.message() << "\r\n";
        LOG("Error: " << error.message());

        socket_.cancel();
        socket_.close();

        bool ok = false;
        try {
            connect();
            ok = true;
        } catch (...) {
            ok = false;
        }

        if (ok) {
            stat_connected_inc();
            std::cout << "Reconected :-)\r\n";
            start();
            return;
        } else {
            std::cout << "Failed to reconnect (will continue to retry)\r\n";
            reconnect_timeout();
            return;
        }
    }

    std::string s(&buf_[0], length);
    hub_->post(shared_from_this(), &buf_[0], length);
    stat_rx_inc(length);
    start();
}

void TcpClient::write_start() {
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
    auto x = std::bind(&TcpClient::write_completion, shared_from_this(),
                       std::placeholders::_1, std::placeholders::_2);
    boost::asio::async_write(socket_, boost::asio::buffer(data, length),
                             boost::asio::transfer_at_least(1), x);
}

void TcpClient::write_completion(const boost::system::error_code &error,
                                 size_t length) {
    write_in_progress_ = false;
    if (shutting_down_)
        return;

    if (error) {
        std::cout << "Write error, clearning socket!\r\n";
        stat_tx_error();
        tx_buf_.clear();
        return;
    }

    stat_tx_complete(length);
    tx_buf_.consume(length);
    write_start();
}

void TcpClient::inject(const char *p, size_t l) {
    l -= tx_buf_.push(p, l);
    stat_tx_drop_inc(l);
    write_start();
}

void TcpClient::status_dump(std::stringstream &ss, const now_t &base_time) {
    ss << "dut-tcp-client(" << host_ << ":" << port_ << ") {\n";
    stat.pr(ss, base_time);
    ss << "}";
}

TcpClient::TcpClient(boost::asio::io_service &asio, HubPtr h, std::string host,
                     std::string port)
    : asio_(asio), timer_(asio), hub_(h), socket_(asio), host_(host),
      port_(port) {
    LOG("tcp-client construct " << (void *)this);
    connect();
    stat_connected_inc();
}

} // namespace TermHub