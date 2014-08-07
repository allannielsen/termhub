#include "rs232_client.hxx"

namespace io = boost::asio;

namespace TermHub {
Rs232Client::Rs232Client(boost::asio::io_service &asio, HubPtr h,
                         std::string path, int baudrate)
    : hub_(h), serial_(asio, path) {
    LOG("rs232 client construct " << (void *)this);
    serial_.set_option(io::serial_port_base::baud_rate(baudrate));
    serial_.set_option(
            io::serial_port_base::parity(io::serial_port_base::parity::none));
    serial_.set_option(io::serial_port_base::character_size(8));
    serial_.set_option(io::serial_port_base::flow_control(
            io::serial_port_base::flow_control::none));
    serial_.set_option(io::serial_port_base::stop_bits(
            io::serial_port_base::stop_bits::one));
}
void Rs232Client::shutdown() {
    LOG("Shutting down Rs232Client");
    shutting_down_ = true;
    serial_.close();
}

void Rs232Client::start() {
    LOG("start");
    auto x = std::bind(&Rs232Client::handle_read, shared_from_this(),
                       std::placeholders::_1, std::placeholders::_2);
    boost::asio::async_read(serial_, boost::asio::buffer(&buf_[0], buf_.size()),
                            boost::asio::transfer_at_least(1), x);
}

void Rs232Client::handle_read(const boost::system::error_code &error,
                              size_t length) {
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

void Rs232Client::inject(const std::string &s) {
    LOG("tcp-client inject");
    write(serial_, boost::asio::buffer(s));
}
}  // namespace TermHub
