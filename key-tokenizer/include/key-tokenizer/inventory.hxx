#ifndef _KEY_TOKENIZER_INVENTORY_HXX_
#define _KEY_TOKENIZER_INVENTORY_HXX_

#include <string>
#include <chrono>
#include <vector>
#include <functional>
#include "key-tokenizer/element.hxx"

namespace KeyTokenizer {
struct Inventory {
  public:
    typedef std::chrono::nanoseconds ns_t;
    typedef std::function<void(uint32_t)> token_cb_t;
    typedef std::function<void(const char *c, size_t s)> data_cb_t;

    Inventory(token_cb_t token_cb, data_cb_t data_cb)
            : token_cb_(token_cb), data_cb_(data_cb) {}

    void flush();
    void key_add(const std::string &key, uint32_t token,
                 ns_t silence_time = ns_t(0));

    // prefered interface
    void put() { return put(std::chrono::steady_clock::now()); }
    ns_t put(char c) { return put(std::chrono::steady_clock::now(), c); }
    ns_t put(const char *buf, size_t size) {
        return put(std::chrono::steady_clock::now(), buf, size);
    }

    // should only be used for testing
    void put(std::chrono::steady_clock::time_point tp);
    ns_t put(std::chrono::steady_clock::time_point tp, char c) {
        return put(tp, &c, 1);
    }
    ns_t put(std::chrono::steady_clock::time_point tp, const char *buf,
             size_t size);

  private:
    void action_apply(const Action &a);

    Action pending_timed;
    token_cb_t token_cb_;
    data_cb_t data_cb_;
    std::chrono::steady_clock::time_point last_time;

    std::string buf;
    std::vector<Element> elements;
};
}  // namespace KeyTokenizer

#endif  // _KEY_TOKENIZER_INVENTORY_HXX_
