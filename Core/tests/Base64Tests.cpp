//
//  Base64Tests.cpp
//  PureMVC Core tests
//

#include <gtest/gtest.h>

#include <string>

#include "Infrastructure/Security/Base64.hpp"

using namespace core;

namespace {

std::string encode(const std::string& s) {
    return base64Encode(reinterpret_cast<const unsigned char*>(s.data()), s.size());
}

} // namespace

// RFC 4648 test vectors.
TEST(Base64, Rfc4648Vectors) {
    EXPECT_EQ(encode(""), "");
    EXPECT_EQ(encode("f"), "Zg==");
    EXPECT_EQ(encode("fo"), "Zm8=");
    EXPECT_EQ(encode("foo"), "Zm9v");
    EXPECT_EQ(encode("foob"), "Zm9vYg==");
    EXPECT_EQ(encode("fooba"), "Zm9vYmE=");
    EXPECT_EQ(encode("foobar"), "Zm9vYmFy");
}

TEST(Base64, EncodesBinaryBytes) {
    const unsigned char bytes[] = {0x00, 0xFF, 0x10, 0x7F, 0x80};
    EXPECT_EQ(base64Encode(bytes, sizeof(bytes)), "AP8Qf4A=");
}
