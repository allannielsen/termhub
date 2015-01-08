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

    void reconnect_timeout() {
        auto x = std::bind(&TcpClient::handle_reconnect_timeout,
                           shared_from_this(), std::placeholders::_1);
        timer_.expires_from_now(boost::posix_time::seconds(1));
        timer_.async_wait(x);
    }

    void handle_reconnect_timeout(const boost::system::error_code& e) {
        bool ok = false;
        try {
            connect();
            ok = true;
        } catch (...) {
            ok = false;
        }

        if (ok) {
            std::cout << "Reconected :-)\r\n";
            start();
            return;
        } else {
            reconnect_timeout();
            return;
        }
    }

    void connect() {
        using boost::asio::ip::tcp;

        tcp::resolver resolver(asio_);
        boost::asio::connect(socket_, resolver.resolve({host_, port_}));

        struct timeval tv;
        tv.tv_sec  = 1;
        tv.tv_usec = 0;

        int fd = socket_.native_handle();
        setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

        boost::asio::socket_base::keep_alive option(true);
        socket_.set_option(option);
    }

    void handle_read(const boost::system::error_code &error, size_t length) {
        LOG("handle_read" << (void *)this);
        if (shutting_down_) return;

        if (error) {
            std::cout << "\r\nError: " << error.message() << "\r\n";
            LOG("Error: " << error.message());

            socket_.cancel();
            socket_.close();

            bool ok = false;
            try {
                connect();
                ok = true;
            } catch (...) {
                ok = false;
            }

            if (ok) {
                std::cout << "Reconected :-)\r\n";
                start();
                return;
            } else {
                std::cout << "Failed to reconnect (will continue to retry)\r\n";
                reconnect_timeout();
                return;
            }
        }

        std::string s(&buf_[0], length);
        hub_->post(shared_from_this(), s);
        start();
    }

    void inject(const std::string &s) {
        LOG("tcp-client inject");
        try {
            write(socket_, boost::asio::buffer(s));
        } catch (...) {
            // dont care
        }
    }

  private:
    TcpClient(boost::asio::io_service &asio, HubPtr h, std::string host,
              std::string port)
            : asio_(asio), timer_(asio), hub_(h), socket_(asio), host_(host),
              port_(port) {
        LOG("tcp-client construct " << (void *)this);
        connect();
    }

    boost::asio::io_service &asio_;
    boost::asio::deadline_timer timer_;
    HubPtr hub_;
    std::array<char, 32> buf_;
    boost::asio::ip::tcp::socket socket_;
    bool shutting_down_ = false;
    std::string host_, port_;
};
}  // namespace TermHub

#endif
