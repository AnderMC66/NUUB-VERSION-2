#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace nuub::domain::services {

class IEncryptionService {
public:
    virtual ~IEncryptionService() = default;

    [[nodiscard]] virtual std::vector<unsigned char> encrypt(
        const std::vector<unsigned char>& data,
        const std::vector<unsigned char>& aad = {}) const = 0;

    [[nodiscard]] virtual std::vector<unsigned char> decrypt(
        const std::vector<unsigned char>& encrypted,
        const std::vector<unsigned char>& aad = {}) const = 0;

    [[nodiscard]] virtual std::uint32_t get_current_key_id() const = 0;

    virtual void rotate_key() = 0;
};

} // namespace nuub::domain::services
