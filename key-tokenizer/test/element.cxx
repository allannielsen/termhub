#define BOOST_TEST_MODULE MyTest
#include <boost/test/unit_test.hpp>

#include "key-tokenizer/element.hxx"
typedef std::chrono::nanoseconds ns_t;

namespace std {
std::ostream& operator<<(std::ostream &o, ns_t t) {
    o << t.count();
    return o;
}
}  // namespace std

namespace KeyTokenizer {

BOOST_AUTO_TEST_CASE(no_match_1) {
    Element e("aaa", 42);
    Action a = e.process("asdf");

    BOOST_CHECK_EQUAL(a.token, 42);
    BOOST_CHECK_EQUAL(a.partial_match, false);
    BOOST_CHECK_EQUAL(a.silence_time, ns_t(0));
    BOOST_CHECK_EQUAL(a.consume_data, 4);
    BOOST_CHECK_EQUAL(a.consume_token, 0);
}

BOOST_AUTO_TEST_CASE(no_match_2) {
    Element e("xxx", 42);
    Action a = e.process("yyy");

    BOOST_CHECK_EQUAL(a.token, 42);
    BOOST_CHECK_EQUAL(a.partial_match, false);
    BOOST_CHECK_EQUAL(a.silence_time, ns_t(0));
    BOOST_CHECK_EQUAL(a.consume_data, 3);
    BOOST_CHECK_EQUAL(a.consume_token, 0);
}

BOOST_AUTO_TEST_CASE(no_match_3) {
    Element e("aa", 42);
    Action a = e.process("bbbbbab");

    BOOST_CHECK_EQUAL(a.token, 42);
    BOOST_CHECK_EQUAL(a.partial_match, false);
    BOOST_CHECK_EQUAL(a.silence_time, ns_t(0));
    BOOST_CHECK_EQUAL(a.consume_data, 7);
    BOOST_CHECK_EQUAL(a.consume_token, 0);
}

BOOST_AUTO_TEST_CASE(match_1) {
    Element e("aa", 42);
    Action a = e.process("aaaaaa");

    BOOST_CHECK_EQUAL(a.token, 42);
    BOOST_CHECK_EQUAL(a.partial_match, false);
    BOOST_CHECK_EQUAL(a.silence_time, ns_t(0));
    BOOST_CHECK_EQUAL(a.consume_data, 0);
    BOOST_CHECK_EQUAL(a.consume_token, 2);
}

BOOST_AUTO_TEST_CASE(match_2) {
    Element e("aa", 42);
    Action a = e.process("bbbbba");

    BOOST_CHECK_EQUAL(a.token, 42);
    BOOST_CHECK_EQUAL(a.partial_match, true);
    BOOST_CHECK_EQUAL(a.silence_time, ns_t(0));
    BOOST_CHECK_EQUAL(a.consume_data, 5);
    BOOST_CHECK_EQUAL(a.consume_token, 0);
}

BOOST_AUTO_TEST_CASE(match_3) {
    Element e("aa", 42);
    Action a = e.process("bbbbbaa");

    BOOST_CHECK_EQUAL(a.token, 42);
    BOOST_CHECK_EQUAL(a.partial_match, false);
    BOOST_CHECK_EQUAL(a.silence_time, ns_t(0));
    BOOST_CHECK_EQUAL(a.consume_data, 5);
    BOOST_CHECK_EQUAL(a.consume_token, 2);
}

BOOST_AUTO_TEST_CASE(match_4) {
    Element e("aa", 42);
    Action a = e.process("bbbbbaabb");

    BOOST_CHECK_EQUAL(a.token, 42);
    BOOST_CHECK_EQUAL(a.partial_match, false);
    BOOST_CHECK_EQUAL(a.silence_time, ns_t(0));
    BOOST_CHECK_EQUAL(a.consume_data, 5);
    BOOST_CHECK_EQUAL(a.consume_token, 2);
}

BOOST_AUTO_TEST_CASE(match_5) {
    Element e("aa", 42);
    Action a = e.process("bbbbbaabbaa");

    BOOST_CHECK_EQUAL(a.token, 42);
    BOOST_CHECK_EQUAL(a.partial_match, false);
    BOOST_CHECK_EQUAL(a.silence_time, ns_t(0));
    BOOST_CHECK_EQUAL(a.consume_data, 5);
    BOOST_CHECK_EQUAL(a.consume_token, 2);
}

BOOST_AUTO_TEST_CASE(no_match_timed_1) {
    Element e("aa", 42, ns_t(1000000));
    Action a = e.process("bbbbbaabb");

    BOOST_CHECK_EQUAL(a.token, 42);
    BOOST_CHECK_EQUAL(a.partial_match, false);
    BOOST_CHECK_EQUAL(a.silence_time, ns_t(1000000));
    BOOST_CHECK_EQUAL(a.consume_data, 9);
    BOOST_CHECK_EQUAL(a.consume_token, 0);
}

BOOST_AUTO_TEST_CASE(match_timed_1) {
    Element e("aa", 42, ns_t(1000000));
    Action a = e.process("bbbbbaabbaa");

    BOOST_CHECK_EQUAL(a.token, 42);
    BOOST_CHECK_EQUAL(a.partial_match, false);
    BOOST_CHECK_EQUAL(a.silence_time, ns_t(1000000));
    BOOST_CHECK_EQUAL(a.consume_data, 9);
    BOOST_CHECK_EQUAL(a.consume_token, 2);
}

}  // namespace KeyTokenizer
