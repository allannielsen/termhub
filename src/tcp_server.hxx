#ifndef __TERMHUB_TCP_SERVER_HXX__
#define __TERMHUB_TCP_SERVER_HXX__

#include <functional>
#include <boost/asio.hpp>

#include "log.hxx"
#include "iobase.hxx"
#include "signal_exit.hxx"

namespace TermHub {
class TcpSession : public Iobase,
                   public std::enable_shared_from_this<TcpSession> {
  public:
    static std::shared_ptr<TcpSession> create(boost::asio::io_service& asio,
                                              IoPtr d, HubPtr h) {
        return std::shared_ptr<TcpSession>(new TcpSession(asio, d, h));
    }

    ~TcpSession() {}

    boost::asio::ip::tcp::socket& socket() { return socket_; }

    void inject(const std::string& s) {
        LOG("tcp-server inject");
        write(socket_, boost::asio::buffer(s));
    }
    void start() { read(); }

    void shutdown() {
        shutting_down_ = true;
        socket_.cancel();
        socket_.close();
    }

  private:
    TcpSession(boost::asio::io_service& asio, IoPtr d, HubPtr h)
            : dut_(d), hub_(h), socket_(asio) {}

    void read() {
        auto x = std::bind(&TcpSession::handle_read, shared_from_this(),
                           std::placeholders::_1, std::placeholders::_2);
        boost::asio::async_read(socket_,
                                boost::asio::buffer(&buf_[0], buf_.size()),
                                boost::asio::transfer_at_least(1), x);
    }

    void handle_read(const boost::system::error_code& error, size_t length) {
        if (shutting_down_) return;

        if (error) {
            LOG("Error: " << error.message());
            hub_->disconnect(shared_from_this());
            return;
        }

        std::string s(&buf_[0], length);
        dut_->inject(s);
        read();
    }

    IoPtr dut_;
    HubPtr hub_;
    std::array<char, 32> buf_;
    boost::asio::ip::tcp::socket socket_;
    bool shutting_down_ = false;
};

template <typename EndPoint, typename Session>
struct TcpServer
    : public std::enable_shared_from_this<TcpServer<EndPoint, Session>> {

    typedef TcpServer<EndPoint, Session> THIS;
    typedef std::enable_shared_from_this<TcpServer<EndPoint, Session>> BASE;

    static std::shared_ptr<THIS> create(boost::asio::io_service& asio,
                                        EndPoint ep, IoPtr d, HubPtr h) {
        std::shared_ptr<THIS> p(new TcpServer(asio, ep, d, h));
        signal_exit_reg(std::bind(&TcpServer::shutdown, p));
        p->start_accept();
        return p;
    }

    void shutdown() {
        LOG("Shutting down TcpServer");
        shutting_down_ = true;
        acceptor_.cancel();
        acceptor_.close();
    }

  private:
    TcpServer(boost::asio::io_service& asio, EndPoint ep, IoPtr d, HubPtr h)
            : dut_(d), hub_(h), asio_(asio), acceptor_(asio_) {
        acceptor_.open(ep.protocol());
        acceptor_.set_option(
            boost::asio::ip::tcp::acceptor::reuse_address(true));
        acceptor_.bind(ep);
        acceptor_.listen();
    }

    void start_accept() {
        std::shared_ptr<Session> new_session =
            Session::create(asio_, dut_, hub_);
        acceptor_.async_accept(new_session->socket(),
                               std::bind(&TcpServer::handle_accept, this,
                                         new_session, std::placeholders::_1));
    }

    void handle_accept(std::shared_ptr<Session> new_session,
                       const boost::system::error_code& error) {
        if (shutting_down_) return;

        if (error) {
            LOG("Error: " << error.message());
            return;
        }

        LOG("Accept");
        hub_->connect(new_session);
        new_session->start();
        start_accept();
    }

    IoPtr dut_;
    HubPtr hub_;
    boost::asio::io_service& asio_;
    boost::asio::ip::tcp::acceptor acceptor_;
    bool shutting_down_ = false;
};
}  // namespace TermHub

#endif
