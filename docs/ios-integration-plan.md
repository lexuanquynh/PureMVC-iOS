# iOS integration plan — wiring the `Core` module into the app

Status: **proposal, for review** (issue #14). No `project.pbxproj` or code changes
happen until this is approved. Each slice below becomes its own issue → branch →
tests → PR → approved merge, per the workflow in `CLAUDE.md`.

## Goal

Make the iOS app use the tested, headless `Core` module for authentication:

```
LoginCommand (ObjC++)  →  LoginUseCase  →  AuthRepository  →  HttplibHttpClient (TLS + pinning)
                                        →  SecureTokenStore →  KeychainSecureStore (Keychain)
```

…replacing the current `NetworkManager` / `UserTokenProvider` login path and the
insecure `verifySSL = false` configuration.

---

## ⚠️ Decision point 0 — OpenSSL for iOS (blocker)

`PureMVC/Network/NetworkManager.h` already does `#define CPPHTTPLIB_OPENSSL_SUPPORT`
and uses `httplib::SSLClient`, **but there is no OpenSSL anywhere in the repo**
(`OTHER_LDFLAGS` is empty, no `libssl`/`libcrypto`, no OpenSSL xcframework). So
either the app does not currently link/exercise the TLS path, or OpenSSL is
provided outside version control.

TLS verification + certificate pinning **cannot work without OpenSSL for iOS.**
We must pick one before integration:

| Option | What | Trade-off |
|---|---|---|
| **A. Prebuilt OpenSSL xcframework** (recommended) | Add an `OpenSSL.xcframework` (e.g. `krzyzanowskim/OpenSSL` via SwiftPM, or `OpenSSL-Universal`) | Fast, maintained; adds a dependency |
| B. Build OpenSSL from source for iOS | Own build scripts | Full control; heavy to set up/maintain |
| C. Drop OpenSSL, use httplib without SSL | Not acceptable | No HTTPS — rejected |

**Needed from you:** confirm Option A (and which package), or state how OpenSSL is
already provided. Everything below assumes an `OpenSSL.xcframework` is available to
the app target.

---

## Current build settings (app target, both configs)

- `CLANG_CXX_LANGUAGE_STANDARD = gnu++0x` (C++11); `OTHER_CPLUSPLUSFLAGS` has
  `-std=c++11 -frtti`
- `HEADER_SEARCH_PATHS = $(PROJECT_DIR)/PureMVC/Libs/include/**`
- `LIBRARY_SEARCH_PATHS` includes `$(PROJECT_DIR)/PureMVC/Libs`
- `OTHER_LDFLAGS = ""`, no `GCC_PREPROCESSOR_DEFINITIONS` for the app target
- `SWIFT_OBJC_BRIDGING_HEADER = PureMVC/PureMVC-Bridging-Header.h`

`Core` is written to be C++11-valid, so `gnu++0x` is fine — but this must be
verified when it first compiles in the app target.

---

## Slice 1 — make `Core` compile in the app target (no behavior change)

**Add sources** to the app target's *Compile Sources* build phase:
- `Core/Domain/UseCases/LoginUseCase.cpp`
- `Core/Infrastructure/Auth/AuthRepository.cpp`
- `Core/Infrastructure/Http/HttplibHttpClient.cpp`
- `Core/Infrastructure/Security/SecureTokenStore.cpp`
- `Core/Infrastructure/Security/KeychainSecureStore.mm`
(headers resolve via search paths)

**Build settings (Debug + Release):**
- `HEADER_SEARCH_PATHS`: add
  - `$(PROJECT_DIR)/Core` — Core uses root-relative includes (`Domain/...`, `Infrastructure/...`)
  - `$(PROJECT_DIR)/PureMVC/Libs` — so Core's `#include <httplib.h>` resolves (httplib.h lives in `Libs/`, 0.21.0 — matches the host build)
- `GCC_PREPROCESSOR_DEFINITIONS`: add `CPPHTTPLIB_OPENSSL_SUPPORT` **project-wide**
  so Core's `HttplibHttpClient.cpp` compiles the same SSL path as `NetworkManager`
  (avoids one TU seeing SSL and another not)
- Link `OpenSSL.xcframework` (from Decision 0) to the app target
- Add the **Keychain Sharing** capability only if cross-app sharing is needed;
  basic per-app Keychain (what `KeychainSecureStore` uses) needs no entitlement on
  iOS. On device, normal code signing/provisioning applies.

**nlohmann/json path.** Core includes `<nlohmann/json.hpp>`, but the vendored
header is `PureMVC/Libs/json/json.hpp` (no `nlohmann/` prefix). Fix by placing the
single header at `PureMVC/Libs/nlohmann/json.hpp` (copy of the vendored one) so
`<nlohmann/json.hpp>` resolves via the `Libs` search path added above. Keeps the
include identical to the host build.

**Verification:** app builds for the simulator with Core linked but not yet used
(reference one symbol, e.g. construct a `core::HttpClientConfig`, to force linkage).
No functional change. `xcodebuild ... build`.

## Slice 2 — wire login through Core (behavior-preserving)

Build the object graph once (e.g. a small ObjC++ composition owned by
`PureMVCWrapper` or `UserProxy`), with lifetimes that outlive async calls:
- a process-lifetime `core::ThreadExecutor` (static/singleton)
- `HttplibHttpClient client(config, executor)` where `config` = host/443,
  `useSSL=true`, `verifySSL=true`, pins empty *for now*
- `AuthRepository repo(client, "/api/v1/auth/login")`
- `KeychainSecureStore keychain; SecureTokenStore store(keychain);`
- `LoginUseCase login(repo, store)`

Rewrite `LoginCommand::execute` (ObjC++): read `username`/`password` from the
notification body, build a `core::LoginCredentials`, call `login.execute(creds, cb)`;
in `cb` send `LOGIN_SUCCESS` / `LOGIN_FAILED` (message from `DomainError`) on the
main queue. The ObjC↔C++ conversion stays in this bridge layer; the domain stays
clean. `logout()` clears via `SecureTokenStore::clear()`.

**Verification:** run on simulator, tap login, observe the same notifications as
today (success + failure). Keep `NetworkManager` in place until Slice 3.

## Slice 3 — remove the legacy insecure path + enable pinning

- Delete the `NetworkManager` / `UserTokenProvider` login usage from `UserProxy`
  (or the whole files if nothing else uses them), removing `verifySSL = false` and
  the hard-coded, inconsistent base URLs.
- Add real SPKI pins to `config.pinnedSpkiSha256Base64`. Obtain a pin with:
  ```sh
  openssl s_client -connect HOST:443 -servername HOST </dev/null 2>/dev/null \
    | openssl x509 -pubkey -noout \
    | openssl pkey -pubin -outform der \
    | openssl dgst -sha256 -binary \
    | openssl enc -base64
  ```
  Include a **current + backup** pin (the pinner accepts any match) to survive key
  rotation.

**Verification (on simulator/device):**
- Correct pin → login succeeds.
- Deliberately wrong pin → request fails (pinning rejects) — proves pinning is live.
- Token round-trips through the Keychain across app launches.

## Slice 4 — CI (optional, recommended)

GitHub Actions: (1) `cmake`/`ctest` for `Core` (fast, already green), (2)
`xcodebuild ... test` for the app on a simulator. Gate PRs on both.

---

## Risks & notes

- **OpenSSL for iOS is the critical prerequisite** (Decision 0). Nothing TLS works
  without it.
- **`project.pbxproj` edits are error-prone.** Prefer doing the file-adds and
  build-setting changes in the Xcode UI (or a careful scripted edit) and committing
  the result, rather than hand-editing the pbxproj blind.
- **C++ standard.** Core must compile at `gnu++11` in the app target; verify at
  Slice 1. If any C++14+ slips in, either fix it or raise the app target's C++
  standard.
- **httplib version.** App vendored header is 0.21.0, matching the host build — the
  pinning API (`set_server_certificate_verifier`, `SSLVerifierResponse`) is present.
- **Keychain on simulator** works without special provisioning; device builds need
  the usual signing.
- **Executor lifetime.** The `ThreadExecutor` must not be destroyed while a request
  is in flight — keep it process-lifetime.

## Suggested issue breakdown

1. OpenSSL.xcframework for the app target (Decision 0)
2. Slice 1 — Core compiles in the app target
3. Slice 2 — login routed through Core
4. Slice 3 — remove legacy path + enable pinning
5. Slice 4 — CI (optional)
