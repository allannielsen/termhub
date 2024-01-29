#include "dut.hxx"
#include "log.hxx"

namespace TermHub {

Dut::Dut(boost::asio::io_service &asio, HubPtr h, const std::string &type)
    : hub_(h), timer_(asio), child_type_(type) {}

void Dut::stat_tx_complete(uint64_t x) {
    stat.tx.inc(x);
    stat.tx_pending = 0;
}

void Dut::reconnect_timeout_start() {
    if (shutting_down_)
        return;

    if (!sleeping_) {
        LOG("rs232(" << (void *)this << "): async_sleep");
        auto x = std::bind(&Dut::reconnect_timeout_handler, this);
        timer_.expires_from_now(boost::posix_time::milliseconds(100));
        timer_.async_wait(x);
        sleeping_ = true;
    }
}

void Dut::start() { reconnect_timeout_handler(); }

void Dut::reconnect_timeout_handler() {
    std::string ep;
    sleeping_ = false;

    try {
        child_close();
    } catch (...) {
    }

    bool ok = false;
    try {
        ep = child_connect();
        ok = true;
    } catch (...) { // TODO, print error
        ok = false;
    }

    if (ok) {
        stat_connected_inc();
        std::string s;
        s.append("<Connected to ");
        s.append(ep);
        s.append(">\r\n");
        hub_->post(this, s.c_str(), s.size());
        child_async_read();
        return;

    } else {
        std::string s;
        s.append("<Could not connect to ");
        s.append(ep);
        s.append(" - retrying>\r\n");
        hub_->post(this, s.c_str(), s.size());
        reconnect_timeout_start();
        return;
    }
}

void Dut::shutdown() {
    LOG("DUT/" << child_type_ << " Shutting down");
    shutting_down_ = true;
    child_close();
}

void Dut::read_handler(const boost::system::error_code &error, size_t length) {
    read_in_progress_ = false;
    if (shutting_down_)
        return;

    if (error) {
        LOG(child_type_ << ": Read-Error: " << error.message());
        reconnect_timeout_handler();
        return;
    }

    std::string s(&read_buf_[0], length);
    // LOG("rs232(" << (void *)this
    //              << "): handle_read data: " << Fmt::EscapedString(s));
    LOG(child_type_ << ": read-complete: " << length << " "
                    << Fmt::EscapedString(s));
    stat_rx_inc(length);
    hub_->post(this, &read_buf_[0], length);
    child_async_read();
    // LOG("rs232(" << (void *)this << "): handle_read ended");
}

void Dut::write_start() {
    if (write_in_progress_ || shutting_down_) {
        return;
    }

    size_t length = 0;
    const char *data = write_buf_.get_data_buf(&length);
    if (length == 0) {
        return;
    }

    std::string s(data, length);
    LOG(child_type_ << "(" << (void *)this << "): write-start: " << length
                    << " " << Fmt::EscapedString(s));
    write_in_progress_ = true;
    stat_tx_request(length);
    child_async_write(length, data);
}

void Dut::write_handler(const boost::system::error_code &error, size_t length) {
    write_in_progress_ = false;
    if (shutting_down_)
        return;

    if (error) {
        LOG(child_type_ << ": write-error: " << error.message());
        stat_tx_complete(0);
        stat_tx_error();
        write_buf_.clear();
        return;
    }

    LOG(child_type_ << ": write-complete: " << length);
    stat_tx_complete(length);
    write_buf_.consume(length);

    if (write_buf_.size() <= write_buf_.cap() / 2) {
        LOG(child_type_ << ": write-complete: Wake up read");
        hub_->wake_up_read();
    }

    write_start();
}

void Dut::inject(const char *p, size_t l) {
    l -= write_buf_.push(p, l);
    stat_tx_drop_inc(l);
    write_start();
}

boost::asio::mutable_buffer Dut::inject_buffer() {
    auto b = write_buf_.push_buffer();
    LOG(child_type_ << ": inject buffer size: " << b.size());
    return b;
}

void Dut::inject_commit(size_t s) {
    write_buf_.push_commit(s);
    write_start();
}

} // namespace TermHub
