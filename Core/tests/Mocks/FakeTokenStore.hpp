//
//  FakeTokenStore.hpp
//  PureMVC Core tests
//
//  In-memory ITokenStore that records how it was used.
//

#ifndef PUREMVC_CORE_FAKE_TOKEN_STORE_HPP
#define PUREMVC_CORE_FAKE_TOKEN_STORE_HPP

#include "Domain/Ports/ITokenStore.hpp"

namespace core { namespace test {

class FakeTokenStore : public ITokenStore {
public:
    Token stored;
    int saveCallCount = 0;
    int clearCallCount = 0;

    Token load() override { return stored; }

    void save(const Token& token) override {
        ++saveCallCount;
        stored = token;
    }

    void clear() override {
        ++clearCallCount;
        stored = Token{};
    }
};

}} // namespace core::test

#endif // PUREMVC_CORE_FAKE_TOKEN_STORE_HPP
