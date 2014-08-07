#include "key-tokenizer/action.hxx"

namespace KeyTokenizer {
Action operator%(const Action &a, const Action &b) {
    Action res;

    // least common represantant
    res.consume_data = std::min(a.consume_data, b.consume_data);

    // prioritize two complete matches
    if (a.complete() && b.complete()) {
        // first match wins
        if (a.consume_data < b.consume_data) {
            return a;
        } else if (b.consume_data < a.consume_data) {
            return b;
        } else {
            // match start at same place, longest match wins
            if (a.consume_token > b.consume_token)
                return a;
            else
                return b;
        }
    }

    // timed completion overrules partial matches!
    if (a.complete_timed() || b.complete_timed()) {
        if (a.consume_token > b.consume_token)
            return a;
        else
            return b;
    }

    // one or more partial matches implies result is partial
    if (a.partial_match || b.partial_match) {
        res.partial_match = true;

        // leave consume_token at zero!
        return res;
    }

    // no partial complete or multiple - just a single match
    if (a.complete()) {
        return a;
    } else if (b.complete()) {
        return b;
    }

    // consume the data nobody wants
    return res;
}

}  // namespace KeyTokenizer
