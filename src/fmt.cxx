#include "fmt.hxx"
#include <algorithm>
#include <vector>
#include <string.h>
#include <sstream>
#include <streambuf>
#include <iterator>

namespace TermHub {
namespace Fmt {

std::ostream &operator<<(std::ostream &o, LogTemplate l) {
    const char *f = strrchr(l.file_, '/');
    if (f)
        f++;
    else
        f = l.file_;

    std::stringstream ss;
    ss << "[" << f << ":" << l.line_ << " ";

    for (int i = ss.tellp(); i < 32; ++i) ss.put(' ');
    ss << "] ";

    copy(std::istreambuf_iterator<char>(ss), std::istreambuf_iterator<char>(),
         std::ostreambuf_iterator<char>(o));
    return o;
}

template <unsigned BASE>
bool lexical_convert(const char c, unsigned &v);

template <>
inline bool lexical_convert<2>(const char c, unsigned &v) {
    if (c >= '0' && c <= '1') {
        v = c - '0';
        return true;
    }
    return false;
}

template <>
inline bool lexical_convert<8>(const char c, unsigned &v) {
    if (c >= '0' && c <= '7') {
        v = c - '0';
        return true;
    }
    return false;
}

template <>
inline bool lexical_convert<16>(const char c, unsigned &v) {
    if (c >= '0' && c <= '9') {
        v = c - '0';
        return true;

    } else if (c >= 'A' && c <= 'F') {
        v = (c - 'A') + 10;
        return true;

    } else if (c >= 'a' && c <= 'f') {
        v = (c - 'a') + 10;
        return true;
    }

    return false;
}

template <typename T, unsigned BASE, unsigned MIN_DIGITS, unsigned MAX_DIGITS>
struct IntParser {
    typedef T value_type;

    IntParser() : i(0) {}

    T i;
};

template <typename T, unsigned BASE, unsigned MIN_DIGITS, unsigned MAX_DIGITS>
bool parse(const char *&b, const char *e,
           IntParser<T, BASE, MIN_DIGITS, MAX_DIGITS> &i) {
    static_assert(BASE == 2 || BASE == 8 || BASE == 16,
                  "BASE must be 2, 8 or 16");
    i.i = 0;
    T val = 0;
    const char *_b = b;
    unsigned digit_cnt = 0;

    while (b != e) {
        unsigned d;

        if (!lexical_convert<BASE>(*b, d)) break;

        ++b;
        ++digit_cnt;
        val *= BASE;
        val += d;

        if (digit_cnt >= MAX_DIGITS) break;
    }

    if (digit_cnt == 0) goto Error;
    if (MIN_DIGITS != 0 && digit_cnt < MIN_DIGITS) goto Error;
    if (MAX_DIGITS != 0 && digit_cnt > MAX_DIGITS) goto Error;

    i.i = (T)val;
    return true;

Error:
    b = _b;
    return false;
}


static inline bool is_whitespace(char c) { return c == ' ' || c == '\t'; }

bool parse(const char *&b, const char *e, const Word &s) {
    std::string w;
    const char *_b = b;

    // skip white spaces
    while (b != e && is_whitespace(*b)) ++b;

    // consume
    while (b != e && !is_whitespace(*b)) w.push_back(*b++);

    while (b != e && is_whitespace(*b)) ++b;

    if (w.size()) {
        swap(w, s.w_);
        return true;
    }

    // Error
    b = _b;
    return false;
}

bool parse(const char *&b, const char *e, const Literal &lit) {
    const char *_b = b;
    const char *i = lit.t.c_str();
    const char *lit_end = i + lit.t.size();

    // skip white spaces
    while (lit.accept_pre_whitespace_ && b != e && is_whitespace(*b)) {
        ++b;
    }

    while (b != e && i != lit_end) {
        if (*b != *i) goto Error;
        ++b;
        ++i;
    }

    // skip white spaces
    while (lit.accept_post_whitespace_ && b != e && is_whitespace(*b)) {
        ++b;
    }

    // Check that the complete literal is parsed
    if (i != lit_end) goto Error;

    return true;

Error:
    b = _b;
    return false;
}

std::ostream &operator<<(std::ostream &o, const Literal &e) {
    o << e.t;
    return o;
}

bool parse(const char *&b, const char *e, Comment &s) {
    const char *_b = b;

    while (b != e && is_whitespace(*b)) {
        ++b;
    }

    if (b == e) return true;

    if (*b == '#') {
        b = e;
        return true;
    }

    b = _b;
    return false;
}
std::ostream &operator<<(std::ostream &o, const Comment &e) { return o; }

bool parse(const char *&b, const char *e, EscapedString &s_) {
    std::string s;
    const char *i = b;
    bool escape_mode = false;
    bool escape_hex_mode = false;

    Literal quote_start("\"", true, false);
    Literal quote_end("\"", false, true);

    // Consume starting quote
    if (!parse(i, e, quote_start)) return false;

    // parse string body
    while (i != e) {
        if (escape_hex_mode) {
            IntParser<uint8_t, 16, 2, 2> hex;
            if (!parse(i, e, hex)) return false;
            s.push_back(hex.i);
            escape_hex_mode = false;

        } else if (escape_mode) {
            switch (*i) {
            case '"':
                s.push_back('"');
                break;

            case '\\':
                s.push_back('\\');
                break;

            case 'n':
                s.push_back('\n');
                break;

            case 'r':
                s.push_back('\r');
                break;

            case 't':
                s.push_back('\t');
                break;

            case 'x':
                escape_hex_mode = true;
                break;

            default:
                return false;
            }
            escape_mode = false;
            ++i;

        } else if (*i == '\\') {
            // We are entering eschape mode
            escape_mode = true;
            ++i;

        } else if (*i == '"') {
            // End of string
            break;

        } else {
            // Normal string processing
            s.push_back(*i);
            ++i;
        }
    }

    // Consume ending quote
    if (!parse(i, e, quote_end)) return false;

    // String has been parsed
    b = i;
    swap(s_.t, s);
    return true;
}

std::ostream &operator<<(std::ostream &o, const EscapedString &e) {
    o.put('"');

    for (auto c : e.t) {
        if (c == '\n') {
            o << "\\n";
        } else if (c == '\r') {
            o << "\\r";
        } else if (c == '\t') {
            o << "\\t";
        } else if (c == '\\') {
            o << "\\\\";
        } else if (c >= 32 && c <= 126) {
            o.put(c);
        } else {
            o << "\\x" << hex_fixed<2>(c);
        }
    }

    o.put('"');
    return o;
}

}  // namespace Fmt
}  // namespace TermHub
