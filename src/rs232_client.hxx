#ifndef __TERMHUB_RS232_CLIENT_HXX__
#define __TERMHUB_RS232_CLIENT_HXX__

#include <boost/asio.hpp>
#include <iostream>
#include <memory>
#include <string>

#include "dut.hxx"
#include "hub.hxx"
#include "iobase.hxx"
#include "log.hxx"
#include "ringbuf.hxx"
#include "signal_exit.hxx"

namespace TermHub {
struct Rs232Client : public Dut, std::enable_shared_from_this<Rs232Client> {
    static DutPtr create(boost::asio::io_service &asio, HubPtr h,
                         std::string path, int baudrate) {
        std::shared_ptr<Rs232Client> p(
            new Rs232Client(asio, h, path, baudrate));
        LOG("create rs232 client " << (void *)p.get());
        signal_exit_reg(std::bind(&Rs232Client::shutdown, p));

        p->reconnect_timeout();
        return p;
    }

    ~Rs232Client() { LOG("destruct rs232 client " << (void *)this); }

    void open_and_start();
    void start();
    void shutdown();
    void send_break();
    void inject(const char *p, size_t l);
    void handle_read(const boost::system::error_code &error, size_t length);
    void reconnect_timeout();
    void handle_reconnect_timeout(const boost::system::error_code &e);

    void write_start();
    void write_completion(const boost::system::error_code &error,
                          size_t length);

    void status_dump(std::stringstream &ss, const now_t &base_time);

  private:
    Rs232Client(boost::asio::io_service &asio, HubPtr h, std::string path,
                int baudrate);

    HubPtr hub_;
    std::array<char, 32> buf_;
    RingBuf<10240> tx_buf_;
    bool shutting_down_ = false;
    bool write_in_progress_ = false;
    boost::asio::serial_port serial_;
    std::string path_;
    int baudrate_;
    boost::asio::deadline_timer timer_;
    bool sleeping_ = false;
};
} // namespace TermHub

#endif
