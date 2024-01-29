#include "rs232_client.hxx"

namespace io = boost::asio;

namespace TermHub {
Rs232Client::Rs232Client(boost::asio::io_service &asio, HubPtr h,
                         std::string path, int baudrate)
    : Dut(asio, h, "RS232"), serial_(asio), path_(path), baudrate_(baudrate) {
    LOG("rs232(" << (void *)this << "): construct");
}

void Rs232Client::child_connect() {
    serial_.open(path_);
    serial_.set_option(io::serial_port_base::baud_rate(baudrate_));
    serial_.set_option(
        io::serial_port_base::parity(io::serial_port_base::parity::none));
    serial_.set_option(io::serial_port_base::character_size(8));
    serial_.set_option(io::serial_port_base::flow_control(
        io::serial_port_base::flow_control::none));
    serial_.set_option(
        io::serial_port_base::stop_bits(io::serial_port_base::stop_bits::one));
}

void Rs232Client::child_close() { serial_.close(); }

void Rs232Client::send_break() {
    LOG("rs232(" << (void *)this << "): break");
    try {
        serial_.send_break();
    } catch (...) {
        LOG("rs232(" << (void *)this << "): break-unknown error...");
    }
}

void Rs232Client::child_async_read() {
    // LOG("rs232(" << (void *)this << "): async_read_started");
    auto x = std::bind(&Dut::read_handler, shared_from_this(),
                       std::placeholders::_1, std::placeholders::_2);
    boost::asio::async_read(
        serial_, boost::asio::buffer(&read_buf_[0], read_buf_.size()),
        boost::asio::transfer_at_least(1), x);
    // LOG("rs232(" << (void *)this << "): async_read_started-ended");
}

void Rs232Client::child_async_write(size_t length, const char *data) {
    auto x = std::bind(&Dut::write_handler, shared_from_this(),
                       std::placeholders::_1, std::placeholders::_2);
    boost::asio::async_write(serial_, boost::asio::buffer(data, length),
                             boost::asio::transfer_at_least(1), x);
}

void Rs232Client::status_dump(std::stringstream &ss, const now_t &base_time) {
    ss << "dut-rs232(" << path_ << ") {\n";
    stat.pr(ss, base_time);
    ss << "}\n\n";
}

} // namespace TermHub
