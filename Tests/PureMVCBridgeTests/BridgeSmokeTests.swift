//
//  BridgeSmokeTests.swift
//  PureMVCBridgeTests
//
//  Verifies the package is importable from Swift and that the ObjC++ -> C++
//  bridge links. Deliberately does NOT perform Keychain I/O (save/load/clear),
//  which is unreliable under `swift test` on a CLI host without entitlements —
//  that path is exercised on device/simulator during app integration.
//

import XCTest
import PureMVCBridge

final class BridgeSmokeTests: XCTestCase {
    func testCanConstructKeychainTokenStore() {
        let store = PMVCKeychainTokenStore(service: "com.puremvc.tests")
        XCTAssertNotNil(store)
    }

    func testTokenPairHoldsValues() {
        let pair = PMVCTokenPair()
        pair.accessToken = "a"
        pair.refreshToken = "r"
        XCTAssertEqual(pair.accessToken, "a")
        XCTAssertEqual(pair.refreshToken, "r")
    }
}
