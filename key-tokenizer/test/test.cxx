#define BOOST_TEST_MODULE MyTest
#include <boost/test/unit_test.hpp>

#include "key-tokenizer.hxx"

void token_cb(uint32_t t) {
}

void data_cb(const char *c, size_t s) {
}

BOOST_AUTO_TEST_CASE(my_test) {
    boost::asio::io_service io;
    KeyTokenizer key_tokenizer(io, token_cb, data_cb);

}
