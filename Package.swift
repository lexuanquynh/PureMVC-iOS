// swift-tools-version:5.9
import PackageDescription

// Local Swift Package exposing the headless C++ Core as a reusable module.
// The pure-C++ unit tests keep living under Core/ (CMake + GoogleTest); this
// manifest is the consumption path for the iOS app (and other projects).
let package = Package(
    name: "PureMVCCore",
    platforms: [
        .iOS(.v13),
        .macOS(.v12),
    ],
    products: [
        .library(name: "PureMVCCoreCxx", targets: ["PureMVCCoreCxx"]),
    ],
    dependencies: [
        .package(url: "https://github.com/krzyzanowskim/OpenSSL-Package.git", from: "3.3.3001"),
    ],
    targets: [
        // Domain + Infrastructure (C++). Third-party single headers are vendored
        // under Core/ThirdParty so the package is self-contained. The ObjC++
        // bridge (KeychainSecureStore.mm) is excluded here — it belongs to a
        // separate bridge target added when wiring the app.
        .target(
            name: "PureMVCCoreCxx",
            dependencies: [
                .product(name: "OpenSSL", package: "OpenSSL-Package"),
            ],
            path: "Core",
            exclude: [
                "Infrastructure/Security/KeychainSecureStore.mm",
            ],
            sources: [
                "Domain",
                "Infrastructure",
            ],
            publicHeadersPath: ".",
            cxxSettings: [
                .headerSearchPath("."),
                .headerSearchPath("ThirdParty"),
                .headerSearchPath("ThirdParty/httplib"),
                .define("CPPHTTPLIB_OPENSSL_SUPPORT"),
            ]
        ),
    ],
    cxxLanguageStandard: .gnucxx14
)
