#include "dut-dummy-echo.hxx"

namespace TermHub {

DutDummyEcho::DutDummyEcho(boost::asio::io_service &asio, HubPtr h)
    : Dut(asio, h, "dummy"), async_write_timer_(asio), async_read_timer_(asio) {
}

DutDummyEcho::~DutDummyEcho() {}

void DutDummyEcho::child_close() {
    async_write_timer_.cancel();
    async_write_timer_.cancel();
};

void DutDummyEcho::child_connect(){};
void DutDummyEcho::child_async_read(){};

void DutDummyEcho::child_async_write(size_t s, const char *c) {
    size_t i;
    size_t max = std::min(read_buf_.size(), s);

    for (i = 0; i < max; ++i, ++c) {
        read_buf_[i] = *c;
    }

    timer_.expires_from_now(boost::posix_time::milliseconds(1));
    timer_.async_wait(std::bind(&Dut::write_handler, this, write_ec, i));

    timer_.expires_from_now(boost::posix_time::milliseconds(1));
    timer_.async_wait(std::bind(&Dut::read_handler, this, read_ec, i));
}

void DutDummyEcho::status_dump(std::stringstream &ss, const now_t &base_time) {
    ss << "Dummy-dut here, no further status\n";
}

} // namespace TermHub
