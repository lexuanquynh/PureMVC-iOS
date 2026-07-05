//
//  HttplibHttpClient.hpp
//  PureMVC Core — Infrastructure
//
//  Concrete IHttpClient over cpp-httplib. httplib itself is an implementation
//  detail confined to the .cpp — nothing here leaks it, so callers and tests
//  depend only on the IHttpClient / HttpTypes abstractions.
//
//  send() runs the blocking httplib call on the injected IExecutor, so the
//  concurrency policy (thread-per-request, pool, GCD, or synchronous in tests)
//  is chosen from the outside.
//

#ifndef PUREMVC_CORE_HTTPLIB_HTTP_CLIENT_HPP
#define PUREMVC_CORE_HTTPLIB_HTTP_CLIENT_HPP

#include "Domain/Ports/IExecutor.hpp"
#include "Infrastructure/Http/HttpClientConfig.hpp"
#include "Infrastructure/Http/IHttpClient.hpp"

namespace core {

class HttplibHttpClient : public IHttpClient {
public:
    HttplibHttpClient(HttpClientConfig config, IExecutor& executor);

    void send(const HttpRequest& request, Callback callback) override;

private:
    HttpClientConfig config_;
    IExecutor& executor_;
};

} // namespace core

#endif // PUREMVC_CORE_HTTPLIB_HTTP_CLIENT_HPP
