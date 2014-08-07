#include <iostream>
#include <fstream>
#include "fmt.hxx"

namespace TermHub {
extern std::ofstream log;
}  // namespace TermHub

#ifdef NDEBUG
#define LOG(...)
#else
#define LOG(...)                                                               \
    ::TermHub::log << TermHub::Fmt::LogTemplate(__FILE__, __LINE__)            \
                   << __VA_ARGS__ << std::endl;                                \
    ::TermHub::log.flush()
#endif

