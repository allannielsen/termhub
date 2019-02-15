#include <algorithm>

#include "hub.hxx"
#include "iobase.hxx"
#include "signal_exit.hxx"

namespace TermHub {

std::shared_ptr<Hub> Hub::create() {
    std::shared_ptr<Hub> p(new Hub());
    signal_exit_reg(std::bind(&Hub::shutdown, p));
    return p;
}

void Hub::post(IoPtr peer, const std::string &s) {
    auto i = sinks.begin();

    while (i != sinks.end()) {
        auto p = i->lock();
        if (p) {
            if (p != peer) {
                p->inject(s);
            }
            ++i;
        } else {
            i = sinks.erase(i);
        }
    }
}

void Hub::shutdown() {
    auto i = sinks.begin();

    while (i != sinks.end()) {
        auto p = i->lock();

        if (p) {
            p->shutdown();
            ++i;
        } else {
            i = sinks.erase(i);
        }
    }
}

void Hub::connect(IoPtr c) { sinks.push_back(c); }

void Hub::disconnect(IoPtr c) {
    auto i = sinks.begin();

    while (i != sinks.end()) {
        auto p = i->lock();

        if (p) {
            if (p == c)
                i = sinks.erase(i);
            else
                ++i;
        } else {
            i = sinks.erase(i);
        }
    }
}
}  // namespace TermHub
