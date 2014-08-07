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
    for (auto i = sinks.begin(); i != sinks.end();) {
        if (i->expired()) {
            sinks.erase(i);
        } else {
            auto p = i->lock();
            if (p != peer) p->inject(s);
            ++i;
        }
    }
}

void Hub::shutdown() {
    for (auto i = sinks.begin(); i != sinks.end();) {
        if (i->expired()) {
            sinks.erase(i);
        } else {
            auto p = i->lock();
            p->shutdown();
            ++i;
        }
    }
}

void Hub::connect(IoPtr c) { sinks.push_back(c); }

void Hub::disconnect(IoPtr c) {
    for (auto i = sinks.begin(); i != sinks.end();) {
        if (i->expired()) {
            sinks.erase(i);
        } else {
            auto p = i->lock();
            if (p == c)
                sinks.erase(i);
            else
                ++i;
        }
    }
}
}  // namespace TermHub
