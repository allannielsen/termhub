#include <boost/program_options.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <fstream>
#include <wordexp.h>
#include <signal.h>
#include <vector>
#include <regex>
#include <iomanip>
#include <sstream>
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
bool logging_on = false;
unsigned int listen_port_number = 0;
static std::chrono::time_point<std::chrono::system_clock> start_time;

struct ConfEntry {
    int line;
    std::string cmd;
    std::string args;
    bool processed = false;
};

std::vector<ConfEntry> conf_db;

ConfEntry* conf_db_get(const std::string &s) {
    for (auto &e : conf_db) {
        if (e.cmd == s) {
            return &e;
        }
    }

    return nullptr;
}

bool apply_config_file(std::string f) {
    LOG("Reading config file: " << f);
    std::regex r_empty("\\s*");
    std::regex r_comment("\\s*#.*");
    std::regex r_cmd_args("\\s*([[:graph:]]+)\\s*(.*)");

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
        int i = 0;
        while (std::getline(conf, line)) {
            i ++;
            std::smatch m;

            if (std::regex_match(line, r_empty)) {
                // std::cout << "empty line at " << i << std::endl;
            } else if (std::regex_match(line, r_comment)) {
                // std::cout << "comment at " << i << std::endl;
            } else if (std::regex_match(line, m, r_cmd_args) && m.size() == 3) {
                conf_db.emplace_back(ConfEntry({i, m[1].str(), boost::trim_copy(m[2].str()), false}));
            } else {
                std::cout << "Error parsing line " << i << std::endl;
            }
        }

        return true;
    } else {
        return false;
    }
}

std::string log_time_stamp() {
    std::stringstream ss;
    std::chrono::duration<long int, std::milli> diff =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now() - start_time);
    ss << std::setw(10) << diff.count() << " ";
    return ss.str();
}
}  // namespace TermHub

template<typename T>
void config_overlay(const boost::program_options::variables_map &vm, const char *name, T& dst) {
    if (vm.count(name) == 0) {
        auto c = conf_db_get(name);
        if (c) {
            try {
                dst = boost::lexical_cast<T>(c->args);
                c->processed = true;
            } catch (...) {
                std::cout << "Error at line: " << c->line << std::endl;
            }
        }
    }
}

int main(int ac, char *av[]) {
    signal(SIGPIPE, SIG_IGN);
    start_time = std::chrono::system_clock::now();

    namespace po = boost::program_options;
    using boost::asio::ip::tcp;
    po::variables_map vm;

    int headless = 0;
    int baudrate = 0;
    std::string device;
    std::string config_file;
    boost::asio::io_service asio;
    std::string log_file;

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
                    "log",
                    po::value<std::string>(&log_file),
                    "log file (only for debugging)"
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
                    po::value<std::string>(&device),
                    "Device - /dev/rs232-device|(ip-address|hostname):port|dummy"
            );

    po::positional_options_description pos;
    pos.add("device", -1);

    try {
        po::store(po::command_line_parser(ac, av).
                  options(desc).positional(pos).run(), vm);
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

    if (log_file.size()) {
        ::TermHub::log.open(log_file);
        logging_on = true;
        LOG("hello log file");
    }

    bool has_config = false;
    if (vm.count("config")) {
        has_config = apply_config_file(config_file);
    }

    if (!has_config) {
        has_config = apply_config_file("~/.termhub");
    }

    if (!has_config) {
        has_config = apply_config_file("/etc/termhub");
    }

    if (!has_config) {
        has_config = apply_config_file("/usr/local/etc/termhub");
    }

    if (!has_config) {
        std::cout << "No config file applied!" << std::endl;
    }

    config_overlay(vm, "port", listen_port_number);
    config_overlay(vm, "baudrate", baudrate);
    config_overlay(vm, "device", device);

    if (conf_db_get("headless")) {
        conf_db_get("headless")->processed = true;
        headless = 1;
    }

    if (vm.count("headless")) {
        headless = 1;
    }

    HubPtr hub = Hub::create();

    // try to figure out what device type to use
    IoPtr dut;
    if (device.size() == 0) {
        std::cout << "No device specified!" << std::endl;
        exit(-1);

    } else if (device == "dummy") {
        dut = DutDummyEcho::create(hub);
        std::cout << "Connected to dummy echo-device\r\n";

    } else if (std::find(device.begin(), device.end(), ':') != device.end()) {
        auto x = std::find(device.begin(), device.end(), ':');
        std::string host(device.begin(), x);
        std::string port(x + 1, device.end());
        dut = TcpClient::create(asio, hub, host, port);
        std::cout << "Connected to " << device << "\r\n";

    } else if (std::find(device.begin(), device.end(), '/') != device.end()) {
        std::cout << "Device at " << device << "\r\n";
        std::string path(device.begin(), device.end());
        dut = Rs232Client::create(asio, hub, path, baudrate);

    } else {
        std::cout << "Device: " << device << " not understood" << std::endl;
        exit(-1);
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

    if (headless) {
        std::cout << "Running in headless mode" << std::endl;

    } else {
        IoPtr tty = Tty::create(asio, hub, dut);

        if (has_config) {
            for (auto &e : conf_db) {
                if (e.processed)
                    continue;

                if (global_cmd_exec(e.cmd, e.args))
                    e.processed = true;
            }
        } else {
            LOG("No configuration file could be read - applying defaults");
            global_cmd_exec("tty-cmd-add", "\"\\x1dq\" quit");
        }

        hub->connect(tty);
        tty->start();
    }

    asio.run();

    return 0;
}

