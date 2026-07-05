//
//  CertificatePinnerTests.cpp
//  PureMVC Core tests
//

#include <gtest/gtest.h>

#include "Infrastructure/Security/CertificatePinner.hpp"

using namespace core;

TEST(CertificatePinner, DisabledWhenNoPins) {
    CertificatePinner pinner;
    EXPECT_FALSE(pinner.enabled());
    // With no pins nothing is "trusted" by pin — pinning simply does not apply.
    EXPECT_FALSE(pinner.isTrusted("anything"));
}

TEST(CertificatePinner, EnabledWhenPinsPresent) {
    CertificatePinner pinner({"pin-a"});
    EXPECT_TRUE(pinner.enabled());
}

TEST(CertificatePinner, TrustsAMatchingPin) {
    CertificatePinner pinner({"pin-a", "pin-b"});
    EXPECT_TRUE(pinner.isTrusted("pin-a"));
    EXPECT_TRUE(pinner.isTrusted("pin-b")); // backup pin (key rotation)
}

TEST(CertificatePinner, RejectsANonMatchingPin) {
    CertificatePinner pinner({"pin-a"});
    EXPECT_FALSE(pinner.isTrusted("pin-x"));
    EXPECT_FALSE(pinner.isTrusted(""));
}
