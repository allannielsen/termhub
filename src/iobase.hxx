#ifndef __TERMHUB_IOBASE_HXX__
#define __TERMHUB_IOBASE_HXX__

#include "hub.hxx"
#include <boost/asio.hpp>
#include <chrono>

namespace TermHub {

typedef std::chrono::time_point<std::chrono::system_clock> now_t;

struct StatCnt {
    now_t ts_updated;
    uint64_t val = 0;
    uint64_t val_total = 0;

    void inc(uint64_t x) {
        ts_updated = std::chrono::system_clock::now();
        val += x;
        val_total += x;
    }

    void ss_fill(std::stringstream &ss, size_t basew, size_t w) {
        if (ss.str().size() - basew < w) {
            for (size_t fill = w - (ss.str().size() - basew); fill > 0;
                 fill--) {
                ss << " ";
            }
        }
    }

    void pr(std::ostream &o, const char *name, size_t width,
            const now_t &base_time) {
        size_t size_base = 0;
        std::stringstream ss;
        auto since = std::chrono::duration_cast<std::chrono::seconds>(
            base_time - ts_updated);

        ss << "    " << name << ":";
        ss_fill(ss, 0, width);
        ss << val;
        ss_fill(ss, 0, width + 12);
        if (ts_updated.time_since_epoch().count() == 0) {
            ss << "Never updated\n";
        } else {
            ss << "Updated: " << since.count() << " seconds ago\n";
        }

        size_base = ss.str().size();
        ss << "    " << name << "-total:";
        ss_fill(ss, size_base, width);
        ss << val_total << "\n";
        val = 0;

        o << ss.str();
    }
};

struct IoStat {
    StatCnt rx;
    StatCnt tx, tx_drop, tx_error;
    StatCnt connected;
    uint64_t tx_pending = 0;

    void pr(std::ostream &o, const now_t &base_time) {
        rx.pr(o, "rx", 20, base_time);
        tx.pr(o, "tx", 20, base_time);
        tx_drop.pr(o, "tx-drop", 20, base_time);
        tx_error.pr(o, "tx-error", 20, base_time);
        o << "    tx-pending:     " << tx_pending << "\n";
        rx.pr(o, "con", 20, base_time);
    }
};

struct Iobase {
    virtual void send_break() {}
    virtual void io_wake_up_read() = 0;
    virtual void inject(const char *p, size_t l) = 0;
    virtual ~Iobase() {}
    virtual void start() = 0;
    virtual void shutdown() = 0;
    virtual void status_dump(std::stringstream &ss, const now_t &base_time) = 0;

    void stat_connected_inc() { stat.connected.inc(1); }
    void stat_rx_inc(uint64_t x) { stat.rx.inc(x); }
    void stat_tx_drop_inc(uint64_t x) { stat.tx_drop.inc(x); }
    void stat_tx_request(uint64_t x) { stat.tx_pending = x; }
    void stat_tx_error() { stat.tx_error.inc(1); }
    void stat_tx_complete(uint64_t x) {
        stat.tx.inc(x);
        stat.tx_pending = 0;
    }

    bool dead = false;
    IoStat stat;
};

} // namespace TermHub

#endif
