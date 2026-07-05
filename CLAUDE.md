# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

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
   - `initializeFacade` wires the whole PureMVC graph: registers `UserProxy`, registers commands (`LoginCommand`, `LogoutCommand`, `DataRefreshCommand`) against notification names, and registers `WrapperObserver`s.
   - Converts Swift calls into `facade.sendNotification(...)`, passing bodies as `NSDictionary` bridged to `void*`.
   - `WrapperObserver` (a C++ `IObserver`) receives C++ notifications and calls back into Swift via the `PureMVCDelegate` protocol, **always dispatched to the main queue**.

3. **Commands** (`Command/`) — C++ `SimpleCommand` subclasses run by the PureMVC controller when a notification fires. `LoginCommand` unbridges the `NSDictionary` body, retrieves `UserProxy` from the facade, and calls its async `login`; on the callback it sends `LOGIN_SUCCESS` / `LOGIN_FAILED` notifications (with an error body) back through the facade. `LogoutCommand` and `DataRefreshCommand` are defined inline in `PureMVCWrapper.mm`.

4. **Model** (`Model/UserProxy.{h,cpp}`) — a PureMVC `Proxy` holding session/user state and owning the `NetworkManager` + `UserTokenProvider`. This is where the backend base URL and network config live.

5. **Network** (`Network/`) — `NetworkManager` is a self-contained async HTTP client over `httplib` (`Libs/httplib.h`) + nlohmann `json` (`Libs/json/json.hpp`). Each request runs on a detached `std::thread`; retry, SSL config, header merging, and token injection are handled here. `UserTokenProvider` (`Network/Provider/user_token_provider.h`) supplies the bearer token and implements `refreshAccessToken` for automatic 401/403 retry.

### Notification names — keep three places in sync
The notification/command string constants are the contract across the whole stack:
- `Constants/PureMVCConstants.h` — C++ `std::string` constants (source of truth), used inside commands/wrapper.
- `Wrapper/PureMVCConstantsWrapper.{h,mm}` — `PureMVCNotifications` ObjC class exposing the same strings to Swift as `NSString`.
- Swift `switch` in `ViewController.onNotificationReceived` matches against `PureMVCNotifications.*`.

Adding or renaming a notification means editing all three. Commands are wired to these names in `initializeFacade`.

## Conventions & gotchas

- **`.h` vs `.mm`**: any header reachable from the Swift bridging header (`PureMVCWrapper.h`, `PureMVCConstantsWrapper.h`) must stay pure Objective-C / C — no C++ includes. C++ belongs in `.mm`/`.cpp` and in headers only included from those.
- **Threading**: network callbacks arrive on background threads; anything touching UIKit or the `PureMVCDelegate` must `dispatch_async(dispatch_get_main_queue(), ...)` (see `WrapperObserver` and `LoginCommand`).
- **String bridging**: use the helpers rather than raw `stringWithUTF8String:` — `safeStdStringFromNSString` / `safeStringFromStdString` in `Helper/StringHelper.h` (ObjC++ only) guard against nil/invalid UTF-8. `Helper/string_helper.h` is pure C++ (`parseErrorMessage` for backend error bodies).
- **Memory**: C++ objects created in `initializeFacade` (proxies, commands, observers) are `new`ed and owned by the facade/static storage; observers are cleaned up in `PureMVCWrapper.dealloc`. Bodies passed as `(__bridge void*)` must outlive the async call — see the `[username copy]` pattern in `LoginCommand`.
- **`Libs/`** (`PureMVC.xcframework`, `httplib.h`, `json.hpp`) is vendored third-party code — treat as read-only; the framework is built separately (see `README.md`).

## Backend

The demo points `UserProxy`/`UserTokenProvider` at a hosted API (`/api/v1/auth/login`, `/api/v1/auth/refresh`). Base URLs and SSL/verify flags are hard-coded in `UserProxy.cpp` and `user_token_provider.h`; change them there.
