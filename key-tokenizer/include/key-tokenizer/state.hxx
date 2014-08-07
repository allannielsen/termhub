#ifndef _KEY_TOKENIZER_STATE_HXX_
#define _KEY_TOKENIZER_STATE_HXX_

namespace KeyTokenizer {
enum State {
    DATA,
    TOKEN,
    TOKEN_TIMMED,
    INTERMEDIATE,
};
}  // namespace KeyTokenizer

#endif  // _KEY_TOKENIZER_STATE_HXX_
