#ifndef __TERMHUB_FMT_HXX__
#define __TERMHUB_FMT_HXX__

#include <iostream>

namespace TermHub {
namespace Fmt {

struct LogTemplate {
    LogTemplate(const char *file, unsigned line) : file_(file), line_(line) {}
    const char *const file_;
    const unsigned line_;
};

std::ostream &operator<<(std::ostream &o, LogTemplate l);

template <typename T>
struct FormatHex {
    FormatHex(T &_t, char _b, uint32_t _min = 0, uint32_t _max = 0,
              char _c = '0')
            : t(_t),
              base(_b),
              width_min(_min),
              width_max(_max),
              fill_char(_c) {}

    T &t;
    char base;
    uint32_t width_min;
    uint32_t width_max;
    char fill_char;
};

template <typename T>
FormatHex<T> hex(T &t, char _c = '0') {
    return FormatHex<T>(t, 'a', 0, 0, _c);
}

template <typename T>
FormatHex<T> HEX(T &t, char _c = '0') {
    return FormatHex<T>(t, 'A', 0, 0, _c);
}

template <uint32_t width, typename T>
FormatHex<T> hex_fixed(T &t, char _c = '0') {
    return FormatHex<T>(t, 'a', width, width, _c);
}

template <uint32_t width, typename T>
FormatHex<T> HEX_fixed(T &t, char _c = '0') {
    return FormatHex<T>(t, 'A', width, width, _c);
}

template <uint32_t min, uint32_t max, typename T>
FormatHex<T> hex_range(T &t, char _c = '0') {
    return FormatHex<T>(t, 'a', min, max, _c);
}

template <uint32_t min, uint32_t max, typename T>
FormatHex<T> HEX_range(T &t, char _c = '0') {
    return FormatHex<T>(t, 'A', min, max, _c);
}

template <typename T>
std::ostream &operator<<(std::ostream &o, FormatHex<T> s) {
    char buf[32];
    char *i = buf + 31;
    char *e = buf + 31;

    typename std::make_unsigned<T>::type val = s.t;

    if (val == 0) {
        *i = '0';
        --i;
    }

    while (val) {
        char v = val & 0xf;
        if (v > 9)
            *i = (v - 10) + s.base;
        else
            *i = v + '0';
        val >>= 4;
        --i;
    }

    // push fill chars
    for (unsigned fill = e - i; fill < s.width_min; ++fill) o.put(s.fill_char);

    // write the actual value
    o.write(i + 1, e - i);

    return o;
}

// WORD ////////////////////////////////////////////////////////////////////////
struct Word {
    Word(std::string &w) : w_(w) {}
    std::string &w_;
};
bool parse(const char *&b, const char *e, const Word &s);

// LITERAL /////////////////////////////////////////////////////////////////////
struct Literal {
    Literal(const std::string s, bool pre = false, bool post = false)
            : t(s),
              accept_pre_whitespace_(pre),
              accept_post_whitespace_(post) {}

    std::string t;
    bool accept_pre_whitespace_;
    bool accept_post_whitespace_;
};
bool parse(const char *&b, const char *e, const Literal &s);
std::ostream &operator<<(std::ostream &o, const Literal &e);

// COMMENT /////////////////////////////////////////////////////////////////////
struct Comment {
    Comment();
};
bool parse(const char *&b, const char *e, Comment &s);
std::ostream &operator<<(std::ostream &o, const Comment &e);

// ESCAPEDSTRING ///////////////////////////////////////////////////////////////
struct EscapedString {
    EscapedString(std::string &x) : t(x) {}
    std::string &t;
};
bool parse(const char *&b, const char *e, EscapedString &s);
std::ostream &operator<<(std::ostream &o, const EscapedString &e);


// PARSE SEQUENCE OF PARSERS ///////////////////////////////////////////////////
template <typename... Args>
inline bool parse_group_(const char *&b, const char *e, Args &... args);

template <>
inline bool parse_group_(const char *&b, const char *e) {
    return true;
}

template <typename Arg, typename... Args>
inline bool parse_group_(const char *&b, const char *e, Arg &arg,
                         Args &... args) {
    if (parse(b, e, arg)) return parse_group_(b, e, args...);
    return false;
}

template <typename... Args>
bool parse_group(const char *&b, const char *e, Args &... args) {
    const char *_b = b;
    if (!parse_group_(b, e, args...)) {
        b = _b;
        return false;
    }

    return true;
}

template <typename... Args>
bool parse_group(const std::string &s, Args &... args) {
    const char *b = s.c_str();
    const char *e = b + s.size();

    if (!parse_group(b, e, args...))
        return false;

    return b == e;
}

}  // namespace Fmt
}  // namespace TermHub

#endif
