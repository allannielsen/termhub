#ifndef __TERMHUB_DUT_DUMMY_ECHO_HXX__
#define __TERMHUB_DUT_DUMMY_ECHO_HXX__
#include "dut.hxx"
#include "hub.hxx"
#include "iobase.hxx"
#include <boost/asio.hpp>

namespace TermHub {
struct DutDummyEcho : public Dut {
    DutDummyEcho(boost::asio::io_service &asio, HubPtr h);
    ~DutDummyEcho();

    void child_close() override;
    std::string child_connect() override;
    void child_async_read() override;

    void read_delay_handler(const boost::system::error_code &error,
                            size_t length);
    void write_delay_handler(const boost::system::error_code &error,
                             size_t length);

    void child_async_write(size_t s, const char *c) override;

    void status_dump(std::stringstream &ss, const now_t &base_time) override;

  private:
    boost::asio::deadline_timer read_delay_timer_;
    boost::asio::deadline_timer write_delay_timer_;

#if 0
    RingBuf<4> dummy_buf_;

    bool internal_timer_running = false;

    boost::system::error_code write_ec;
    boost::asio::deadline_timer async_write_timer_;

    boost::system::error_code read_ec;
    boost::asio::deadline_timer async_read_timer_;

    boost::system::error_code read_parent_ec;
    boost::asio::deadline_timer async_read_parent_timer_;
#endif

    boost::asio::readable_pipe rp;
    boost::asio::writable_pipe wp;
};

} // namespace TermHub

#endif
