# Android

Placeholder for the Android app + the JNI bridge to the shared C++ `Core`.
**Create your new Android Studio project inside this `android/` directory.**

See the design/decisions in [`../docs/cross-platform-core.md`](../docs/cross-platform-core.md).

## How it will consume the shared Core

The shared business logic lives in `../Core` (pure C++, ports:
`ISecureStorage` / `IHttpClient` / `IExecutor`). Android reaches it through the NDK
using the **same `Core/CMakeLists.txt`** that the host tests use, plus a small JNI
bridge — no logic is reimplemented.

### Suggested module layout (create these when you make the project)

```
android/
  settings.gradle(.kts)
  app/                       your Kotlin app (Compose UI)
  core-native/               Android library wrapping the C++ Core via JNI
    build.gradle(.kts)       externalNativeBuild { cmake { path "src/main/cpp/CMakeLists.txt" } }
    src/main/cpp/
      CMakeLists.txt         add_subdirectory(../../../../../Core)  # -> puremvc_core
                             add_library(puremvc_jni SHARED jni_bridge.cpp)
                             target_link_libraries(puremvc_jni puremvc_core ...)
      jni_bridge.cpp         JNI surface mirroring iOS PMVCAuthClient
    src/main/java/.../PureMVCCore.kt   Kotlin wrapper (holds the native handle)
    src/main/java/.../KeystoreSecureStorage.kt   ISecureStorage impl helper
```

### Wiring checklist

1. **NDK + CMake**: enable `externalNativeBuild` in `core-native`; point CMake at a
   thin `src/main/cpp/CMakeLists.txt` that does `add_subdirectory(<path-to>/Core)`
   and links `puremvc_core` into a `puremvc_jni` shared lib. Set `abiFilters`
   (`arm64-v8a`, `armeabi-v7a`, `x86_64`) and `CMAKE_CXX_STANDARD` matching Core.
2. **Third-party**: `httplib` + `nlohmann/json` are vendored in `Core/ThirdParty/`
   already — reused as-is.
3. **OpenSSL**: provide OpenSSL/BoringSSL prebuilt for the NDK ABIs and link it
   (needed by `HttplibHttpClient` + TLS pinning). If you instead choose OkHttp,
   implement `IHttpClient` on the JVM side and skip native OpenSSL (see ADR D1).
4. **PureMVC framework**: build the PureMVC C++ multicore framework from source for
   the NDK (the iOS `PureMVC.xcframework` is iOS-only).
5. **JNI bridge**: expose an auth facade (`login` / `logout` / `currentTokens`)
   mirroring `Bridge/include/PureMVCBridge/PMVCAuthClient.h`. Marshal callbacks to
   the UI thread; hold a `GlobalRef` for the callback; UTF-8 for strings.
6. **Secure storage**: implement `core::ISecureStorage` for Android (Android
   Keystore + EncryptedSharedPreferences), the counterpart of iOS
   `KeychainSecureStore`.

## Status

**Slice 1 done** — the shared C++ Core builds into the app via the NDK:
- `app/src/main/cpp/CMakeLists.txt` does `add_subdirectory(../Core)` with
  `PUREMVC_CORE_WITH_HTTPLIB=OFF` (Domain + non-network Infrastructure; native
  OpenSSL is a later slice) and links a JNI lib `puremvc_jni`.
- `app/src/main/cpp/jni_bridge.cpp` + `PureMVCCore.kt` run the real
  `LoginUseCase` validation across JNI (smoke).
- `./gradlew :app:assembleDebug` builds `libpuremvc_jni.so` for `arm64-v8a` +
  `x86_64` and packages it in the APK. Instrumented test in
  `app/src/androidTest/.../PureMVCCoreTest.kt` (run with
  `./gradlew :app:connectedDebugAndroidTest` on a device/emulator).

**Slice 2 done** — auth facade + Compose login:
- `jni_bridge.cpp` `AndroidAuthClient` composes `ThreadExecutor` + a stub
  `IHttpClient` (canned login, async) + `AuthRepository` + `LoginUseCase` +
  in-memory token store; JNI create/login/logout/currentAccessToken/destroy with a
  Kotlin callback (GlobalRef + AttachCurrentThread).
- `AndroidAuthClient.kt` wrapper (posts results to the main thread) + Compose
  `LoginScreen` in `MainActivity`.
- Verified on emulator: `./gradlew :app:connectedDebugAndroidTest` — 6/6 pass,
  incl. real login through the Core pipeline.
- The stub `IHttpClient` is swapped for `HttplibHttpClient` once native OpenSSL
  lands; the JNI/Kotlin API stays the same.

**Slice 3 done** — real HTTPS via the shared `HttplibHttpClient` (+ shared cert
pinning), backed by static OpenSSL 3.x for the NDK:
- **Run `android/openssl/build-openssl.sh` once** (before building; also in CI) to
  produce `android/openssl/prebuilt/<abi>/` — httplib 0.21 needs OpenSSL ≥ 3.0 and
  Google's NDK prefab only ships 1.1.1, so we build 3.x from source (output is
  gitignored).
- `app/src/main/cpp/CMakeLists.txt` links that static OpenSSL and builds Core with
  `PUREMVC_CORE_WITH_HTTPLIB=ON`; `AndroidAuthClient` now uses the real client
  (`nativeCreate(host, port)`).
- Verified on emulator: `connectedDebugAndroidTest` 6/6 (validation + real TLS
  transport failure to an unreachable host).

**Slice 4 done** — Keystore-backed token storage (counterpart of iOS Keychain):
- `SecureStorage.kt` uses EncryptedSharedPreferences (Android Keystore); the C++
  `JniSecureStorage` implements `core::ISecureStorage` by calling up into it via
  JNI, and `AndroidAuthClient` composes `SecureTokenStore` over it.
- `AndroidAuthClient(context, host, port)` — tokens persist encrypted.
- Verified on emulator: `connectedDebugAndroidTest` 7/7 (incl. Keystore round-trip).

### Next slices
1. Build the PureMVC C++ framework from source for the NDK (if using Command/Proxy
   on Android).
2. Point at a real backend and add SPKI pins in `jni_bridge.cpp`.
