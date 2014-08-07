#define BOOST_TEST_MODULE MyTest
#include <boost/test/unit_test.hpp>

#include "key-tokenizer/action.hxx"
typedef std::chrono::nanoseconds ns_t;

namespace std {
std::ostream& operator<<(std::ostream& o, ns_t t) {
    o << t.count();
    return o;
}
}  // namespace std

namespace KeyTokenizer {

BOOST_AUTO_TEST_CASE(consume_data_min) {
    Action a(1, ns_t(0), false, 3, 0);
    Action b(2, ns_t(0), false, 6, 0);

    Action c = a % b;

    BOOST_CHECK_EQUAL(c.token, 0);
    BOOST_CHECK_EQUAL(c.partial_match, false);
    BOOST_CHECK_EQUAL(c.silence_time, ns_t(0));
    BOOST_CHECK_EQUAL(c.consume_data, 3);
    BOOST_CHECK_EQUAL(c.consume_token, 0);
}

BOOST_AUTO_TEST_CASE(partial_match_and_no_match) {
    Action a(1, ns_t(0), true, 3, 0);
    Action b(2, ns_t(0), false, 6, 0);

    Action c = a % b;

    BOOST_CHECK_EQUAL(c.token, 0);
    BOOST_CHECK_EQUAL(c.partial_match, true);
    BOOST_CHECK_EQUAL(c.silence_time, ns_t(0));
    BOOST_CHECK_EQUAL(c.consume_data, 3);
    BOOST_CHECK_EQUAL(c.consume_token, 0);
}

BOOST_AUTO_TEST_CASE(partial_match_and_match) {
    Action a(1, ns_t(0), true, 3, 0);
    Action b(2, ns_t(0), false, 1, 5);

    Action c = a % b;

    BOOST_CHECK_EQUAL(c.token, 0);
    BOOST_CHECK_EQUAL(c.partial_match, true);
    BOOST_CHECK_EQUAL(c.silence_time, ns_t(0));
    BOOST_CHECK_EQUAL(c.consume_data, 1);
    BOOST_CHECK_EQUAL(c.consume_token, 0);
}

BOOST_AUTO_TEST_CASE(match_and_no_match) {
    Action a(1, ns_t(0), false, 10, 0);
    Action b(2, ns_t(0), false, 2, 5);

    Action c = a % b;

    BOOST_CHECK_EQUAL(c.token, 2);
    BOOST_CHECK_EQUAL(c.partial_match, false);
    BOOST_CHECK_EQUAL(c.silence_time, ns_t(0));
    BOOST_CHECK_EQUAL(c.consume_data, 2);
    BOOST_CHECK_EQUAL(c.consume_token, 5);
}

BOOST_AUTO_TEST_CASE(no_match_and_match) {
    Action a(1, ns_t(0), false, 10, 0);
    Action b(2, ns_t(0), false, 2, 5);

    Action c = b % a;

    BOOST_CHECK_EQUAL(c.token, 2);
    BOOST_CHECK_EQUAL(c.partial_match, false);
    BOOST_CHECK_EQUAL(c.silence_time, ns_t(0));
    BOOST_CHECK_EQUAL(c.consume_data, 2);
    BOOST_CHECK_EQUAL(c.consume_token, 5);
}

BOOST_AUTO_TEST_CASE(two_match_different_start) {
    Action a(1, ns_t(0), false, 3, 100);
    Action b(2, ns_t(0), false, 2, 5);

    Action c = a % b;

    BOOST_CHECK_EQUAL(c.token, 2);
    BOOST_CHECK_EQUAL(c.partial_match, false);
    BOOST_CHECK_EQUAL(c.silence_time, ns_t(0));
    BOOST_CHECK_EQUAL(c.consume_data, 2);
    BOOST_CHECK_EQUAL(c.consume_token, 5);
}

BOOST_AUTO_TEST_CASE(two_match_different_length) {
    Action a(1, ns_t(0), false, 2, 100);
    Action b(2, ns_t(0), false, 2, 5);

    Action c = a % b;

    BOOST_CHECK_EQUAL(c.token, 1);
    BOOST_CHECK_EQUAL(c.partial_match, false);
    BOOST_CHECK_EQUAL(c.silence_time, ns_t(0));
    BOOST_CHECK_EQUAL(c.consume_data, 2);
    BOOST_CHECK_EQUAL(c.consume_token, 100);
}


BOOST_AUTO_TEST_CASE(timmed_complete_beat_partial) {
    Action a(1, ns_t(1), false, 2, 1);
    Action b(2, ns_t(0), true, 1, 0);

    Action c = a % b;

    BOOST_CHECK_EQUAL(c.token, 1);
    BOOST_CHECK_EQUAL(c.partial_match, false);
    BOOST_CHECK_EQUAL(c.silence_time, ns_t(1));
    BOOST_CHECK_EQUAL(c.consume_data, 2);
    BOOST_CHECK_EQUAL(c.consume_token, 1);
}

BOOST_AUTO_TEST_CASE(timmed_vs_complete_1) {
    Action a(1, ns_t(1), false, 1, 10);
    Action b(2, ns_t(0), false, 2, 20);

    Action c = a % b;

    BOOST_CHECK_EQUAL(c.token, 1);
    BOOST_CHECK_EQUAL(c.partial_match, false);
    BOOST_CHECK_EQUAL(c.silence_time, ns_t(1));
    BOOST_CHECK_EQUAL(c.consume_data, 1);
    BOOST_CHECK_EQUAL(c.consume_token, 10);
}

BOOST_AUTO_TEST_CASE(timmed_vs_complete_2) {
    Action a(1, ns_t(1), false, 5, 10);
    Action b(2, ns_t(0), false, 2, 20);

    Action c = a % b;

    BOOST_CHECK_EQUAL(c.token, 2);
    BOOST_CHECK_EQUAL(c.partial_match, false);
    BOOST_CHECK_EQUAL(c.silence_time, ns_t(0));
    BOOST_CHECK_EQUAL(c.consume_data, 2);
    BOOST_CHECK_EQUAL(c.consume_token, 20);
}

}  // namespace KeyTokenizer
