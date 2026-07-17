#pragma once

#include <optional>
#include <string>

namespace nuub::domain::services {

class IReportingService {
public:
    virtual ~IReportingService() = default;

    virtual void flush_buffer(const std::string& log_text) = 0;
    [[nodiscard]] virtual bool has_data() const = 0;
    [[nodiscard]] virtual std::optional<std::string> generate_encrypted_report() = 0;
    virtual void cleanup(bool remove_master = true) = 0;
    virtual void delete_report(const std::string& path) = 0;
};

} // namespace nuub::domain::services
