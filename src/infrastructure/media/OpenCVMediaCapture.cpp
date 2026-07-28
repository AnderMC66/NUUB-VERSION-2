#include "infrastructure/media/OpenCVMediaCapture.hpp"

#include <chrono>
#include <vector>

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>

#include <portaudio.h>
#include <sndfile.h>

#ifdef _WIN32
#include <Windows.h>
#elif defined(__linux__)
#include "infrastructure/media/LinuxScreenCapture.hpp"
#endif

namespace nuub::infrastructure::media {

OpenCVMediaCapture::OpenCVMediaCapture(std::string pc_id)
    : pc_id_(std::move(pc_id)) {}

std::optional<std::string> OpenCVMediaCapture::take_photo() {
    cv::VideoCapture cap(0);
    if (!cap.isOpened()) return std::nullopt;

    cv::Mat frame;
    bool ret = cap.read(frame);
    cap.release();

    if (!ret || frame.empty()) return std::nullopt;

    auto ts = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    std::string path = "foto_" + pc_id_ + "_" + std::to_string(ts) + ".jpg";
    if (!cv::imwrite(path, frame)) return std::nullopt;
    return path;
}

std::optional<std::string> OpenCVMediaCapture::take_video(int duration_sec) {
    cv::VideoCapture cap(0);
    if (!cap.isOpened()) return std::nullopt;

    auto ts = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    std::string path = "video_" + pc_id_ + "_" + std::to_string(ts) + ".mp4";
    int fourcc = cv::VideoWriter::fourcc('a', 'v', 'c', '1');
    cv::VideoWriter writer(path, fourcc, 20.0, cv::Size(640, 480));
    if (!writer.isOpened()) {
        fourcc = cv::VideoWriter::fourcc('m', 'p', '4', 'v');
        writer.open(path, fourcc, 20.0, cv::Size(640, 480));
    }

    auto start = std::chrono::steady_clock::now();
    cv::Mat frame;

    while (true) {
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - start).count();
        if (elapsed >= duration_sec) break;

        if (!cap.read(frame) || frame.empty()) break;
        writer.write(frame);
    }

    writer.release();
    cap.release();
    return path;
}

std::optional<std::string> OpenCVMediaCapture::record_audio(int duration_sec) {
    constexpr int SAMPLE_RATE = 44100;
    constexpr int CHANNELS = 2;

    Pa_Initialize();

    PaStream* stream = nullptr;
    PaStreamParameters input_params{};
    input_params.device = Pa_GetDefaultInputDevice();
    if (input_params.device == paNoDevice) {
        Pa_Terminate();
        return std::nullopt;
    }
    input_params.channelCount = CHANNELS;
    input_params.sampleFormat = paInt16;
    input_params.suggestedLatency =
        Pa_GetDeviceInfo(input_params.device)->defaultLowInputLatency;

    PaError err = Pa_OpenStream(&stream, &input_params, nullptr,
                                SAMPLE_RATE, paFramesPerBufferUnspecified,
                                paClipOff, nullptr, nullptr);
    if (err != paNoError) {
        Pa_Terminate();
        return std::nullopt;
    }

    Pa_StartStream(stream);

    int total_samples = SAMPLE_RATE * duration_sec * CHANNELS;
    std::vector<short> buffer(total_samples);

    Pa_ReadStream(stream, buffer.data(), SAMPLE_RATE * duration_sec);

    Pa_StopStream(stream);
    Pa_CloseStream(stream);
    Pa_Terminate();

    auto ts = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    std::string path = "audio_" + pc_id_ + "_" + std::to_string(ts) + ".wav";

    SF_INFO sf_info{};
    sf_info.samplerate = SAMPLE_RATE;
    sf_info.channels = CHANNELS;
    sf_info.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;

    SNDFILE* sndfile = sf_open(path.c_str(), SFM_WRITE, &sf_info);
    if (!sndfile) return std::nullopt;

    sf_write_short(sndfile, buffer.data(), total_samples);
    sf_close(sndfile);

    return path;
}

std::optional<std::string> OpenCVMediaCapture::take_screenshot() {
#ifdef _WIN32
    int width = GetSystemMetrics(SM_CXSCREEN);
    int height = GetSystemMetrics(SM_CYSCREEN);

    HDC hdc_screen = GetDC(nullptr);
    HDC hdc_mem = CreateCompatibleDC(hdc_screen);
    HBITMAP hbitmap = CreateCompatibleBitmap(hdc_screen, width, height);
    SelectObject(hdc_mem, hbitmap);

    BitBlt(hdc_mem, 0, 0, width, height, hdc_screen, 0, 0, SRCCOPY);

    BITMAPINFOHEADER bi{};
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = width;
    bi.biHeight = -height;
    bi.biPlanes = 1;
    bi.biBitCount = 32;
    bi.biCompression = BI_RGB;

    std::vector<unsigned char> buffer(width * height * 4);
    GetDIBits(hdc_mem, hbitmap, 0, height, buffer.data(),
              reinterpret_cast<BITMAPINFO*>(&bi), DIB_RGB_COLORS);

    DeleteObject(hbitmap);
    DeleteDC(hdc_mem);
    ReleaseDC(nullptr, hdc_screen);

    cv::Mat mat(height, width, CV_8UC4, buffer.data());
    cv::Mat bgr;
    cv::cvtColor(mat, bgr, cv::COLOR_BGRA2BGR);

    auto ts = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    std::string path = "screenshot_" + pc_id_ + "_" + std::to_string(ts) + ".png";
    if (!cv::imwrite(path, bgr)) return std::nullopt;
    return path;
#elif defined(__linux__)
    LinuxScreenCapture linux_capture(pc_id_);
    return linux_capture.take_screenshot();
#else
    return std::nullopt;
#endif
}

std::optional<std::string> OpenCVMediaCapture::screen_record(int duration_sec) {
    auto ts = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    std::string path = "screenrecord_" + pc_id_ + "_" + std::to_string(ts) + ".mp4";

#ifdef _WIN32
    int width = GetSystemMetrics(SM_CXSCREEN);
    int height = GetSystemMetrics(SM_CYSCREEN);

    int fourcc = cv::VideoWriter::fourcc('a', 'v', 'c', '1');
    cv::VideoWriter writer(path, fourcc, 15.0, cv::Size(width, height));
    if (!writer.isOpened()) {
        fourcc = cv::VideoWriter::fourcc('m', 'p', '4', 'v');
        writer.open(path, fourcc, 15.0, cv::Size(width, height));
    }
    if (!writer.isOpened()) return std::nullopt;

    auto start = std::chrono::steady_clock::now();

    while (true) {
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - start).count();
        if (elapsed >= duration_sec) break;

        HDC hdc_screen = GetDC(nullptr);
        HDC hdc_mem = CreateCompatibleDC(hdc_screen);
        HBITMAP hbitmap = CreateCompatibleBitmap(hdc_screen, width, height);
        SelectObject(hdc_mem, hbitmap);
        BitBlt(hdc_mem, 0, 0, width, height, hdc_screen, 0, 0, SRCCOPY);

        BITMAPINFOHEADER bi{};
        bi.biSize = sizeof(BITMAPINFOHEADER);
        bi.biWidth = width;
        bi.biHeight = -height;
        bi.biPlanes = 1;
        bi.biBitCount = 32;
        bi.biCompression = BI_RGB;

        std::vector<unsigned char> buffer(width * height * 4);
        GetDIBits(hdc_mem, hbitmap, 0, height, buffer.data(),
                  reinterpret_cast<BITMAPINFO*>(&bi), DIB_RGB_COLORS);

        DeleteObject(hbitmap);
        DeleteDC(hdc_mem);
        ReleaseDC(nullptr, hdc_screen);

        cv::Mat mat(height, width, CV_8UC4, buffer.data());
        cv::Mat bgr;
        cv::cvtColor(mat, bgr, cv::COLOR_BGRA2BGR);
        writer.write(bgr);
    }

    writer.release();
    return path;
#else
    return std::nullopt;
#endif
}

} // namespace nuub::infrastructure::media
