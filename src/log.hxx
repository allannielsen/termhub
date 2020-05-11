#include <iostream>
#include <fstream>
#include "fmt.hxx"

namespace TermHub {
extern std::ofstream log;
extern bool logging_on;
}  // namespace TermHub

#define LOG(...)                                                               \
    do {                                                                       \
        if (::TermHub::logging_on) {                                           \
            ::TermHub::log << TermHub::Fmt::LogTemplate(__FILE__, __LINE__)    \
                           << __VA_ARGS__ << std::endl;                        \
            ::TermHub::log.flush();                                            \
        }                                                                      \
    } while(false)

