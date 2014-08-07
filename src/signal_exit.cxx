#include <vector>
#include "signal_exit.hxx"
#include "log.hxx"

namespace TermHub {
static std::vector<signal_exit_cb_t> signal_exit_subscriber;

void signal_exit() {
    for (auto e : signal_exit_subscriber) {
        LOG("exit-cb");
        e();
    }
}

void signal_exit_reg(signal_exit_cb_t cb) {
    LOG("exit-reg");
    signal_exit_subscriber.push_back(cb);
}

}  // namespace TermHub

