#ifndef __TERMHUB_DUT_DUMMY_ECHO_HXX__
#define __TERMHUB_DUT_DUMMY_ECHO_HXX__
#include "dut.hxx"
#include "hub.hxx"
#include "iobase.hxx"

namespace TermHub {
struct DutDummyEcho : public Dut {
    DutDummyEcho(boost::asio::io_service &asio, HubPtr h);
    ~DutDummyEcho();

    void child_close() override;
    void child_connect() override;
    void child_async_read() override;
    void child_async_write(size_t s, const char *c) override;

    void status_dump(std::stringstream &ss, const now_t &base_time) override;

  private:
    boost::system::error_code write_ec;
    boost::asio::deadline_timer async_write_timer_;

    boost::system::error_code read_ec;
    boost::asio::deadline_timer async_read_timer_;
};

} // namespace TermHub

#endif
