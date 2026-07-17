#include "domain/services/ReportingService.hpp"

#include <chrono>
#include <cstdio>
#include <filesystem>

namespace nuub::domain::services {

ReportingService::ReportingService(
    IEncryptionService& encryption,
    std::string master_log_path,
    std::string pc_identifier)
    : encryption_(encryption)
    , master_log_path_(std::move(master_log_path))
    , pc_identifier_(std::move(pc_identifier)) {}

void ReportingService::flush_buffer(const std::string& log_text) {
    if (log_text.empty()) return;
    std::ofstream file(master_log_path_, std::ios::app);
    if (file.is_open()) {
        file << log_text;
    }
}

bool ReportingService::has_data() const {
    std::error_code ec;
    auto size = std::filesystem::file_size(master_log_path_, ec);
    return !ec && size > 0;
}

std::optional<std::string> ReportingService::generate_encrypted_report() {
    if (!has_data()) return std::nullopt;

    std::ifstream file(master_log_path_, std::ios::binary);
    if (!file.is_open()) return std::nullopt;

    std::vector<unsigned char> plaintext(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>());
    file.close();

    // Use PC identifier as AAD for additional authentication
    std::vector<unsigned char> aad(pc_identifier_.begin(), pc_identifier_.end());
    auto encrypted = encryption_.encrypt(plaintext, aad);

    auto ts = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    std::string report_path = "reporte_" + pc_identifier_ + "_" +
                              std::to_string(ts) + ".enc";

    std::ofstream out(report_path, std::ios::binary);
    if (!out.is_open()) return std::nullopt;
    out.write(reinterpret_cast<const char*>(encrypted.data()), encrypted.size());
    out.close();

    return report_path;
}

void ReportingService::cleanup(bool remove_master) {
    if (remove_master) {
        std::error_code ec;
        std::filesystem::remove(master_log_path_, ec);
    }
}

void ReportingService::delete_report(const std::string& path) {
    std::error_code ec;
    std::filesystem::remove(path, ec);
}

} // namespace nuub::domain::services
