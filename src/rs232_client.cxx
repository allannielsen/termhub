#include "rs232_client.hxx"

namespace io = boost::asio;

namespace TermHub {
Rs232Client::Rs232Client(boost::asio::io_service &asio, HubPtr h,
                         std::string path, int baudrate)
    : hub_(h), serial_(asio), path_(path), baudrate_(baudrate), timer_(asio) {
    LOG("rs232 client construct " << (void *)this);
}

void Rs232Client::open_and_start() {
    serial_.open(path_);
    serial_.set_option(io::serial_port_base::baud_rate(baudrate_));
    serial_.set_option(
            io::serial_port_base::parity(io::serial_port_base::parity::none));
    serial_.set_option(io::serial_port_base::character_size(8));
    serial_.set_option(io::serial_port_base::flow_control(
                    io::serial_port_base::flow_control::none));
    serial_.set_option(io::serial_port_base::stop_bits(
                    io::serial_port_base::stop_bits::one));

    start();
}

void Rs232Client::shutdown() {
    LOG("Shutting down Rs232Client");
    shutting_down_ = true;
    serial_.close();
}

void Rs232Client::send_break() {
    LOG("break!");
    serial_.send_break();
}

void Rs232Client::start() {
    LOG("async_read_started");
    auto x = std::bind(&Rs232Client::handle_read, shared_from_this(),
                       std::placeholders::_1, std::placeholders::_2);
    boost::asio::async_read(serial_, boost::asio::buffer(&buf_[0], buf_.size()),
                            boost::asio::transfer_at_least(1), x);
    LOG("async_read_started-ended");
}

void Rs232Client::handle_read(const boost::system::error_code &error,
                              size_t length) {
    if (shutting_down_) return;

    if (error) {
        boost::system::error_code e;
        LOG("Error: " << error.message());
        reconnect_timeout();
        return;
    }

    std::string s(&buf_[0], length);
    LOG("handle_read " << (void *)this << " data: " << Fmt::EscapedString(s));
    hub_->post(shared_from_this(), s);
    LOG("handle_read " << (void *)this << " ended");
    start();
}

void Rs232Client::inject(const std::string &s) {
    LOG("rs232-client inject: " << Fmt::EscapedString(
                                           const_cast<std::string &>(s)));
    try {
        write(serial_, boost::asio::buffer(s));
    } catch (...) { // TODO, print error
        reconnect_timeout();
    }
    LOG("rs232-client inject ended");
}

void Rs232Client::reconnect_timeout() {
    if (shutting_down_) return;

    if (!sleeping_) {
        auto x = std::bind(&Rs232Client::handle_reconnect_timeout,
                           shared_from_this(), std::placeholders::_1);
        timer_.expires_from_now(boost::posix_time::milliseconds(100));
        timer_.async_wait(x);
        sleeping_ = true;
    }
}

void Rs232Client::handle_reconnect_timeout(const boost::system::error_code& e) {
    sleeping_ = false;

    try {
        if (serial_.is_open()) {
            serial_.close();
        }
    } catch (...) {
    }

    bool ok = false;
    try {
        open_and_start();
        ok = true;
    } catch (...) { // TODO, print error
        ok = false;
    }

    if (ok) {
        std::string s;
        s.append("<Connected to ");
        s.append(path_);
        s.append(">\r\n");
        hub_->post(shared_from_this(), s);
        return;
    } else {
        std::string s;
        s.append("<no dut at ");
        s.append(path_);
        s.append(" - retrying>\r\n");
        hub_->post(shared_from_this(), s);
        reconnect_timeout();
        return;
    }
}


}  // namespace TermHub
