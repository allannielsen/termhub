#ifndef __TERMHUB_TCP_CLIENT_HXX__
#define __TERMHUB_TCP_CLIENT_HXX__

#include <iostream>
#include <string>
#include <memory>
#include <boost/asio.hpp>

#include "hub.hxx"
#include "iobase.hxx"
#include "signal_exit.hxx"

namespace TermHub {
struct TcpClient : public Iobase, std::enable_shared_from_this<TcpClient> {
    static IoPtr create(boost::asio::io_service &asio, HubPtr h,
                        std::string host, std::string port) {
        std::shared_ptr<TcpClient> p(new TcpClient(asio, h, host, port));
        LOG("create " << (void *)p.get());
        signal_exit_reg(std::bind(&TcpClient::shutdown, p));
        p->start();
        return p;
    }

    ~TcpClient() {
        LOG("tcp-client destruct" << (void *)this);
    }

    void shutdown() {
        LOG("Shutting down TcpClient");
        shutting_down_ = true;
        socket_.cancel();
        socket_.close();
    }

    void start() {
        LOG("start");
        auto x = std::bind(&TcpClient::handle_read, shared_from_this(),
                           std::placeholders::_1, std::placeholders::_2);
        boost::asio::async_read(socket_,
                                boost::asio::buffer(&buf_[0], buf_.size()),
                                boost::asio::transfer_at_least(1), x);
    }

    void handle_read(const boost::system::error_code &error, size_t length) {
        LOG("handle_read" << (void *)this);
        if (shutting_down_) return;

        if (error) {
            LOG("Error: " << error.message());
            return;
        }

        std::string s(&buf_[0], length);
        hub_->post(shared_from_this(), s);
        start();
    }

    void inject(const std::string &s) {
        LOG("tcp-client inject");
        write(socket_, boost::asio::buffer(s));
    }

  private:
    TcpClient(boost::asio::io_service &asio, HubPtr h, std::string host,
              std::string port)
            : hub_(h), socket_(asio) {
        using boost::asio::ip::tcp;
        tcp::resolver resolver(asio);
        LOG("tcp-client construct " << (void *)this);
        boost::asio::connect(socket_, resolver.resolve({host, port}));
    }

    HubPtr hub_;
    std::array<char, 32> buf_;
    boost::asio::ip::tcp::socket socket_;
    bool shutting_down_ = false;
};
}  // namespace TermHub

#endif
