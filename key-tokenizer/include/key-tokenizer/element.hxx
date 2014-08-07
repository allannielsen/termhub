#ifndef _KEY_TOKENIZER_ELEMENT_HXX_
#define _KEY_TOKENIZER_ELEMENT_HXX_

#include <string>
#include <chrono>
#include "action.hxx"

namespace KeyTokenizer {
struct Element {
    typedef std::chrono::nanoseconds ns_t;

    Element(const std::string &s, uint32_t token, ns_t t = ns_t(0))
            : silence_time_(t), match_(s), token_(token) {}

    // find first possible match position
    Action process(const char *const b, const char *const e);
    Action process(const std::string &s) {
        return process(&*s.begin(), &*s.end());
    }

    ns_t silence_time_;
    std::string match_;
    uint32_t token_;
};
}  // namespace KeyTokenizer

#endif  // _KEY_TOKENIZER_ELEMENT_HXX_
