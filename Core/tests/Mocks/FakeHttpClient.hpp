//
//  FakeHttpClient.hpp
//  PureMVC Core tests
//
//  Records the request and replies synchronously with a canned response.
//

#ifndef PUREMVC_CORE_FAKE_HTTP_CLIENT_HPP
#define PUREMVC_CORE_FAKE_HTTP_CLIENT_HPP

#include "Infrastructure/Http/IHttpClient.hpp"

namespace core { namespace test {

class FakeHttpClient : public IHttpClient {
public:
    HttpResponse responseToReturn;
    int sendCallCount = 0;
    HttpRequest lastRequest;

    void send(const HttpRequest& request, Callback callback) override {
        ++sendCallCount;
        lastRequest = request;
        callback(responseToReturn);
    }
};

}} // namespace core::test

#endif // PUREMVC_CORE_FAKE_HTTP_CLIENT_HPP
