#define BOOST_TEST_MODULE MyTest
#include <boost/test/unit_test.hpp>
#include "cmd-hot-key.hxx"

namespace TermHub {

BOOST_AUTO_TEST_CASE(x) {
    CmdHotKey k;
    std::string exp;
    exp.push_back(0x1b);
    exp.push_back('O');
    exp.push_back('P');
    BOOST_CHECK(k.process(R"(hot-key "\x1bOP" quit)"));
    BOOST_CHECK_EQUAL(k.str1, exp);
    BOOST_CHECK_EQUAL(k.type, CmdHotKey::Quit);
}

}  // namespace KeyTokenizer
