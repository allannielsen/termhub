#ifndef __TERMHUB_IOBASE_HXX__
#define __TERMHUB_IOBASE_HXX__

#include <string>
#include <memory>

#include "hub.hxx"

namespace TermHub {
struct Iobase {
    virtual void send_break() {};
    virtual void inject(const char *p, size_t l) = 0;
    virtual ~Iobase() {}
    virtual void start() = 0;
    virtual void shutdown() = 0;

    bool dead = false;
};

}  // namespace TermHub

#endif
