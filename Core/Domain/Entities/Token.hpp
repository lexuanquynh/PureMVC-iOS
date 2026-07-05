//
//  Token.hpp
//  PureMVC Core — Domain entity
//
//  A pair of auth tokens. Pure C++, no platform dependency.
//

#ifndef PUREMVC_CORE_TOKEN_HPP
#define PUREMVC_CORE_TOKEN_HPP

#include <string>

namespace core {

struct Token {
    std::string accessToken;
    std::string refreshToken;

    bool empty() const { return accessToken.empty(); }
};

} // namespace core

#endif // PUREMVC_CORE_TOKEN_HPP
