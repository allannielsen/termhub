#ifndef __TERMHUB_RING_BUF_HXX__
#define __TERMHUB_RING_BUF_HXX__

#include <algorithm>

namespace TermHub {

template <size_t buf_size> class RingBuf {
public:
  RingBuf() {}

  void clear() {
    head = 0;
    tail = 0;
  }

  size_t push(const char *data, size_t length) {
    size_t l = length;
    for (int i = 0; i < 2; i++) {
      size_t chunk = std::min(l, cap_cont());
      if (chunk == 0) {
        break;
      }

      memcpy(&buf[tail], data, chunk);
      tail_inc(chunk);
      l -= chunk;
    }

    return length - l;
  }

  const char *get_data_buf(size_t *length) {
    if (head == tail) {
      *length = 0;
      return 0;
    } else if (tail > head) {
      // ....ddddddddddddd.......
      //     ^            ^
      //     head         tail
      *length = tail - head;
      return &buf[head];
    } else {
      // ddddd...........dddddddd
      //      ^          ^
      //      tail       head
      *length = buf_size - head;
      return &buf[head];
    }
  }

  void consume(size_t length) {
    head += length;
    if (head == buf_size) {
      head = 0;
    }
  }

  size_t size() const {
    if (head == tail) {
      return 0;
    } else if (tail > head) {
      // ....ddddddddddddd.......
      //     ^            ^
      //     head         tail
      return tail - head;
    } else {
      // ddddd...........dddddddd
      //      ^          ^
      //      tail       head
      return buf_size - (head - tail);
    }
  }

  size_t cap() const {
    // minus 1, because tail must point at a non-used slot, otherwise it is
    // not possible to distinguish between empty and full.
    return buf_size - size() - 1;
  }

private:
  void tail_inc(size_t s) {
    tail += s;
    if (tail == buf_size) {
      tail = 0;
    }
  }

  size_t cap_cont() const {
    if (tail >= head) {
      if (head == 0) {
        // head == tail means empty!
        return buf_size - tail - 1;
      } else {
        return buf_size - tail;
      }
    } else {
      return head - tail - 1;
    }
  }

  size_t head = 0;
  size_t tail = 0;
  std::array<char, buf_size> buf;
};

} // namespace TermHub
#endif // __TERMHUB_RING_BUF_HXX__
