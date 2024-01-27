#ifndef __TERMHUB_DUT_HXX__
#define __TERMHUB_DUT_HXX__

#include "iobase.hxx"

namespace TermHub {
struct Dut {
    virtual void send_break(){};
    virtual void inject(const char *p, size_t l) = 0;
    virtual ~Dut() {}
    virtual void start() = 0;
    virtual void shutdown() = 0;
    virtual void status_dump(std::stringstream &ss, const now_t &base_time) = 0;

    void stat_connected_inc() { stat.connected.inc(1); }
    void stat_rx_inc(uint64_t x) { stat.rx.inc(x); }
    void stat_tx_drop_inc(uint64_t x) { stat.tx_drop.inc(x); }
    void stat_tx_request(uint64_t x) { stat.tx_pending = x; }
    void stat_tx_error() { stat.tx_error.inc(1); }
    void stat_tx_complete(uint64_t x);

    bool dead = false;
    IoStat stat;
};
} // namespace TermHub

#endif
