#ifndef __TERMHUB_HUB_HXX__
#define __TERMHUB_HUB_HXX__

#include <string>
#include <memory>
#include <vector>

namespace TermHub {
struct Iobase;
typedef std::shared_ptr<Iobase> IoPtr;

class DisconnectPostpone;

struct Hub {
    friend class DisconnectPostpone;

    static std::shared_ptr<Hub> create();
    void post(IoPtr peer, const std::string &s);

    void shutdown();
    void connect(IoPtr c);
    void disconnect();

  private:
    Hub() {}
    std::vector<std::weak_ptr<Iobase>> sinks;
    bool shutting_down_ = false;
    unsigned int disconnect_not_now = 0;
};

typedef std::shared_ptr<Hub> HubPtr;
}  // namespace TermHub

#endif
