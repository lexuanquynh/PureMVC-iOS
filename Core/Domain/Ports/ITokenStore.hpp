//
//  ITokenStore.hpp
//  PureMVC Core — outbound port
//
//  Secure token persistence. The iOS bridge implements this over Keychain;
//  tests use an in-memory fake. The domain never sees the storage mechanism.
//

#ifndef PUREMVC_CORE_ITOKEN_STORE_HPP
#define PUREMVC_CORE_ITOKEN_STORE_HPP

#include "../Entities/Token.hpp"

namespace core {

class ITokenStore {
public:
    // Returns an empty Token (accessToken.empty() == true) when nothing stored.
    virtual Token load() = 0;
    virtual void save(const Token& token) = 0;
    virtual void clear() = 0;

    virtual ~ITokenStore() = default;
};

} // namespace core

#endif // PUREMVC_CORE_ITOKEN_STORE_HPP
