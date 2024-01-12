#ifndef __TERMHUB_DUT_DUMMY_ECHO_HXX__
#define __TERMHUB_DUT_DUMMY_ECHO_HXX__
#include "hub.hxx"
#include "iobase.hxx"
#include <memory>
#include <string>

namespace TermHub {
struct DutDummyEcho : public Iobase,
                      public std::enable_shared_from_this<DutDummyEcho> {

    static IoPtr create(HubPtr h) {
        return std::shared_ptr<DutDummyEcho>(new DutDummyEcho(h));
    }

    ~DutDummyEcho() {}

    void status_dump(std::stringstream &ss, const now_t &base_time) {
        ss << "Dummy-dut here, no further status\n";
    }

    void start() {}

    void inject(const char *c, size_t s) {
        std::string res;
        for (size_t i = 0; i < s; ++i, ++c) {
            if (*c == '\r')
                res.append("\r\n");
            else
                res.push_back(*c);
        }

        stat_rx_inc(s);
        stat_tx_complete(s);
        hub_->post(shared_from_this(), res.c_str(), res.size());
    }

    void shutdown() {}

  private:
    DutDummyEcho(HubPtr h) : hub_(h) {}
    HubPtr hub_;
};
} // namespace TermHub

#endif
