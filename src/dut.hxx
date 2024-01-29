#ifndef __TERMHUB_DUT_HXX__
#define __TERMHUB_DUT_HXX__

#include "iobase.hxx"
#include "ringbuf.hxx"

namespace TermHub {
struct Dut {
    virtual void send_break(){};
    void inject(const char *p, size_t l);
    virtual ~Dut() {}

    void start();
    void shutdown();
    virtual void child_close() = 0;
    virtual std::string child_connect() = 0;
    virtual void child_async_read() = 0;
    virtual void child_async_write(size_t length, const char *data) = 0;

    void write_start();
    void read_handler(const boost::system::error_code &error, size_t length);
    void write_handler(const boost::system::error_code &error, size_t length);

    virtual void status_dump(std::stringstream &ss, const now_t &base_time) = 0;

    Dut(boost::asio::io_service &asio, HubPtr h, const std::string &type);

    void reconnect_timeout_start();
    void reconnect_timeout_handler();

    void stat_connected_inc() { stat.connected.inc(1); }
    void stat_rx_inc(uint64_t x) { stat.rx.inc(x); }
    void stat_tx_drop_inc(uint64_t x) { stat.tx_drop.inc(x); }
    void stat_tx_request(uint64_t x) { stat.tx_pending = x; }
    void stat_tx_error() { stat.tx_error.inc(1); }
    void stat_tx_complete(uint64_t x);

  protected:
    bool dead = false;
    bool sleeping_ = false;
    bool shutting_down_ = false;
    bool write_in_progress_ = false;
    IoStat stat;

    HubPtr hub_;
    boost::asio::deadline_timer timer_;

    std::string child_type_;
    RingBuf<4096> write_buf_;
    std::array<char, 32> read_buf_;
};
} // namespace TermHub

#endif
