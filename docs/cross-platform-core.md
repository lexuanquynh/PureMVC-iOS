# Sharing the C++ Core across iOS + Android

Status: **Proposed** (issue #27). Decisions below are recommended defaults —
adjust on review. Implementation follows the workflow in `CLAUDE.md`
(issue → branch → tests → PR → approved merge).

## Context

The business logic (PureMVC use cases / proxies / commands, `AuthRepository`,
`HttplibHttpClient`, `SecureTokenStore`, `CertificatePinner`) already lives in a
**headless, pure-C++ `Core`** that talks to the outside world only through *ports*
(`ISecureStorage`, `IHttpClient`, `IExecutor`). It is unit-tested off-device
(CMake + GoogleTest) and consumed on iOS via a thin ObjC++ bridge (`PureMVCBridge`).

Goal: write the logic layer once in C++ and reuse it on **both iOS and Android**,
so each new app only adds UI + a thin bridge + a couple of platform adapters.

## The pattern

```
        ┌──────────── SHARED C++ (write once, test once) ─────────────┐
        │  Domain (UseCases, Entities) · Infrastructure               │
        │  (AuthRepository, HttplibHttpClient, SecureTokenStore,      │
        │   CertificatePinner) · Ports · PureMVC C++ framework        │
        └───────▲──────────────────────────────────────▲─────────────┘
        ObjC++ bridge                              JNI bridge
        ┌───────┴─────────┐                    ┌────────┴──────────┐
        │ iOS PureMVCBridge│                   │ Android core-native│
        │ PMVCAuthClient    │                  │ (.so) → Kotlin     │
        │ → Swift UI        │                  │ → Compose UI       │
        └───────────────────┘                  └────────────────────┘
```

Ports are the platform seam: the same C++ business code runs on both; only the
adapters behind the ports (and the UI + bridge) differ.

## Decisions (proposed)

### D1 — HTTP on Android: **reuse the shared `HttplibHttpClient`** (httplib + OpenSSL)
httplib compiles under the Android NDK, so the whole network path — retries, error
mapping, and **certificate pinning** (`CertificatePinner`, already C++) — is shared.
- Cost: ship OpenSSL for the NDK (prebuilt OpenSSL/BoringSSL for `arm64-v8a`,
  `armeabi-v7a`, `x86_64`).
- Alternative (escape hatch): implement `IHttpClient` with **OkHttp** via JNI —
  native system TLS + OkHttp `CertificatePinner`, no native OpenSSL — but that is a
  second, non-shared HTTP impl. Choose this only if OkHttp integration (proxy,
  system trust, interceptors) is required.

### D2 — Bridge: **hand-written JNI** now; adopt **djinni** if the surface grows
The shared API surface is small (an auth facade like `PMVCAuthClient`:
`login/logout/currentTokens`). Hand-writing a JNI bridge (mirroring the ObjC++
one) is trivial and adds no tooling. If the shared interface grows large, switch to
[djinni](https://github.com/dropbox/djinni), which generates **both** the ObjC++ and
Java/JNI glue from one interface definition. (Kotlin Multiplatform is a different
philosophy — sharing Kotlin, not C++ — and does not fit the existing C++ investment.)

### D3 — Repo: **monorepo**
Keep `core/` (shared) + `platform/ios` + `platform/android` in one repo so a Core
change and both apps move together. (Alternative: a versioned core repo consumed via
submodule — better if Core becomes a base for many unrelated apps, at the cost of
submodule friction.)

## Build system: CMake is canonical

The same `Core/CMakeLists.txt` used for host tests is also the Android NDK build:
- **Android**: Gradle `externalNativeBuild` → `Core/CMakeLists.txt` + a JNI target →
  `.so` per ABI, packaged in an AAR / app module.
- **iOS**: `Package.swift` (SPM) compiles the *same* sources + the ObjC++ bridge.
- **Tests**: CMake + GoogleTest — business logic covered once for both platforms.

Notes:
- **PureMVC framework**: iOS currently uses the prebuilt `PureMVC.xcframework`. For
  Android, build the PureMVC C++ framework **from source** via CMake. Longer term,
  build it from source for both platforms to drop the binary xcframework.
- **Vendored deps**: `httplib` + `nlohmann/json` are already in `Core/ThirdParty/` —
  reused verbatim by Android.
- **OpenSSL**: iOS via SPM (`krzyzanowskim/OpenSSL-Package`); Android via a prebuilt
  OpenSSL/BoringSSL for the NDK (per D1).

## Platform port implementations

| Port | iOS | Android |
|---|---|---|
| `ISecureStorage` | `KeychainSecureStore` (Keychain) | new adapter over **Android Keystore + EncryptedSharedPreferences** (JNI → Kotlin helper) |
| `IHttpClient` | `HttplibHttpClient` | **shared** `HttplibHttpClient` (D1) |
| `IExecutor` | `ThreadExecutor` (std::thread) | **shared** `ThreadExecutor` |
| Cert pinning | `CertificatePinner` (C++) | **shared** (with D1) |

Only the secure-storage adapter, the JNI bridge, and the UI are Android-specific.

## Bridge & interop notes (Android)

- Keep the JNI surface small — mirror `PMVCAuthClient` (`login`, `logout`,
  `currentTokens`). A native handle is held by a Kotlin class.
- **Callbacks C++ → JVM**: hold a `GlobalRef` to the callback, `AttachCurrentThread`
  on the worker thread, then marshal the result to the UI thread (`Handler` /
  `runOnUiThread`). (iOS already marshals to the main queue.)
- **Strings**: marshal `std::string` ↔ `jstring` as UTF-8 (same as the iOS
  `NSString` ↔ `std::string` helpers).
- **Lifetime**: the native object graph must outlive in-flight requests — tie it to
  the Kotlin wrapper's lifecycle (release in `close()`/`onCleared`).
- **C++ standard**: keep Core at a standard both NDK and SPM accept (C++14/17).

## Target repo layout

```
core/                     shared C++ (Domain, Infrastructure, ThirdParty, CMakeLists, tests)
platform/ios/             Package.swift + ObjC++ bridge + Xcode app
platform/android/         Gradle project + JNI bridge (core-native) + Kotlin app
```

Today the tree is iOS-first (`Core/`, `Bridge/`, `Package.swift`, `PureMVC.xcodeproj`).
The `android/` directory is scaffolded now; a physical reorg into `platform/` can
happen later if desired.

## Suggested migration steps

1. Scaffold `android/` (this change) — see `android/README.md`.
2. Build PureMVC framework from source for the NDK (or vendor a prebuilt Android lib).
3. Source OpenSSL/BoringSSL for the NDK (per D1).
4. `core-native` Android library: CMake `add_subdirectory(Core)` + a JNI target →
   `.so`; hand-written JNI bridge mirroring `PMVCAuthClient`.
5. Android `ISecureStorage` adapter (Keystore + EncryptedSharedPreferences).
6. Kotlin wrapper + Compose UI; verify login/logout end to end on an emulator.
7. CI: add an Android NDK build job alongside the existing `ctest` / iOS jobs.

## Consequences

- **+** Business logic, networking, and pinning written and tested once.
- **+** New apps (either platform) cost only UI + bridge.
- **−** Two build integrations to maintain (SPM + Gradle/NDK) over shared sources.
- **−** Android must ship native OpenSSL (unless D1 escape hatch / OkHttp is chosen).
