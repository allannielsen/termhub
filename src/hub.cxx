#include <boost/asio.hpp>
#include <iostream>

#include "hub.hxx"
#include "iobase.hxx"
#include "log.hxx"
#include "signal_exit.hxx"

namespace TermHub {

class DisconnectPostpone {
  public:
    DisconnectPostpone(Hub *h) : hub(h) { hub->disconnect_not_now += 1; }

    ~DisconnectPostpone() {
        hub->disconnect_not_now -= 1;
        hub->disconnect();
    }

  private:
    Hub *hub;
};

std::shared_ptr<Hub> Hub::create() {
    std::shared_ptr<Hub> p(new Hub());
    signal_exit_reg(std::bind(&Hub::shutdown, p));
    return p;
}

// void Hub::post(IoPtr peer, const char *data, size_t l) {
//     DisconnectPostpone dis(this);
//     auto i = sinks.begin();
//
//     LOG("post: " << sinks.size() << " " << disconnect_not_now << " DATA: >"
//     << l
//                  << "<");
//     while (i != sinks.end()) {
//         auto p = i->lock();
//         if (p) {
//             if (p != peer) {
//                 // LOG("post-inject");
//                 p->inject(data, l);
//             }
//             ++i;
//         } else {
//             // LOG("post-ease");
//             i = sinks.erase(i);
//         }
//     }
//
//     // LOG("post done");
// }

void Hub::post(DutPtr peer, const char *data, size_t l) {
    DisconnectPostpone dis(this);
    auto i = sinks.begin();

    LOG("post: " << sinks.size() << " " << disconnect_not_now << " DATA: >" << l
                 << "<");
    while (i != sinks.end()) {
        auto p = i->lock();
        if (p) {
            p->inject(data, l);
            ++i;
        } else {
            // LOG("post-ease");
            i = sinks.erase(i);
        }
    }

    // LOG("post done");
}

void Hub::shutdown() {
    DisconnectPostpone dis(this);
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

void Hub::connect(IoPtr c) {
    DisconnectPostpone dis(this);
    sinks.push_back(c);
}

void Hub::disconnect() {
    if (disconnect_not_now) {
        return;
    }

    auto i = sinks.begin();

    while (i != sinks.end()) {
        auto p = i->lock();

        if (!p || p->dead) {
            i = sinks.erase(i);
            LOG("hub: disconnect delete");
        } else {
            ++i;
        }
    }
}

void Hub::status_dump(std::stringstream &ss, const now_t &base_time) {
    DisconnectPostpone dis(this);
    auto i = sinks.begin();

    while (i != sinks.end()) {
        auto p = i->lock();
        if (p) {
            p->status_dump(ss, base_time);
            ++i;
        } else {
            i = sinks.erase(i);
        }
    }
}

} // namespace TermHub
