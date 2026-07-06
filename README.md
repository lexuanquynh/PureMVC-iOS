# PureMVC-iOS

Ứng dụng demo với **business logic viết một lần bằng C++** (trên PureMVC multicore
framework + Clean Architecture) và **chạy chung cho cả iOS lẫn Android**. Mỗi nền
tảng chỉ thêm UI + một lớp bridge mỏng + một adapter secure-storage; phần
logic / network / TLS pinning / token store dùng lại nguyên.

Test một lần trên host (không cần simulator), consume qua Swift Package (iOS) và
NDK/JNI (Android). CI kiểm cả ba build surface trên mỗi PR.

## Kiến trúc

```
        iOS (Swift / UIKit)                 Android (Kotlin / Compose)
        PureMVCBridge (ObjC++)              AndroidAuthClient (JNI)
                └──────────── SHARED C++ Core ────────────┘
        LoginUseCase → AuthRepository → HttplibHttpClient (TLS + SPKI pinning)
                                      → SecureTokenStore
        KeychainSecureStore  ⟷  ISecureStorage  ⟷  JniSecureStorage → Keystore
```

Ports (`IHttpClient` / `IExecutor` / `ISecureStorage`) là "đường may" đa nền tảng:
cùng một C++ chạy trên cả hai, chỉ adapter sau port là khác nhau.

## Bố cục repo

| Đường dẫn | Nội dung |
|---|---|
| `Core/` | **C++ Core** headless (Domain + Infrastructure) + unit test (CMake/GoogleTest). Xem [`Core/README.md`](Core/README.md) |
| `Bridge/` | ObjC++ bridge cho iOS (`PMVCAuthClient`, `KeychainSecureStore`) |
| `Package.swift` | Swift Package (`PureMVCCoreCxx` + `PureMVCBridge`) để iOS consume |
| `PureMVC.xcodeproj`, `PureMVC/` | App iOS (Swift/UIKit + PureMVC facade) |
| `android/` | App Android (Kotlin/Compose) + JNI bridge + OpenSSL build. Xem [`android/README.md`](android/README.md) |
| `docs/` | [`cross-platform-core.md`](docs/cross-platform-core.md) (ADR), [`ios-integration-plan.md`](docs/ios-integration-plan.md) |

Chi tiết kiến trúc + quy ước: [`CLAUDE.md`](CLAUDE.md).

## Build & test

### Core (C++, trên host — nhanh nhất)
```sh
cd Core
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```
GoogleTest + cpp-httplib tải qua `FetchContent` (nlohmann/json vendored trong
`Core/ThirdParty/`). Test **không cần simulator, không cần mạng** khi chạy.

### iOS
Mở `PureMVC.xcodeproj` trong Xcode rồi Run, hoặc:
```sh
# Package (C++ core + ObjC++ bridge + Swift interop)
swift build && swift test

# App — dùng simulator arm64 (xcframework/OpenSSL chỉ có slice arm64-simulator)
xcodebuild -project PureMVC.xcodeproj -scheme PureMVC \
  -destination 'platform=iOS Simulator,name=iPhone 17 Pro' build
```
OpenSSL cho iOS đến từ SPM (`krzyzanowskim/OpenSSL-Package`).

### Android
```sh
cd android
# httplib cần OpenSSL >= 3.0; Google prefab chỉ có 1.1.1 nên build 3.x cho NDK một lần:
bash openssl/build-openssl.sh
./gradlew :app:assembleDebug
# test trên emulator/device:
./gradlew :app:connectedDebugAndroidTest
```

## CI

`.github/workflows/ci.yml` chạy trên mỗi PR: **Core** (`ctest`) · **iOS**
(`swift build`/`test`) · **Android** (`build-openssl.sh` + `assembleDebug`).

## Thư viện PureMVC (vendored)

`PureMVC.xcframework` + header trong `PureMVC/Libs/` build riêng từ framework gốc,
coi như read-only. Nguồn:
https://github.com/lexuanquynh/puremvc-cpp-multicore-framework (đọc `build-ios.md`).

## Quy trình đóng góp

Xem **Contribution workflow** trong [`CLAUDE.md`](CLAUDE.md): mỗi task tạo issue →
làm trên branch → test xanh → mở PR → merge sau khi được duyệt. Không push thẳng
vào `main`.

## Lộ trình

- [x] Core C++ Clean Architecture (LoginUseCase, AuthRepository, HttplibHttpClient, SecureTokenStore, CertificatePinner)
- [x] Đóng gói Swift Package + tích hợp app iOS; gỡ path `verifySSL=false` cũ
- [x] TLS verify + SPKI certificate pinning (dùng chung)
- [x] Chia sẻ Core sang Android (NDK/JNI): HTTPS thật + Keystore secure storage
- [x] CI cho host / iOS / Android
- [ ] Build PureMVC framework từ source cho NDK (nếu dùng Command/Proxy phía Android)
- [ ] Trỏ backend thật + thêm SPKI pins; Android instrumented tests vào CI
- [ ] UI iOS bằng Texture (AsyncDisplayKit)
