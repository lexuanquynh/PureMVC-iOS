//
//  CertificatePinner.hpp
//  PureMVC Core — Infrastructure
//
//  Holds the set of accepted public-key pins (SHA-256 of the certificate's
//  SubjectPublicKeyInfo, base64-encoded) and decides whether a presented pin is
//  trusted. Pure C++ so the policy is host-testable; the OpenSSL code that turns
//  a live certificate into a pin string lives in the guarded client path.
//

#ifndef PUREMVC_CORE_CERTIFICATE_PINNER_HPP
#define PUREMVC_CORE_CERTIFICATE_PINNER_HPP

#include <string>
#include <utility>
#include <vector>

namespace core {

class CertificatePinner {
public:
    CertificatePinner() = default;
    explicit CertificatePinner(std::vector<std::string> pins)
        : pins_(std::move(pins)) {}

    // No pins configured => pinning is off (the client falls back to normal
    // chain verification only).
    bool enabled() const { return !pins_.empty(); }

    bool isTrusted(const std::string& spkiSha256Base64) const {
        for (const std::string& pin : pins_) {
            if (pin == spkiSha256Base64) {
                return true;
            }
        }
        return false;
    }

private:
    std::vector<std::string> pins_;
};

} // namespace core

#endif // PUREMVC_CORE_CERTIFICATE_PINNER_HPP
