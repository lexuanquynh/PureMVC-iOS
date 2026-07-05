# PureMVC Core (C++)

Headless, platform-agnostic C++ core for the app. No iOS / UIKit / Texture /
Objective-C dependencies — so it builds and unit-tests on any host in
milliseconds, without a simulator, and can be reused as a base across projects.

## Layers

```
Domain/
  Entities/       value objects (User, Token)
  AuthTypes.hpp   shared domain types (LoginCredentials, AuthSession, DomainError)
  Ports/          interfaces the inner layers depend on
                  (IAuthRepository, ITokenStore, IExecutor)
  UseCases/       application business rules (LoginUseCase)
Infrastructure/
  Http/           HttpTypes, IHttpClient (hides httplib), HttpError mapping,
                  HttplibHttpClient (concrete client over cpp-httplib; TLS
                  verification + SPKI pinning under CPPHTTPLIB_OPENSSL_SUPPORT)
  Concurrency/    ThreadExecutor (IExecutor over std::thread)
  Auth/           AuthRepository (implements IAuthRepository via IHttpClient + JSON)
  Security/       ISecureStorage, SecureTokenStore (ITokenStore logic),
                  KeychainSecureStore (iOS Keychain adapter — .mm, iOS target only),
                  CertificatePinner, Base64 (pin policy + encoding)
tests/
  Mocks/          in-memory fakes (FakeAuthRepository, FakeTokenStore,
                  FakeHttpClient, SyncExecutor)
  *Tests.cpp      GoogleTest suites
```

Dependency rule: everything points **inward**. Use cases depend only on ports;
infrastructure (httplib) and the iOS bridge (Keychain, GCD) implement those
ports from the outside. Headers are kept C++11-compatible so the same sources
also compile into the iOS target (gnu++11).

## Build & test (host)

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

GoogleTest, nlohmann/json and cpp-httplib are fetched automatically via CMake
`FetchContent` (needs network on first configure). The `HttplibHttpClient` tests
run a real httplib server on `127.0.0.1` — still no simulator and no external
network.

## Status

- **#1** — `LoginUseCase`: input validation, delegation to `IAuthRepository`,
  token persistence via `ITokenStore`.
- **#4** — `IHttpClient` / `IExecutor` ports; `AuthRepository` builds the login
  request and maps success/HTTP-error/transport-error/malformed-JSON responses
  into domain types; `ThreadExecutor`.
- **#8** — `HttplibHttpClient`: concrete `IHttpClient` over cpp-httplib, running
  through an injected `IExecutor`; SSL guarded behind `CPPHTTPLIB_OPENSSL_SUPPORT`
  (iOS only). 19 host tests, including end-to-end against a local httplib server.
- **#10** — Keychain-backed token storage: `ISecureStorage` port,
  `SecureTokenStore` (host-tested logic), `KeychainSecureStore` (iOS Keychain
  adapter, `.mm`).
- **#12** — TLS hardening: secure-by-default `HttpClientConfig` (verification on),
  `CertificatePinner` + `base64Encode` (host-tested), and SPKI public-key pinning
  wired into the `HttplibHttpClient` SSL path via `set_server_certificate_verifier`
  (compiled against httplib 0.21.0 + OpenSSL). 31 host tests.

Next: wire `AuthRepository` + `HttplibHttpClient` + `SecureTokenStore` into
`UserProxy` / `LoginCommand` (removing the legacy `verifySSL=false` path); CI.
