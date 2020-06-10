#include <iostream>
#include <fstream>
#include "fmt.hxx"

namespace TermHub {
extern std::ofstream log;
extern bool logging_on;
std::string log_time_stamp();
}  // namespace TermHub

#define LOG(...)                                                               \
    do {                                                                       \
        if (::TermHub::logging_on) {                                           \
            ::TermHub::log << TermHub::Fmt::LogTemplate(__FILE__, __LINE__)    \
                           << ::TermHub::log_time_stamp()                      \
                           << __VA_ARGS__ << std::endl;                        \
            ::TermHub::log.flush();                                            \
        }                                                                      \
    } while(false)

