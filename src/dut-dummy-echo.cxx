#ifdef TERMHUB_FEATURE_DUMMY_DEVICE
#include "dut-dummy-echo.hxx"
#include "log.hxx"

#include <fcntl.h>

namespace TermHub {

int open_rd() { return ::open("/tmp/dummy", O_RDONLY | O_NONBLOCK); }
int open_wr() { return ::open("/tmp/dummy", O_WRONLY | O_NONBLOCK); }

DutDummyEcho::DutDummyEcho(boost::asio::io_service &asio, HubPtr h)
    : Dut(asio, h, "dummy"), read_delay_timer_(asio), write_delay_timer_(asio),
      rp(asio), wp(asio) {}

DutDummyEcho::~DutDummyEcho() {}

void DutDummyEcho::child_close() {
    rp.close();
    wp.close();
    read_delay_timer_.cancel();
}

std::string DutDummyEcho::child_connect() {
    LOG("dummy: Connect!");

    int r = open_rd();
    LOG("dummy: open-rd: " << r);
    rp.assign(r);

    int w = open_wr();
    LOG("dummy: open-wr: " << w);
    wp.assign(w);

    return "/tmp/dummy";
}

void DutDummyEcho::read_delay_handler(const boost::system::error_code &error,
                                      size_t length) {
    auto x = std::bind(&Dut::read_handler, this, error, length);
    read_delay_timer_.expires_from_now(boost::posix_time::milliseconds(10));
    read_delay_timer_.async_wait(x);
}

void DutDummyEcho::write_delay_handler(const boost::system::error_code &error,
                                       size_t length) {
    auto x = std::bind(&Dut::write_handler, this, error, length);
    read_delay_timer_.expires_from_now(boost::posix_time::milliseconds(1000));
    read_delay_timer_.async_wait(x);
}

void DutDummyEcho::child_async_read() {
    LOG("dummy: read!");
    auto x = std::bind(&DutDummyEcho::read_delay_handler, this,
                       std::placeholders::_1, std::placeholders::_2);
    boost::asio::async_read(
        rp, boost::asio::buffer(&read_buf_[0], read_buf_.size()),
        boost::asio::transfer_at_least(1), x);
}

void DutDummyEcho::child_async_write(size_t s, const char *c) {
    LOG("dummy: write!");
    auto x = std::bind(&DutDummyEcho::write_delay_handler, this,
                       std::placeholders::_1, std::placeholders::_2);
    boost::asio::async_write(wp, boost::asio::buffer(c, s),
                             boost::asio::transfer_at_least(1), x);
}

#if 0
DutDummyEcho::DutDummyEcho(boost::asio::io_service &asio, HubPtr h)
    : Dut(asio, h, "dummy"), async_write_timer_(asio), async_read_timer_(asio),
      async_read_parent_timer_(asio) {}

DutDummyEcho::~DutDummyEcho() {}

void DutDummyEcho::child_close() {
    async_write_timer_.cancel();
    async_read_timer_.cancel();
    async_read_parent_timer_.cancel();
};

std::string DutDummyEcho::child_connect() { return "dummy"; };

void DutDummyEcho::child_async_read_int() {
    internal_timer_running = false;
    child_async_read();
}

void DutDummyEcho::child_async_read() {
    if (shutting_down_) {
        return;
    }

    // LOG("Dummy async read - tick");

    if (!internal_timer_running) {
        async_read_timer_.expires_from_now(
            boost::posix_time::milliseconds(1000));
        async_read_timer_.async_wait(
            std::bind(&DutDummyEcho::child_async_read_int, this));
        internal_timer_running = true;
    }

    if (read_in_progress_) {
        return;
    }

    size_t l;
    const char *d = dummy_buf_.get_data_buf(&l);
    l = std::min(l, read_buf_.size());
    if (l > 0) {
        LOG("Dummy async read: " << l);
        dummy_buf_.consume(l);
        std::memcpy(&read_buf_[0], d, l);

        read_in_progress_ = true;
        async_read_parent_timer_.expires_from_now(
            boost::posix_time::milliseconds(100));
        async_read_parent_timer_.async_wait(
            std::bind(&Dut::read_handler, this, read_parent_ec, l));
    }
};

void DutDummyEcho::child_async_write(size_t s, const char *c) {
    LOG("Dummy async write" << s);

    size_t ss = dummy_buf_.push(c, s);

    async_write_timer_.expires_from_now(boost::posix_time::milliseconds(10));
    async_write_timer_.async_wait(
        std::bind(&Dut::write_handler, this, read_ec, ss));
}
#endif

void DutDummyEcho::status_dump(std::stringstream &ss, const now_t &base_time) {
    ss << "Dummy-dut here, no further status\n";
}

} // namespace TermHub
#endif
