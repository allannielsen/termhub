#include <assert.h>
#include <algorithm>
#include "key-tokenizer/inventory.hxx"
#include "log.hxx"

namespace KeyTokenizer {

void Inventory::flush() {
    // flush meets all timeouts
    if (pending_timed.complete()) action_apply(pending_timed);

    // rest is data
    if (buf.size() > 0) data_cb_(buf.c_str(), buf.size());

    // empty buffer to avoid double commit
    buf.resize(0);
}

void Inventory::key_add(const std::string &key, uint32_t token,
                        ns_t silence_time) {
    for (auto const &e : elements)
        if (e.match_ == key)
            throw std::runtime_error("Can not add key as it already exists!");

    elements.emplace_back(key, token, silence_time);
}

void Inventory::put(std::chrono::steady_clock::time_point tp) {
    if (!pending_timed.complete_timed()) return;

    ns_t diff = tp - last_time;

    if (diff > pending_timed.silence_time) {
        action_apply(pending_timed);
        pending_timed = Action();
        last_time = tp;
    }
}

Inventory::ns_t Inventory::put(std::chrono::steady_clock::time_point tp,
                               const char *s, size_t size) {
    buf.append(s, size);

    Action action;
    for (auto &e : elements) action %= e.process(buf);

    if (action.complete_timed()) return action.silence_time;

    action_apply(action);
    return ns_t(0);
}

void Inventory::action_apply(const Action &a) {
    const char *begin = buf.c_str();
    const char *end = buf.c_str() + buf.size();
    size_t consume_data = 0;

    LOG("Apply action: " << a.consume_data << " " << a.consume_token << " "
                         << a.partial_match);

    if (a.consume_data) {
        LOG("data_cb: " << std::string(begin, buf.size()));
        consume_data = std::min(a.consume_data, buf.size());
        data_cb_(begin, consume_data);
    }

    begin += consume_data;

    if (a.consume_token) {
        LOG("token_cb: " << a.token);
        assert(a.consume_token <= (size_t)(end - begin));
        token_cb_(a.token);
        begin += a.consume_token;
    }

    std::string new_buf(begin, end);
    swap(buf, new_buf);
}

}  // namespace KeyTokenizer
