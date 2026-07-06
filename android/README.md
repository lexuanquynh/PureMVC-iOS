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

### Next slices
1. Native OpenSSL/BoringSSL for the NDK → flip `PUREMVC_CORE_WITH_HTTPLIB=ON`
   (enables `HttplibHttpClient` + shared cert pinning); or an OkHttp `IHttpClient`.
2. Build the PureMVC C++ framework from source for the NDK.
3. JNI auth facade mirroring `PMVCAuthClient` + Kotlin wrapper.
4. Android `ISecureStorage` (Keystore + EncryptedSharedPreferences).
