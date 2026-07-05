//
//  FakeSecureStore.hpp
//  PureMVC Core tests
//
//  In-memory ISecureStorage for exercising storage logic on the host.
//

#ifndef PUREMVC_CORE_FAKE_SECURE_STORE_HPP
#define PUREMVC_CORE_FAKE_SECURE_STORE_HPP

#include <map>
#include <string>
#include "Infrastructure/Security/ISecureStorage.hpp"

namespace core { namespace test {

class FakeSecureStore : public ISecureStorage {
public:
    std::map<std::string, std::string> data;
    int setCount = 0;
    int removeCount = 0;

    bool get(const std::string& key, std::string& out) override {
        auto it = data.find(key);
        if (it == data.end()) {
            return false;
        }
        out = it->second;
        return true;
    }

    bool set(const std::string& key, const std::string& value) override {
        ++setCount;
        data[key] = value;
        return true;
    }

    bool remove(const std::string& key) override {
        ++removeCount;
        data.erase(key);
        return true;
    }
};

}} // namespace core::test

#endif // PUREMVC_CORE_FAKE_SECURE_STORE_HPP
