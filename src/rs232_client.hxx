#ifndef __TERMHUB_RS232_CLIENT_HXX__
#define __TERMHUB_RS232_CLIENT_HXX__

#include <iostream>
#include <string>
#include <memory>
#include <boost/asio.hpp>

#include "log.hxx"
#include "hub.hxx"
#include "iobase.hxx"
#include "signal_exit.hxx"

namespace TermHub {
struct Rs232Client : public Iobase, std::enable_shared_from_this<Rs232Client> {
    static IoPtr create(boost::asio::io_service &asio, HubPtr h,
                        std::string path, int baudrate) {
        std::shared_ptr<Rs232Client> p(
                new Rs232Client(asio, h, path, baudrate));
        LOG("create rs232 client " << (void *)p.get());
        signal_exit_reg(std::bind(&Rs232Client::shutdown, p));
        p->start();
        return p;
    }

    ~Rs232Client() { LOG("destruct rs232 client " << (void *)this); }

    void start();
    void shutdown();
    void inject(const std::string &s);
    void handle_read(const boost::system::error_code &error, size_t length);

  private:
    Rs232Client(boost::asio::io_service &asio, HubPtr h, std::string path,
                int baudrate);

    HubPtr hub_;
    std::array<char, 32> buf_;
    bool shutting_down_ = false;
    boost::asio::serial_port serial_;
};
}  // namespace TermHub

#endif

