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
  Http/           HttpTypes, IHttpClient (hides httplib), HttpError mapping
  Concurrency/    ThreadExecutor (IExecutor over std::thread)
  Auth/           AuthRepository (implements IAuthRepository via IHttpClient + JSON)
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

GoogleTest and nlohmann/json are fetched automatically via CMake `FetchContent`
(needs network on first configure).

## Status

- **#1** — `LoginUseCase`: input validation, delegation to `IAuthRepository`,
  token persistence via `ITokenStore`.
- **#4** — `IHttpClient` / `IExecutor` ports; `AuthRepository` builds the login
  request and maps success/HTTP-error/transport-error/malformed-JSON responses
  into domain types; `ThreadExecutor`. 14 host tests, no simulator or real
  network.

Next: `HttplibHttpClient` (concrete httplib+OpenSSL binding, iOS target only);
Keychain-backed `ITokenStore` + SSL verify + cert pinning; wire `LoginCommand`
/ `UserProxy` to the use case; CI.
