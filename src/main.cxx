#include <boost/program_options.hpp>
#include <fstream>
#include <wordexp.h>
#include <signal.h>
#include "dut-dummy-echo.hxx"
#include "tty.hxx"
#include "hub.hxx"
#include "log.hxx"
#include "tcp_server.hxx"
#include "tcp_client.hxx"
#include "rs232_client.hxx"
#include "cmd-registry.hxx"

using namespace TermHub;

namespace TermHub {
std::ofstream log;
unsigned int listen_port_number = 0;
}  // namespace TermHub

bool apply_config_file(std::string f) {
    LOG("Reading config file: " << f);

    wordexp_t exp;
    if (wordexp(f.c_str(), &exp, 0) != 0) {
        std::cout << "Failed to open file: \"" << f << "\"\r\n";
        return false;
    }

    if (exp.we_wordc != 1) {
        std::cout << "Failed to open file: \"" << f << "\"\r\n";
        return false;
    }

    f = exp.we_wordv[0];

    wordfree(&exp);

    std::string line;
    std::ifstream conf(f);

    if (!conf.fail()) {
        while (std::getline(conf, line)) {
            LOG("Conf file line: " << line);
            if (!global_cmd_exec(line))
                std::cout << "Error applying: " << line << "\r\n";

        }

        return true;
    } else {
        std::cout << "Failed to open file: \"" << f << "\"\r\n";
        return false;
    }
}

int main(int ac, char *av[]) {
#ifndef NDEBUG
    ::TermHub::log.open("./log.txt");
#endif

    signal(SIGPIPE, SIG_IGN);
    LOG("hello world");

    namespace po = boost::program_options;
    using boost::asio::ip::tcp;
    po::variables_map vm;

    int baudrate = 0;
    std::string device;
    std::string config_file;
    boost::asio::io_service asio;

    po::options_description desc("Allowed options");
    desc.add_options()
            (
                    "help,h",
                    "produce help message"
            ) (
                    "config,c",
                    po::value<std::string>(&config_file),
                    "Config file"
            ) (
                    "port,p",
                    po::value<unsigned int>(&listen_port_number),
                    "Open a TCP server and listen on port"
            ) (
                    "baudrate,b",
                    po::value<int>(&baudrate)->default_value(115200),
                    "Baudrate"
            ) (
                    "headless",
                    "Headless/daemon mode - IO not printed locally (only useful along with -p)"
            ) (
                    "device,d",
                    po::value<std::string>(&device)->required(),
                    "Device - /dev/rs232-device|(ip-address|hostname):port|dummy"
            );

    try {
        po::store(po::parse_command_line(ac, av, desc), vm);
        po::notify(vm);
    }
    catch (const std::exception &e) {
        std::cout << "Error:" << std::endl;
        std::cout << "   " << e.what() << std::endl;
        std::cout << std::endl;
        std::cout << desc << std::endl;
        return 1;
    }

    if (vm.count("help")) {
        std::cout << desc << std::endl;
        return 1;
    }

    HubPtr hub = Hub::create();

    // try to figure out what device type to use
    IoPtr dut;
    if (device == "dummy") {
        dut = DutDummyEcho::create(hub);
        std::cout << "Connected to dummy echo-device\r\n";

    } else if (std::find(device.begin(), device.end(), ':') != device.end()) {
        auto x = std::find(device.begin(), device.end(), ':');
        std::string host(device.begin(), x);
        std::string port(x + 1, device.end());
        dut = TcpClient::create(asio, hub, host, port);
        std::cout << "Connected to " << device << "\r\n";

    } else if (std::find(device.begin(), device.end(), '/') != device.end()) {
        std::string path(device.begin(), device.end());
        dut = Rs232Client::create(asio, hub, path, baudrate);
    }

    typedef TcpServer<tcp::endpoint, TcpSession> Server;
    std::shared_ptr<Server> server_auto;
    if (listen_port_number > 0) {
        try {
            tcp::endpoint ep(boost::asio::ip::tcp::v4(), listen_port_number);
            server_auto = Server::create(asio, ep, dut, hub);
            std::cout << "Listening on " << listen_port_number << std::endl;
        } catch (...) {
            std::cout << "Failed to listen on port " << listen_port_number
                      << std::endl;
            exit(-1);
        }
    }

    if (vm.count("headless)")) {
        IoPtr tty = Tty::create(asio, hub, dut);

        bool config_applied = false;
        if (vm.count("config")) {
            config_applied = apply_config_file(config_file);
        }

        if (!config_applied) {
            config_applied = apply_config_file("~/.termhub");
        }

        if (!config_applied) {
            LOG("No configuration file could be read - applying defaults");
            global_cmd_exec("tty-cmd-add \"\\x1dq\" quit");
        }

        hub->connect(tty);
        tty->start();
    }

    asio.run();

    return 0;
}

