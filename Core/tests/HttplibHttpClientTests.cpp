//
//  HttplibHttpClientTests.cpp
//  PureMVC Core tests
//
//  End-to-end tests against a real httplib server on 127.0.0.1 (ephemeral port).
//  Fully local: no simulator, no external network. Plain HTTP (no SSL).
//

#include <gtest/gtest.h>

#include <chrono>
#include <future>
#include <thread>

#include <httplib.h>

#include "Infrastructure/Http/HttplibHttpClient.hpp"
#include "Infrastructure/Concurrency/ThreadExecutor.hpp"
#include "Mocks/SyncExecutor.hpp"

using namespace core;

namespace {

HttpResponse sendSync(HttplibHttpClient& client, const HttpRequest& request) {
    HttpResponse captured;
    client.send(request, [&captured](const HttpResponse& response) {
        captured = response;
    });
    return captured; // SyncExecutor guarantees the callback ran inline
}

} // namespace

class HttplibHttpClientTest : public ::testing::Test {
protected:
    httplib::Server server;
    std::thread serverThread;
    int port = 0;

    void SetUp() override {
        server.Post("/echo", [](const httplib::Request& req, httplib::Response& res) {
            res.status = 200;
            res.set_content(req.body, "application/json");
            res.set_header("X-Seen-Body-Length", std::to_string(req.body.size()));
        });
        server.Get("/missing", [](const httplib::Request&, httplib::Response& res) {
            res.status = 404;
            res.set_content(R"({"message":"nope"})", "application/json");
        });
        // Reflects a request header back so tests can assert header propagation.
        server.Get("/whoami", [](const httplib::Request& req, httplib::Response& res) {
            res.status = 200;
            res.set_content(req.get_header_value("X-App"), "text/plain");
        });

        port = server.bind_to_any_port("127.0.0.1");
        serverThread = std::thread([this]() { server.listen_after_bind(); });
        server.wait_until_ready();
    }

    void TearDown() override {
        server.stop();
        if (serverThread.joinable()) {
            serverThread.join();
        }
    }

    HttpClientConfig config() const {
        HttpClientConfig c;
        c.host = "127.0.0.1";
        c.port = port;
        c.useSSL = false;
        return c;
    }
};

TEST_F(HttplibHttpClientTest, PostMapsStatusBodyAndHeaders) {
    test::SyncExecutor executor;
    HttplibHttpClient client(config(), executor);

    HttpRequest request;
    request.method = "POST";
    request.path = "/echo";
    request.body = R"({"hello":"world"})";
    request.contentType = "application/json";

    HttpResponse response = sendSync(client, request);

    EXPECT_EQ(executor.runCount, 1);
    EXPECT_TRUE(response.ok());
    EXPECT_EQ(response.status, 200);
    EXPECT_EQ(response.body, R"({"hello":"world"})");
    EXPECT_EQ(response.headers["X-Seen-Body-Length"],
              std::to_string(request.body.size()));
}

TEST_F(HttplibHttpClientTest, NonSuccessStatusAndBodyMapped) {
    test::SyncExecutor executor;
    HttplibHttpClient client(config(), executor);

    HttpRequest request;
    request.method = "GET";
    request.path = "/missing";

    HttpResponse response = sendSync(client, request);

    EXPECT_FALSE(response.ok());
    EXPECT_EQ(response.status, 404);
    EXPECT_NE(response.body.find("nope"), std::string::npos);
    EXPECT_FALSE(response.transportError);
}

TEST_F(HttplibHttpClientTest, DefaultAndRequestHeadersAreSent) {
    test::SyncExecutor executor;
    HttpClientConfig c = config();
    c.defaultHeaders["X-App"] = "from-default";
    HttplibHttpClient client(c, executor);

    HttpRequest request;
    request.method = "GET";
    request.path = "/whoami";

    HttpResponse fromDefault = sendSync(client, request);
    EXPECT_EQ(fromDefault.body, "from-default");

    // Per-request header overrides the default.
    request.headers["X-App"] = "from-request";
    HttpResponse overridden = sendSync(client, request);
    EXPECT_EQ(overridden.body, "from-request");
}

TEST_F(HttplibHttpClientTest, UnreachableServerYieldsTransportError) {
    // Bind a throwaway server to reserve a port, but never accept: the port is
    // listening at the kernel level (so connects into the backlog) yet nothing
    // ever replies. Short timeouts turn this into a fast transport failure.
    httplib::Server dead;
    int deadPort = dead.bind_to_any_port("127.0.0.1");

    test::SyncExecutor executor;
    HttpClientConfig c = config();
    c.port = deadPort;
    c.connectionTimeoutSec = 1;
    c.readTimeoutSec = 1;
    HttplibHttpClient client(c, executor);

    HttpRequest request;
    request.method = "GET";
    request.path = "/missing";

    HttpResponse response = sendSync(client, request);

    EXPECT_TRUE(response.transportError);
    EXPECT_EQ(response.status, 0);
    EXPECT_FALSE(response.ok());
}

TEST_F(HttplibHttpClientTest, RunsThroughThreadExecutorAsynchronously) {
    ThreadExecutor executor;
    HttplibHttpClient client(config(), executor);

    HttpRequest request;
    request.method = "POST";
    request.path = "/echo";
    request.body = "async-body";

    std::promise<HttpResponse> promise;
    std::future<HttpResponse> future = promise.get_future();
    client.send(request, [&promise](const HttpResponse& response) {
        promise.set_value(response);
    });

    ASSERT_EQ(future.wait_for(std::chrono::seconds(3)), std::future_status::ready);
    HttpResponse response = future.get();
    EXPECT_TRUE(response.ok());
    EXPECT_EQ(response.body, "async-body");
}
