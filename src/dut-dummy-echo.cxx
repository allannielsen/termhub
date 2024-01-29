#include "dut-dummy-echo.hxx"
#include "log.hxx"

namespace TermHub {

DutDummyEcho::DutDummyEcho(boost::asio::io_service &asio, HubPtr h)
    : Dut(asio, h, "dummy"), async_write_timer_(asio), async_read_timer_(asio) {
}

DutDummyEcho::~DutDummyEcho() {}

void DutDummyEcho::child_close() {
    async_write_timer_.cancel();
    async_write_timer_.cancel();
};

std::string DutDummyEcho::child_connect() { return "dummy"; };
void DutDummyEcho::child_async_read(){};

void DutDummyEcho::child_async_write(size_t s, const char *c) {
    size_t i = 0, ii = 0;
    LOG("child async write\\r");

    while (i < s) {
        if (ii == read_buf_.size()) {
            LOG("RX Buffer exceeded");
            break;
        }

        if (*c == '\r') {
            LOG("Detected \\r");
            LOG("puts: " << ii);
            read_buf_[ii++] = '\r';
            if (ii == read_buf_.size()) {
                LOG("no room for newline");
                break;
            }
            LOG("puts: " << ii);
            read_buf_[ii++] = '\n';
        } else {
            LOG("puts: " << ii);
            read_buf_[ii++] = *c++;
        }

        i++;
    }

    timer_.expires_from_now(boost::posix_time::milliseconds(1));
    timer_.async_wait(std::bind(&Dut::write_handler, this, write_ec, s));

    timer_.expires_from_now(boost::posix_time::milliseconds(1));
    timer_.async_wait(std::bind(&Dut::read_handler, this, read_ec, ii));

    LOG("child async write - done\\r");
}

void DutDummyEcho::status_dump(std::stringstream &ss, const now_t &base_time) {
    ss << "Dummy-dut here, no further status\n";
}

} // namespace TermHub
