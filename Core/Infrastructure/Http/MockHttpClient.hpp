//
//  MockHttpClient.hpp
//  PureMVC Core — Infrastructure
//
//  An IHttpClient that returns a canned successful login response (asynchronously,
//  via the injected executor). Lets the apps demo the full auth pipeline offline,
//  with no real backend. Header-only so both iOS (SPM) and Android (NDK) can use it.
//

#ifndef PUREMVC_CORE_MOCK_HTTP_CLIENT_HPP
#define PUREMVC_CORE_MOCK_HTTP_CLIENT_HPP

#include "Domain/Ports/IExecutor.hpp"
#include "Infrastructure/Http/IHttpClient.hpp"

namespace core {

class MockHttpClient : public IHttpClient {
public:
    explicit MockHttpClient(IExecutor& executor) : executor_(executor) {}

    void send(const HttpRequest& /*request*/, Callback callback) override {
        executor_.run([callback]() {
            HttpResponse response;
            response.status = 200;
            response.body =
                R"({"access_token":"mock-access-token",)"
                R"("refresh_token":"mock-refresh-token",)"
                R"("is_verify":true})";
            callback(response);
        });
    }

private:
    IExecutor& executor_;
};

} // namespace core

#endif // PUREMVC_CORE_MOCK_HTTP_CLIENT_HPP
