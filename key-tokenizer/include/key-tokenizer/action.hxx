#ifndef _KEY_TOKENIZER_ACTION_HXX_
#define _KEY_TOKENIZER_ACTION_HXX_

#include <string>
#include <chrono>
#include "key-tokenizer/state.hxx"

namespace KeyTokenizer {
struct Action {
    typedef std::chrono::nanoseconds ns_t;

    Action()
            : token(0),
              silence_time(0),
              partial_match(false),
              consume_data(0xffffffff),
              consume_token(0) {}

    Action(uint32_t t, ns_t s, bool p, uint32_t cd, uint32_t ct)
            : token(t),
              silence_time(s),
              partial_match(p),
              consume_data(cd),
              consume_token(ct) {}

    Action(const Action &rhs) = default;
    Action &operator=(const Action &rhs) = default;

    bool complete_timed() const {
        return consume_token != 0 && silence_time != ns_t(0);
    }

    bool complete() const { return consume_token != 0; }


    // represented token
    uint32_t token;

    // silence time required after token was matched
    ns_t silence_time;

    bool partial_match;
    size_t consume_data;
    size_t consume_token;
};

inline bool operator==(const Action &a, const Action &b) {
    return a.token == b.token && a.silence_time == b.silence_time &&
           a.partial_match == b.partial_match &&
           a.consume_data == b.consume_data &&
           a.consume_token == b.consume_token;
}

inline bool operator!=(const Action &a, const Action &b) {
    return a.token != b.token || a.silence_time != b.silence_time ||
           a.partial_match != b.partial_match ||
           a.consume_data != b.consume_data ||
           a.consume_token != b.consume_token;
}

// Combine two actions
Action operator%(const Action &a, const Action &b);
inline Action &operator%=(Action &a, const Action &b) {
    Action c = a % b;
    a = c;
    return a;
}
}  // namespace KeyTokenizer

#endif  // _KEY_TOKENIZER_ACTION_HXX_
