# PureMVC-iOS

Ứng dụng iOS demo với **logic nghiệp vụ chạy trên PureMVC C++ multicore framework**
thay vì Swift. Tầng Swift/UIKit chỉ là lớp vỏ mỏng; phần MVC thật
(Model / Command / Notification) nằm ở C++ và Objective-C++, expose sang Swift
qua một lớp bridging wrapper. Luồng mẫu: đăng nhập / đăng xuất / refresh dữ liệu
với một REST backend.

Mục tiêu dài hạn: làm **base tái sử dụng** cho nhiều dự án yêu cầu hiệu năng cao
và bảo mật tốt — core C++ độc lập nền tảng, test được không cần simulator, UI dựng
bằng [Texture / AsyncDisplayKit](https://github.com/texturegroup/texture).

## Kiến trúc

```
Swift + UIKit (ViewController, AppDelegate)      ← lớp vỏ mỏng
        │  delegate / callback
PureMVCWrapper (Objective-C++)                    ← ranh giới Swift ↔ C++ duy nhất
        │  Facade.sendNotification(...)
PureMVC C++ (Command / Proxy / Notification)      ← logic nghiệp vụ
        │
Network (NetworkManager over httplib + json)      ← HTTP bất đồng bộ
```

- **`PureMVC/`** — app iOS: Swift UI, wrapper ObjC++, Command/Proxy/Constants,
  tầng Network (`httplib` + nlohmann `json`), và framework vendored trong `Libs/`.
- **`Core/`** — module **C++ headless** (Clean Architecture) đang được xây dựng:
  domain thuần, không phụ thuộc iOS/Objective-C, unit-test trên host trong
  mili-giây. Xem [`Core/README.md`](Core/README.md).

Chi tiết kiến trúc, quy ước, và các điểm dễ vướng: xem [`CLAUDE.md`](CLAUDE.md).

## Yêu cầu

- Xcode (chạy trên **iOS Simulator** — app biên dịch stack HTTP C++ và link static
  lib theo từng kiến trúc từ `PureMVC.xcframework`)
- CMake ≥ 3.14 (để build/test module `Core` trên host)

## Build & chạy app iOS

Mở `PureMVC.xcodeproj` trong Xcode, chọn một simulator rồi Run. Hoặc dùng CLI:

```sh
xcodebuild -project PureMVC.xcodeproj -scheme PureMVC \
  -destination 'platform=iOS Simulator,name=iPhone 15' build

# chạy test
xcodebuild -project PureMVC.xcodeproj -scheme PureMVC \
  -destination 'platform=iOS Simulator,name=iPhone 15' test
```

### Build settings bắt buộc (đã cấu hình sẵn — giữ nguyên khi sửa project)

Nếu thêm target C++/ObjC++ mới hoặc mất cấu hình, khôi phục:

- `CLANG_CXX_LANGUAGE_STANDARD = gnu++0x` (C++11), thêm `-std=c++11` vào `OTHER_CPLUSPLUSFLAGS`
- `HEADER_SEARCH_PATHS = $(PROJECT_DIR)/PureMVC/Libs/include/**`
- `SWIFT_OBJC_BRIDGING_HEADER = PureMVC/PureMVC-Bridging-Header.h`

## Build & test module Core (C++, trên host)

```sh
cd Core
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

GoogleTest được tải tự động qua CMake `FetchContent` (cần mạng lần cấu hình đầu).
Test core **không cần simulator, không cần mạng** khi chạy.

## Thư viện PureMVC (vendored)

`PureMVC.xcframework` và các header trong `PureMVC/Libs/` được build riêng từ
framework gốc — coi như read-only:

- Nguồn: https://github.com/lexuanquynh/puremvc-cpp-multicore-framework
- Tải về và làm theo `build-ios.md` trong repo đó để tạo lại xcframework.

## Quy trình đóng góp

Xem mục **Contribution workflow** trong [`CLAUDE.md`](CLAUDE.md): mỗi task tạo
GitHub issue → làm trên branch → test xanh → mở pull request → merge sau khi được
duyệt. Không push thẳng vào `main`.

## Lộ trình

- [x] Core C++ headless + CMake/GoogleTest (LoginUseCase)
- [ ] Tách `IHttpClient` / `IExecutor`, `AuthRepository` test được bằng mock
- [ ] `KeychainTokenStore` + bật SSL verify + certificate pinning
- [ ] Nối `LoginCommand` / `UserProxy` gọi vào use case
- [ ] UI bằng Texture (AsyncDisplayKit) + snapshot test
- [ ] CI (cmake/ctest + xcodebuild test)
