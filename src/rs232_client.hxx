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
struct Rs232Client : public Dut {
    // static DutPtr create(boost::asio::io_service &asio, HubPtr h,
    //                      std::string path, int baudrate) {
    //     std::shared_ptr<Dut> p(new Rs232Client(asio, h, path, baudrate));
    //     LOG("create rs232 client " << (void *)p.get());
    //     signal_exit_reg(std::bind(&Rs232Client::shutdown, p));

    //    return p;
    //}
    Rs232Client(boost::asio::io_service &asio, HubPtr h, std::string path,
                int baudrate);

    ~Rs232Client() { LOG("destruct rs232 client " << (void *)this); }

    void open_and_start();
    void start();

    void child_close() override;
    void child_connect() override;
    void child_async_read() override;
    void child_async_write(size_t length, const char *data) override;

    void send_break() override;

    void status_dump(std::stringstream &ss, const now_t &base_time) override;

  private:
    RingBuf<10240> tx_buf_;
    bool write_in_progress_ = false;
    boost::asio::serial_port serial_;
    std::string path_;
    int baudrate_;
};
} // namespace TermHub

#endif
