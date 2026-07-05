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
        .library(name: "PureMVCBridge", targets: ["PureMVCBridge"]),
    ],
    dependencies: [
        .package(url: "https://github.com/krzyzanowskim/OpenSSL-Package.git", from: "3.3.3001"),
    ],
    targets: [
        // Domain + Infrastructure (pure C++). Third-party single headers are
        // vendored under Core/ThirdParty so the package is self-contained.
        .target(
            name: "PureMVCCoreCxx",
            dependencies: [
                .product(name: "OpenSSL", package: "OpenSSL-Package"),
            ],
            path: "Core",
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

        // Objective-C++ bridge: the Swift-facing surface. Public headers stay
        // pure Objective-C so Swift can import the module directly.
        .target(
            name: "PureMVCBridge",
            dependencies: ["PureMVCCoreCxx"],
            path: "Bridge",
            publicHeadersPath: "include",
            cxxSettings: [
                .headerSearchPath("."),
            ],
            linkerSettings: [
                .linkedFramework("Foundation"),
                .linkedFramework("Security"),
            ]
        ),

        // Swift smoke test: proves Swift -> ObjC++ -> C++ links end to end.
        .testTarget(
            name: "PureMVCBridgeTests",
            dependencies: ["PureMVCBridge"]
        ),
    ],
    cxxLanguageStandard: .gnucxx14
)
