#include <functional>

namespace TermHub {
typedef std::function<void()> signal_exit_cb_t;
void signal_exit();
void signal_exit_reg(signal_exit_cb_t cb);
}  // namespace TermHub

