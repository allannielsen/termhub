#ifndef __TERMHUB_DUT_DUMMY_ECHO_HXX__
#define __TERMHUB_DUT_DUMMY_ECHO_HXX__
#include "hub.hxx"
#include "iobase.hxx"
#include <iostream>
#include <memory>

namespace TermHub {
struct DutDummyEcho : public Iobase,
                      public std::enable_shared_from_this<DutDummyEcho> {

    static IoPtr create(HubPtr h) {
        return std::shared_ptr<DutDummyEcho>(new DutDummyEcho(h));
    }

    ~DutDummyEcho() {}

    void start() {}

    void inject(const std::string &s) {
        std::string res;
        for (auto c : s) {
            if (c == '\r')
                res.append("\r\n");
            else
                res.push_back(c);
        }
        hub_->post(shared_from_this(), res);
    }

    void shutdown() {}

  private:
    DutDummyEcho(HubPtr h) : hub_(h) {}
    HubPtr hub_;
};
}  // namespace TermHub

#endif
