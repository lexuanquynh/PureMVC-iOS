# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Contribution workflow (required)

For any task that changes the repo, follow this sequence — **do not push straight to `main`**:

1. **Create a GitHub issue** describing the task before starting work (`gh issue create`).
2. **Work on a branch** — never commit directly to `main`.
3. **Run the relevant tests and only continue once they pass** — C++ core: `ctest` in `Core/build`; iOS: `xcodebuild ... test`.
4. **Open a pull request** (`gh pr create`) linked to the issue (`Closes #<n>`). Do **not** merge yet.
5. **Merge only after the maintainer's explicit approval.** Merging is a separate, approved step — never merge without an OK.

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
