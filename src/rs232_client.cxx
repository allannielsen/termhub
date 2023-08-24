#include "rs232_client.hxx"

namespace io = boost::asio;

namespace TermHub {
Rs232Client::Rs232Client(boost::asio::io_service &asio, HubPtr h,
                         std::string path, int baudrate)
    : hub_(h), serial_(asio), path_(path), baudrate_(baudrate), timer_(asio) {
    LOG("rs232(" << (void *)this << "): construct");
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
    LOG("rs232(" << (void *)this << "): shutdown");
    shutting_down_ = true;
    serial_.close();
}

void Rs232Client::send_break() {
    LOG("rs232(" << (void *)this << "): break");
    try {
        serial_.send_break();
    } catch(...) {
        LOG("rs232(" << (void *)this << "): break-unknown error...");
    }
}

void Rs232Client::start() {
    LOG("rs232(" << (void *)this << "): async_read_started");
    auto x = std::bind(&Rs232Client::handle_read, shared_from_this(),
                       std::placeholders::_1, std::placeholders::_2);
    boost::asio::async_read(serial_, boost::asio::buffer(&buf_[0], buf_.size()),
                            boost::asio::transfer_at_least(1), x);
    LOG("rs232(" << (void *)this << "): async_read_started-ended");
}

void Rs232Client::handle_read(const boost::system::error_code &error,
                              size_t length) {
    if (shutting_down_) return;

    if (error) {
        boost::system::error_code e;
        LOG("rs232(" << (void *)this << "): Read-Error: " << error.message());
        reconnect_timeout();
        return;
    }

    std::string s(&buf_[0], length);
    LOG("rs232(" << (void *)this << "): handle_read data: " << Fmt::EscapedString(s));
    hub_->post(shared_from_this(), &buf_[0], length);
    LOG("rs232(" << (void *)this << "): handle_read ended");
    start();
}

void Rs232Client::write_start() {
    if (write_in_progress_ || shutting_down_) {
        return;
    }

    size_t length = 0;
    const char *data = tx_buf_.get_data_buf(&length);
    if (length == 0) {
        return;
    }

    write_in_progress_ = true;
    auto x = std::bind(&Rs232Client::write_completion, shared_from_this(),
                       std::placeholders::_1, std::placeholders::_2);
    boost::asio::async_write(serial_, boost::asio::buffer(data, length),
                             boost::asio::transfer_at_least(1), x);
}

void Rs232Client::write_completion(const boost::system::error_code &error,
                              size_t length) {
    write_in_progress_ = false;
    if (shutting_down_) return;

    if (error) {
        boost::system::error_code e;
        LOG("rs232(" << (void *)this << "): Write-Error: " << error.message());
        tx_buf_.clear();
        return;
    }

    LOG("rs232(" << (void *)this << "): write_completion data: " << length);
    tx_buf_.consume(length);
    write_start();
}

void Rs232Client::inject(const char *p, size_t l) {
    tx_buf_.push(p, l);
    write_start();
}

void Rs232Client::reconnect_timeout() {
    if (shutting_down_) return;

    if (!sleeping_) {
        LOG("rs232(" << (void *)this << "): async_sleep");
        auto x = std::bind(&Rs232Client::handle_reconnect_timeout,
                           shared_from_this(), std::placeholders::_1);
        timer_.expires_from_now(boost::posix_time::milliseconds(100));
        timer_.async_wait(x);
        sleeping_ = true;
        LOG("rs232(" << (void *)this << "): async_sleep - exit");
    } else {
        LOG("rs232(" << (void *)this << "): already sleeping...");
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
        hub_->post(shared_from_this(), s.c_str(), s.size());
        return;
    } else {
        std::string s;
        s.append("<no dut at ");
        s.append(path_);
        s.append(" - retrying>\r\n");
        hub_->post(shared_from_this(), s.c_str(), s.size());
        reconnect_timeout();
        return;
    }
}


}  // namespace TermHub
