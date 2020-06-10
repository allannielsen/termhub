#include <algorithm>

#include "hub.hxx"
#include "log.hxx"
#include "iobase.hxx"
#include "signal_exit.hxx"

namespace TermHub {

class DisconnectPostpone {
  public:
    DisconnectPostpone(Hub *h) : hub(h) {
        hub->disconnect_not_now += 1;
    }

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

void Hub::post(IoPtr peer, const std::string &s) {
    DisconnectPostpone dis(this);
    auto i = sinks.begin();

    LOG("post: " << sinks.size() << " " << disconnect_not_now << " DATA: >" << s << "<");
    while (i != sinks.end()) {
        auto p = i->lock();
        if (p) {
            if (p != peer) {
                //LOG("post-inject");
                p->inject(s);
            }
            ++i;
        } else {
            //LOG("post-ease");
            i = sinks.erase(i);
        }
    }

    //LOG("post done");
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
    //LOG("disconnect cnt: " << disconnect_not_now);

    if (disconnect_not_now) {
        //LOG("disconnect postponed");
        return;
    }

    //LOG("do disconnect");
    auto i = sinks.begin();

    while (i != sinks.end()) {
        auto p = i->lock();

        if (!p || p->dead) {
            i = sinks.erase(i);
            //LOG("disconnect delete");
        } else {
            ++i;
        }
    }
}
}  // namespace TermHub
