#pragma once
#include <atomic>
#include <chrono>
#include <mutex>
#include <opencv2/opencv.hpp>
#include <string>
#include <thread>

class RTSPStream {
public:
    RTSPStream();

    ~RTSPStream();

    void SetLogin(const std::string&);

    void SetPassword(const std::string&);

    void SetIpAddress(const std::string&);

    void SetPort(const std::string&);

    void SetSource(const std::string&);

    bool Initialize();

    bool Connect(int timeout_ms = 5000);

    void Reconnect();

    void RequestReconnect();

    void CaptureLoop();

    bool IsRunning();

    bool IsConnected();

    cv::Mat GetFrame();

    RTSPStream(const RTSPStream&) = delete;
    RTSPStream& operator=(const RTSPStream&) = delete;

private:
    std::string login_;
    std::string password_;
    std::string ip_address_;
    std::string port_;
    std::string source_;
    int stream_width_ = 0;
    int stream_height_ = 0;
    double stream_fps_ = 0.;
    std::string stream_full_url_;
    cv::VideoCapture stream_;
    cv::Mat current_frame_;
    std::atomic<bool> running_{false};
    std::atomic<bool> connected_{false};
    std::atomic<bool> reconnect_requested_{false};
    std::mutex frame_mutex_;
    std::thread capture_thread_;
    int reconnect_attempts_ = 5;
    std::vector<int> reconnect_times_{1000, 5000, 10000, 20000, 30000};
    int open_timeout_ms_ = 5000;
    int read_timeout_ms_ = 1000;
};
