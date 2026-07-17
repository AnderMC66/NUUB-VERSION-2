#pragma once

#include <fstream>
#include <optional>
#include <string>

#include "domain/services/IReportingService.hpp"
#include "domain/services/IEncryptionService.hpp"

namespace nuub::domain::services {

class ReportingService final : public IReportingService {
    IEncryptionService& encryption_;
    std::string master_log_path_;
    std::string pc_identifier_;

public:
    ReportingService(IEncryptionService& encryption,
                     std::string master_log_path,
                     std::string pc_identifier);

    void flush_buffer(const std::string& log_text) override;
    [[nodiscard]] bool has_data() const override;
    [[nodiscard]] std::optional<std::string> generate_encrypted_report() override;
    void cleanup(bool remove_master = true) override;
    void delete_report(const std::string& path) override;
};

} // namespace nuub::domain::services
