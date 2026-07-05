# PureMVC Core (C++)

Headless, platform-agnostic C++ core for the app. No iOS / UIKit / Texture /
Objective-C dependencies — so it builds and unit-tests on any host in
milliseconds, without a simulator, and can be reused as a base across projects.

## Layers

```
Domain/
  Entities/     value objects (User, Token)
  AuthTypes.hpp shared domain types (LoginCredentials, AuthSession, DomainError)
  Ports/        interfaces the domain depends on (IAuthRepository, ITokenStore)
  UseCases/     application business rules (LoginUseCase)
tests/
  Mocks/        in-memory fakes implementing the ports
  *Tests.cpp    GoogleTest suites
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

GoogleTest is fetched automatically via CMake `FetchContent` (needs network on
first configure).

## Status

First vertical slice (issue #1): `LoginUseCase` — input validation, delegation
to `IAuthRepository`, token persistence via `ITokenStore` on success. Proves the
core is testable off-device.

Next: extract `IHttpClient` / `IExecutor` and back these ports with real
infrastructure over httplib; add a Keychain-backed `ITokenStore` in the iOS
bridge; wire `LoginCommand` to call the use case.
