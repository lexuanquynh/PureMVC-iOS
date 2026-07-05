//
//  IHttpClient.hpp
//  PureMVC Core — Infrastructure port
//
//  The seam that hides httplib (and its OpenSSL dependency) from everything
//  above it. Repositories depend on this; the concrete httplib-backed client
//  is compiled only into the iOS target, while tests inject a fake.
//

#ifndef PUREMVC_CORE_IHTTP_CLIENT_HPP
#define PUREMVC_CORE_IHTTP_CLIENT_HPP

#include <functional>
#include "HttpTypes.hpp"

namespace core {

class IHttpClient {
public:
    // Sends asynchronously; the callback may fire on any thread.
    using Callback = std::function<void(const HttpResponse& response)>;

    virtual void send(const HttpRequest& request, Callback callback) = 0;

    virtual ~IHttpClient() = default;
};

} // namespace core

#endif // PUREMVC_CORE_IHTTP_CLIENT_HPP
