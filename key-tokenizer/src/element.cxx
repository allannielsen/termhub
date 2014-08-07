#include <algorithm>
#include "key-tokenizer/element.hxx"

namespace KeyTokenizer {
static inline size_t compare(const char *a, const char *a_end, const char *b,
                             const char *b_end) {
    size_t cnt = 0;
    while (a != a_end && b != b_end) {
        if (*a != *b) break;
        a++, b++, cnt++;
    }

    return cnt;
}

Action Element::process(const char *const s_begin, const char *const s_end) {
    Action a;
    a.token = token_;
    a.silence_time = silence_time_;

    const char *i = s_begin;

    for (; i != s_end; ++i) {
        // prefix match cnt
        size_t input_window = s_end - i;

        // number of chars in common
        size_t match_cnt = compare(i, s_end, &*match_.begin(), &*match_.end());

        if (match_cnt == std::min(input_window, match_.size())) {
            // For the match to be valid - either no silence_time is required or
            // no data may be present after the match point
            if (silence_time_ == ns_t(0) || match_cnt == input_window) {
                // consume date untill this point
                a.consume_data = i - s_begin;

                // If the match is complete (input does cover the complete match
                // string) then update the consume_token field
                if (match_cnt == match_.size())
                    a.consume_token = match_.size();
                else
                    a.partial_match = true;

                // Valid match found - may be incomplete
                return a;
            }
        }
    }

    // no possible matches found! consume all as data!
    a.consume_data = s_end - s_begin;

    return a;
}
}  // namespace KeyTokenizer
