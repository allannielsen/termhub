#include "dut.hxx"

namespace TermHub {

void Dut::stat_tx_complete(uint64_t x) {
    stat.tx.inc(x);
    stat.tx_pending = 0;
}

} // namespace TermHub
