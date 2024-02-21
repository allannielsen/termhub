#ifndef __TERMHUB_TCP_SERVER_HXX__
#define __TERMHUB_TCP_SERVER_HXX__

#include <boost/asio.hpp>
#include <functional>

#include "dut.hxx"
#include "iobase.hxx"
#include "log.hxx"
#include "ringbuf.hxx"
#include "signal_exit.hxx"

namespace TermHub {
class TcpSession : public Iobase,
                   public std::enable_shared_from_this<TcpSession> {
  public:
    static std::shared_ptr<TcpSession> create(boost::asio::io_service &asio,
                                              DutPtr d, HubPtr h) {
        return std::shared_ptr<TcpSession>(new TcpSession(asio, d, h));
    }

    void status_dump(std::stringstream &ss, const now_t &base_time) override {
        ss << "tcp-session(" << peer_ep_ << ") {\n";
        stat.pr(ss, base_time);
        ss << "}\n\n";
    }

    ~TcpSession() {}

    boost::asio::ip::tcp::socket &socket() { return socket_; }
    boost::asio::ip::tcp::endpoint &ep() { return peer_ep_; }

    void write_start() {
        if (write_in_progress_ || shutting_down_) {
            return;
        }

        size_t length = 0;
        tx_data = tx_buf_.get_data_buf(&length);
        if (length == 0) {
            return;
        }

        write_in_progress_ = true;
        stat_tx_request(length);
        LOG("tcp-session(" << (void *)this << "): async-write");
        auto x = std::bind(&TcpSession::write_completion, shared_from_this(),
                           std::placeholders::_1, std::placeholders::_2);
        boost::asio::async_write(socket_, boost::asio::buffer(tx_data, length),
                                 boost::asio::transfer_at_least(1), x);
    }

    void write_completion(const boost::system::error_code &error,
                          size_t length) {
        write_in_progress_ = false;
        if (shutting_down_ || dead)
            return;

        if (error) {
            LOG("tcp-session(" << (void *)this
                               << "): Write-Error: " << error.message());
            dead = true;
            hub_->disconnect();
            tx_buf_.clear();
            stat_tx_error();
            return;
        }

        LOG("tcp-session(" << (void *)this << "): async-write complete"
                           << length);
        stat_tx_complete(length);
        tx_buf_.consume(length);
        write_start();
    }

    void inject(const char *p, size_t l) override {
        l -= tx_buf_.push(p, l);
        stat_tx_drop_inc(l);
        write_start();
    }
    void start() override { read(); }

    void shutdown() override {
        shutting_down_ = true;
        socket_.cancel();
        socket_.close();
    }

    void io_wake_up_read() override {
        if (read_pending_) {
            read_pending_ = false;
            LOG("tcp-session(" << (void *)this << "): wake-up-read");
            read();
        } else {
            LOG("tcp-session(" << (void *)this
                               << "): wake-up-read - no pending");
        }
    }

  private:
    TcpSession(boost::asio::io_service &asio, DutPtr d, HubPtr h)
        : dut_(d), hub_(h), socket_(asio) {}

    void read() {
        if (shutting_down_ || dead) {
            return;
        }

        LOG("tcp-session(" << (void *)this << "): async-read");
        auto x = std::bind(&TcpSession::handle_read2, shared_from_this(),
                           std::placeholders::_1);
        socket_.async_wait(boost::asio::ip::tcp::socket::wait_read, x);
    }

    void handle_read2(const boost::system::error_code &error) {
        boost::system::error_code ec;

        if (error) {
            LOG("tcp-session(" << (void *)this
                               << "): Error (disconnect): " << error.message());
            dead = true;
            hub_->disconnect();
            return;
        }

        auto b = dut_->inject_buffer();
        if (b.size() == 0) {
            LOG("tcp-session(" << (void *)this
                               << "): Data ready - but pipeline is full");
            read_pending_ = true;
            hub_->sleep_read(shared_from_this());
            stat_rx_deferred();
        } else {
            size_t s = socket_.read_some(b, ec);
            stat_rx_inc(s);
            if (ec) {
                LOG("tcp-session(" << (void *)this << "): Error (disconnect): "
                                   << error.message());
                dead = true;
                hub_->disconnect();
                return;
            }
            LOG("tcp-session(" << (void *)this
                               << "): async-read complete: " << s);
            dut_->inject_commit(s);
            read();
        }
    }

    boost::asio::ip::tcp::endpoint peer_ep_;
    DutPtr dut_;
    HubPtr hub_;
    std::array<char, 32> rx_buf_;
    RingBuf<102400> tx_buf_;
    boost::asio::ip::tcp::socket socket_;
    bool shutting_down_ = false;
    bool write_in_progress_ = false;
    const char *tx_data;
    bool read_pending_ = false;
};

template <typename EndPoint, typename Session>
struct TcpServer
    : public std::enable_shared_from_this<TcpServer<EndPoint, Session>> {

    typedef TcpServer<EndPoint, Session> THIS;
    typedef std::enable_shared_from_this<TcpServer<EndPoint, Session>> BASE;

    static std::shared_ptr<THIS> create(boost::asio::io_service &asio,
                                        EndPoint ep, DutPtr d, HubPtr h) {
        std::shared_ptr<THIS> p(new TcpServer(asio, ep, d, h));
        signal_exit_reg(std::bind(&TcpServer::shutdown, p));
        p->start_accept();
        return p;
    }

    void status_dump(std::stringstream &ss, const now_t &base_time) {
        ss << "TCP Server listning at: " << ep_ << "\n";
    }

    void shutdown() {
        LOG("tcp-server: Shutting down TcpServer");
        shutting_down_ = true;
        acceptor_.cancel();
        acceptor_.close();
    }

  private:
    TcpServer(boost::asio::io_service &asio, EndPoint ep, DutPtr d, HubPtr h)
        : ep_(ep), dut_(d), hub_(h), asio_(asio), acceptor_(asio_) {
        acceptor_.open(ep.protocol());
        acceptor_.set_option(
            boost::asio::ip::tcp::acceptor::reuse_address(true));
        acceptor_.bind(ep);
        acceptor_.listen();
    }

    void start_accept() {
        std::shared_ptr<Session> new_session =
            Session::create(asio_, dut_, hub_);
        acceptor_.async_accept(new_session->socket(), new_session->ep(),
                               std::bind(&TcpServer::handle_accept, this,
                                         new_session, std::placeholders::_1));
        LOG("tcp-server: async accept");
    }

    void handle_accept(std::shared_ptr<Session> new_session,
                       const boost::system::error_code &error) {
        if (shutting_down_)
            return;

        if (error) {
            LOG("tcp-server: Error: " << error.message());
            return;
        }

        LOG("tcp-server: Accept new session: " << (void *)new_session.get());
        hub_->connect(new_session);
        new_session->start();
        start_accept();
    }

    EndPoint ep_;
    DutPtr dut_;
    HubPtr hub_;
    boost::asio::io_service &asio_;
    boost::asio::ip::tcp::acceptor acceptor_;
    bool shutting_down_ = false;
};

typedef TcpServer<boost::asio::ip::tcp::endpoint, TcpSession> Server;
typedef std::shared_ptr<Server> TcpServerPtr;

} // namespace TermHub

#endif
