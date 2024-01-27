#ifndef __TERMHUB_TTY_HXX__
#define __TERMHUB_TTY_HXX__

#include <array>
#include <boost/asio.hpp>

#include "iobase.hxx"
#include "key-tokenizer.hxx"
#include "process.hxx"
#include "ringbuf.hxx"

namespace TermHub {

struct Tty;

typedef std::shared_ptr<Tty> TtyPtr;

struct Tty : public Iobase, std::enable_shared_from_this<Tty> {
    static TtyPtr create(boost::asio::io_service &asio, HubPtr h, DutPtr d) {
        auto tty = TtyPtr(new Tty(asio, h, d));
        tty->init();
        return tty;
    }

    ~Tty();

    void start();
    void inject(const char *p, size_t l);
    void shutdown();

    void status_dump(std::stringstream &ss, const now_t &now);

    struct Action {
        virtual void exec(TtyPtr tty, DutPtr dut) = 0;
    };

    struct ActionQuit : public Action {
        void exec(TtyPtr tty, DutPtr dut);
    };

    struct ActionBreak : public Action {
        void exec(TtyPtr tty, DutPtr dut);
    };

    struct ActionInject : public Action {
        ActionInject(std::string s) : data(s) {}

        void exec(TtyPtr tty, DutPtr dut);

        std::string data;
    };

    struct ActionSpawn : public Action {
        ActionSpawn(boost::asio::io_service &a, HubPtr h, std::string s,
                    TtyPtr t)
            : tty_(t), asio_(a), hub_(h), app_(s) {}

        void exec(TtyPtr tty, DutPtr dut);

        TtyPtr tty_;
        boost::asio::io_service &asio_;
        HubPtr hub_;
        std::string app_;
    };

    void action_spawn_completed(int pid, int flags, int status);
    void input_disable() { input_disable_ = true; }
    void input_enable() { input_disable_ = false; }

    void write_start();
    void write_completion(const boost::system::error_code &error,
                          size_t length);

  private:
    Tty(boost::asio::io_service &asio, HubPtr h, DutPtr d);
    void init();

    void handle_read(const boost::system::error_code &error, size_t length);
    void handle_read_data(const char *c, size_t s);
    void handle_read_token(uint32_t);

    bool tty_cmd_add(const char *&b, const char *e);
    bool tty_cmd_del(const char *&b, const char *e);

    typedef std::array<char, 32> Buf;
    RingBuf<4096> tx_buf_;
    typedef Buf::iterator BufItr;

    Buf buf_;
    DutPtr dut_;
    HubPtr hub_;
    boost::asio::io_service &asio_;
    boost::asio::posix::stream_descriptor input_;
    boost::asio::posix::stream_descriptor output_;
    struct termios old_tio, new_tio;

    KeyTokenizer::Inventory key_tokenizer;

    size_t write_in_progress_ = 0;
    bool shutting_down_ = false;
    bool input_disable_ = false;

    std::shared_ptr<Process> process_;
    std::vector<std::shared_ptr<Action>> actions;
};
} // namespace TermHub

#endif
