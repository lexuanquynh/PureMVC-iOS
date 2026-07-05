//
//  Base64.hpp
//  PureMVC Core — Infrastructure
//
//  Standard RFC 4648 base64 encoding. Pure C++, header-only, so it is testable
//  on the host and usable from the (OpenSSL-guarded) pin computation.
//

#ifndef PUREMVC_CORE_BASE64_HPP
#define PUREMVC_CORE_BASE64_HPP

#include <cstddef>
#include <string>

namespace core {

inline std::string base64Encode(const unsigned char* data, std::size_t length) {
    static const char table[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    std::string out;
    out.reserve(((length + 2) / 3) * 4);

    std::size_t i = 0;
    while (i + 3 <= length) {
        unsigned n = (static_cast<unsigned>(data[i]) << 16) |
                     (static_cast<unsigned>(data[i + 1]) << 8) |
                     static_cast<unsigned>(data[i + 2]);
        out.push_back(table[(n >> 18) & 63]);
        out.push_back(table[(n >> 12) & 63]);
        out.push_back(table[(n >> 6) & 63]);
        out.push_back(table[n & 63]);
        i += 3;
    }

    const std::size_t remaining = length - i;
    if (remaining == 1) {
        unsigned n = static_cast<unsigned>(data[i]) << 16;
        out.push_back(table[(n >> 18) & 63]);
        out.push_back(table[(n >> 12) & 63]);
        out.push_back('=');
        out.push_back('=');
    } else if (remaining == 2) {
        unsigned n = (static_cast<unsigned>(data[i]) << 16) |
                     (static_cast<unsigned>(data[i + 1]) << 8);
        out.push_back(table[(n >> 18) & 63]);
        out.push_back(table[(n >> 12) & 63]);
        out.push_back(table[(n >> 6) & 63]);
        out.push_back('=');
    }

    return out;
}

} // namespace core

#endif // PUREMVC_CORE_BASE64_HPP
