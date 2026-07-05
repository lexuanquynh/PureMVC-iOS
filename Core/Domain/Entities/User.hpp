//
//  User.hpp
//  PureMVC Core — Domain entity
//

#ifndef PUREMVC_CORE_USER_HPP
#define PUREMVC_CORE_USER_HPP

#include <string>

namespace core {

struct User {
    std::string username;
    bool isVerified = false;
};

} // namespace core

#endif // PUREMVC_CORE_USER_HPP
