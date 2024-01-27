#ifndef __TERMHUB_HUB_HXX__
#define __TERMHUB_HUB_HXX__

#include <chrono>
#include <memory>
#include <vector>

namespace TermHub {
struct Iobase;
struct Dut;
typedef std::chrono::time_point<std::chrono::system_clock> now_t;

typedef std::shared_ptr<Iobase> IoPtr;
typedef std::shared_ptr<Dut> DutPtr;

class DisconnectPostpone;

struct Hub {
    friend class DisconnectPostpone;

    static std::shared_ptr<Hub> create();
    // void post(IoPtr peer, const char *data, size_t l);
    void post(DutPtr peer, const char *data, size_t l);

    void status_dump(std::stringstream &ss, const now_t &now);
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
} // namespace TermHub

#endif
