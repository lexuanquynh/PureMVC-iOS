//
//  AuthClientSmokeTests.swift
//  PureMVCBridgeTests
//
//  Confirms the auth facade is importable and composes the C++ stack. The actual
//  login round-trip (network + Keychain) is verified on the simulator during app
//  integration, not here.
//

import XCTest
import PureMVCBridge

final class AuthClientSmokeTests: XCTestCase {
    func testCanConstructAuthClient() {
        let client = PMVCAuthClient(host: "api.example.com", port: 443, pinnedSPKIHashes: nil)
        XCTAssertNotNil(client)
    }

    func testCanConstructWithPins() {
        let client = PMVCAuthClient(host: "api.example.com",
                                    port: 443,
                                    pinnedSPKIHashes: ["AAAA", "BBBB"])
        XCTAssertNotNil(client)
    }
}
