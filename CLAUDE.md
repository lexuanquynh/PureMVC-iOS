# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Contribution workflow (required)

For any task that changes the repo, follow this sequence — **do not push straight to `main`**:

1. **Create a GitHub issue** describing the task before starting work (`gh issue create`).
2. **Work on a branch** — never commit directly to `main`.
3. **Run the relevant tests and only continue once they pass** (see the playbook below).
4. **Open a pull request** (`gh pr create`) linked to the issue (`Closes #<n>`) and confirm **CI is green**. Do **not** merge yet.
5. **Merge only after the maintainer's explicit approval.** Merging is a separate, approved step — never merge without an OK.

### Add-a-feature playbook

This repo is a **reusable cross-platform base**: business logic lives once in the C++
`Core`; each platform adds only UI + a thin bridge + platform adapters. Add a feature
in this order so it stays shared and testable.

1. **Write the issue** — what changes, which layer, how it's verified. Branch off `main`.
2. **Put logic in the shared C++ `Core`** (the default home for anything non-UI):
   - New behavior → a use case in `Core/Domain/UseCases`, depending only on **ports**
     (interfaces in `Core/Domain/Ports`). Keep it platform-free (no Obj-C/JNI/UIKit).
   - Need the outside world (network, storage, clock…)? Depend on a **port**; put the
     concrete impl in `Core/Infrastructure`. Add a new port if none fits.
   - **Test on the host first** with fakes (`Core/tests/Mocks`) — the fast loop that
     covers *both* platforms at once:
     `cd Core && cmake -S . -B build && cmake --build build && ctest --test-dir build`
3. **Expose it through each bridge** (only the surface the UI needs):
   - iOS: a pure-Obj-C class in `Bridge/include/PureMVCBridge/` + its `.mm`
     (see `PMVCAuthClient`). Keep public headers Obj-C so Swift imports them.
   - Android: a JNI function in `android/app/src/main/cpp/jni_bridge.cpp` + a Kotlin
     wrapper (see `AndroidAuthClient`). Marshal callbacks to the main thread.
4. **Platform adapters** for any new port: iOS (`Bridge/`) and Android
   (`android/.../cpp` + Kotlin). See `KeychainSecureStore` / `JniSecureStorage`.
5. **Wire the UI**: iOS `PureMVC/` (Swift), Android `android/app/.../*.kt` (Compose).
   Prefer **mock data** for demos (`MockHttpClient`) so flows run offline.
6. **Verify the platforms you touched**:
   - iOS: `swift build && swift test`; app: `xcodebuild -project PureMVC.xcodeproj
     -scheme PureMVC -destination 'platform=iOS Simulator,name=iPhone 17 Pro' build`
     (use an **arm64** simulator — no x86_64 slice).
   - Android: `cd android && ./gradlew :app:assembleDebug` (run
     `openssl/build-openssl.sh` once first); runtime: `:app:connectedDebugAndroidTest`
     on an emulator.
7. **PR** linked to the issue; wait for **CI green** (host / iOS / Android) and approval; merge.

### Cutting a release

After a milestone is merged to `main` and CI is green:

```sh
git checkout main && git pull
gh release create vX.Y.Z --title "vX.Y.Z" --generate-notes
```

Use semver; tag from `main` only. Downstream projects consume a tagged version.

### Using this repo as a base for a new project

Start from `main` (or a release tag): keep `Core/` + `Bridge/` + the JNI bridge; replace
the demo use cases/UI with your own; point the clients at your backend
(`AppAuthClient.mm` / `jni_bridge.cpp`) and add SPKI pins; keep the workflow above.

## What this is

An iOS demo app that drives its application logic through the **PureMVC C++ multicore framework** rather than Swift. The Swift/UIKit layer is a thin shell; the real MVC (Model / Command / Notification) lives in C++ and Objective-C++, exposed to Swift through a bridging wrapper. The sample flow is login / logout / data-refresh against a REST backend.

## Build & test

There is no shared xcscheme checked in; the working scheme is `PureMVC`. Use an iOS **simulator** destination (the app compiles a C++ HTTP stack that links per-arch static libs from the bundled xcframework).

```sh
# Build (simulator)
xcodebuild -project PureMVC.xcodeproj -scheme PureMVC \
  -destination 'platform=iOS Simulator,name=iPhone 15' build

# Run all tests
xcodebuild -project PureMVC.xcodeproj -scheme PureMVC \
  -destination 'platform=iOS Simulator,name=iPhone 15' test

# Run a single test
xcodebuild -project PureMVC.xcodeproj -scheme PureMVC \
  -destination 'platform=iOS Simulator,name=iPhone 15' test \
  -only-testing:PureMVCTests/PureMVCTests/testExample
```

Normal workflow is opening `PureMVC.xcodeproj` in Xcode and running on a simulator.

### Required build settings (already configured; preserve when touching project.pbxproj)
The app target must compile C++ with the framework's expectations. If you add a new C++/ObjC++ target or the settings get lost, restore:
- `CLANG_CXX_LANGUAGE_STANDARD = gnu++0x` (C++11) plus `-std=c++11` in `OTHER_CPLUSPLUSFLAGS`
- `HEADER_SEARCH_PATHS = $(PROJECT_DIR)/PureMVC/Libs/include/**`
- `SWIFT_OBJC_BRIDGING_HEADER = PureMVC/PureMVC-Bridging-Header.h`

## Architecture: the Swift ↔ C++ bridge

The layers, from UI down to the network:

1. **Swift UI** (`ViewController.swift`, `AppDelegate.swift`) — talks only to `PureMVCWrapper`. `AppDelegate` calls `initializeFacade()` once at launch. `ViewController` sets itself as the wrapper's `delegate` and calls `onLoginButtonPressed:` / `onLogoutButtonPressed` / `onDataRefreshRequested`.

2. **`PureMVCWrapper`** (`Wrapper/PureMVCWrapper.{h,mm}`) — the single Objective-C++ boundary between Swift and C++. `.h` is pure Objective-C (safe for the Swift bridging header); `.mm` includes the C++ framework. Responsibilities:
   - Singleton (`sharedInstance`).
   - `initializeFacade` wires the PureMVC graph: registers commands (`LoginCommand`, `LogoutCommand`, `DataRefreshCommand`) against notification names, and registers `WrapperObserver`s.
   - Converts Swift calls into `facade.sendNotification(...)`, passing bodies as `NSDictionary` bridged to `void*`.
   - `WrapperObserver` (a C++ `IObserver`) receives C++ notifications and calls back into Swift via the `PureMVCDelegate` protocol, **always dispatched to the main queue**.

3. **Commands** (`Command/`) — C++ `SimpleCommand` subclasses run by the PureMVC controller when a notification fires. `LoginCommand` unbridges the `NSDictionary` body and calls `AppSharedAuthClient()` (`Command/AppAuthClient.{h,mm}` — a shared `PMVCAuthClient`); on the completion (main queue) it sends `LOGIN_SUCCESS` / `LOGIN_FAILED`. `LogoutCommand` (inline in `PureMVCWrapper.mm`) calls the same client's `logout` (clears the Keychain) then sends `LOGOUT_SUCCESS`. `DataRefreshCommand` is a stub.

4. **Auth / networking = the `Core` Swift Package** — login/logout/token storage live in the reusable headless C++ package (see `Core/README.md`), consumed via the `PureMVCBridge` product. `PMVCAuthClient` composes `HttplibHttpClient` (TLS verification + SPKI pinning, OpenSSL via SPM) + `AuthRepository` + `LoginUseCase` + a Keychain-backed `SecureTokenStore`. The old in-app `NetworkManager` / `UserProxy` / `UserTokenProvider` (which used `verifySSL=false`) have been removed. `PureMVC/Model`, `PureMVC/Network`, `PureMVC/Helper` no longer exist.

### Notification names — keep three places in sync
The notification/command string constants are the contract across the whole stack:
- `Constants/PureMVCConstants.h` — C++ `std::string` constants (source of truth), used inside commands/wrapper.
- `Wrapper/PureMVCConstantsWrapper.{h,mm}` — `PureMVCNotifications` ObjC class exposing the same strings to Swift as `NSString`.
- Swift `switch` in `ViewController.onNotificationReceived` matches against `PureMVCNotifications.*`.

Adding or renaming a notification means editing all three. Commands are wired to these names in `initializeFacade`.

## Conventions & gotchas

- **`.h` vs `.mm`**: any header reachable from the Swift bridging header (`PureMVCWrapper.h`, `PureMVCConstantsWrapper.h`) must stay pure Objective-C / C — no C++ includes. C++ belongs in `.mm`/`.cpp` and in headers only included from those.
- **Threading**: async callbacks arrive on background threads; anything touching UIKit or the `PureMVCDelegate` must be on the main queue (`PMVCAuthClient` already delivers its completion there; see also `WrapperObserver`).
- **Memory**: C++ objects created in `initializeFacade` (commands, observers) are `new`ed and owned by static storage; observers are cleaned up in `PureMVCWrapper.dealloc`. Bodies passed as `(__bridge void*)` must outlive the (synchronous) `sendNotification` call.
- **`Libs/`** (`PureMVC.xcframework`, `httplib.h`, `json.hpp`) is vendored third-party code — treat as read-only; the framework is built separately (see `README.md`). Note the `Core` package vendors its own copies of httplib/json under `Core/ThirdParty/`.
- **OpenSSL** comes from the `krzyzanowskim/OpenSSL-Package` SPM dependency (used by the app target and the `Core` package). The simulator has only an `arm64` framework slice, so build/run on an **arm64 simulator** (Apple Silicon), not `x86_64`.

## Backend

`PMVCAuthClient` targets a host configured in `Command/AppAuthClient.mm` (`initWithHost:port:pinnedSPKIHashes:`, currently a placeholder). The login path is `/api/v1/auth/login` (set in `Core`'s `AuthRepository`). Add real SPKI pins in `AppAuthClient.mm` when pointing at a real backend.
