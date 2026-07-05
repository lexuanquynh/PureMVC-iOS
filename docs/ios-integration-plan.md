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

## ✅ Decision point 0 — OpenSSL for iOS (RESOLVED)

OpenSSL is **already provided via Swift Package Manager**: the app depends on
[`krzyzanowskim/OpenSSL-Package`](https://github.com/krzyzanowskim/OpenSSL-Package)
`3.3.3001` (see `Package.resolved`), and the `OpenSSL` product is linked into the
app target's *Frameworks* build phase (`project.pbxproj`). `NetworkManager` already
compiles the `httplib::SSLClient` path (`CPPHTTPLIB_OPENSSL_SUPPORT`) against it.

So there is **no OpenSSL prerequisite** — Core's `HttplibHttpClient.cpp` will resolve
`<openssl/...>` and link the same way `NetworkManager.cpp` does today. This removes
the biggest integration risk.

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

## Approach: local Swift Package (chosen)

Instead of adding loose `Core/*.cpp` files to the app target, `Core` is packaged
as a **local Swift Package** (`Package.swift` at the repo root). This gives a hard,
compiler-enforced module boundary and makes the core reusable across projects — the
app depends on the package rather than reaching into its files. CMake + GoogleTest
stay as the fast pure-C++ unit-test loop; SPM is the consumption path.

## Slice 1 — package Core as a Swift Package ✅ (done, no behavior change)

- Vendor httplib + nlohmann/json single headers under `Core/ThirdParty/` so the
  package is self-contained (SPM has no FetchContent equivalent).
- `Package.swift` → C++ library target `PureMVCCoreCxx` (sources: `Domain`,
  `Infrastructure`), depending on `krzyzanowskim/OpenSSL-Package`, with
  `CPPHTTPLIB_OPENSSL_SUPPORT` and header search paths for the target root +
  `ThirdParty`. The ObjC++ bridge (`KeychainSecureStore.mm`) is excluded here.
- **Verification:** `swift build` compiles all four C++ sources against OpenSSL on
  the host; `ctest` in `Core/build` stays green (31 tests).

## Slice 1b — ObjC++ bridge target (next)

Add a second package target `PureMVCBridge` (Objective-C++) that depends on
`PureMVCCoreCxx` and contains `KeychainSecureStore.mm` plus the Swift-facing
wrapper. Its public headers stay pure Objective-C so Swift can import it. This is
the package's public product the app links against.

## Slice 2 — wire login through Core (behavior-preserving)

Add the package to the app (one **local package dependency** in Xcode — far smaller
than per-file `project.pbxproj` edits) and link `PureMVCBridge`.

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

- **OpenSSL is already available** via SPM (Decision 0), so the previous top risk
  is gone; the remaining risk is mostly Xcode project wiring.
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

1. Slice 1 — package Core as a Swift Package (`PureMVCCoreCxx`) ✅
2. Slice 1b — ObjC++ bridge target (`PureMVCBridge`)
3. Slice 2 — app consumes the package; login routed through Core
4. Slice 3 — remove legacy path + enable pinning
5. Slice 4 — CI (optional)
