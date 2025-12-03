#ifndef INCLUDE_RTSPSTREAM_HPP_
#define INCLUDE_RTSPSTREAM_HPP_
#include <atomic>
#include <chrono>
#include <mutex>
#include <opencv2/opencv.hpp>
#include <string>
#include <thread>

class RTSPStream {
public:
    RTSPStream() {};

    ~RTSPStream() {
        running_ = false;
        if (capture_thread_.joinable()) {
            capture_thread_.join();
            std::cout << "Try to close stream from " + ip_address_ + ":" + port_
                      << std::endl;
            stream_.release();
            std::cout << "Stream succesfully closed!" << std::endl;
        }
    };

    void SetLogin(const std::string&);

    void SetPassword(const std::string&);

    void SetIpAddress(const std::string&);

    void SetPort(const std::string&);

    void SetSource(const std::string&);

    bool Initialize();

    bool Connect(int timeout_ms = 5000);

    void Reconnect();

    void CaptureLoop();

    bool IsRunning() { return running_; };

    bool IsConnected() { return connected_; };

    cv::Mat GetFrame();

    RTSPStream(const RTSPStream&) = delete;
    RTSPStream& operator=(const RTSPStream&) = delete;

private:
    std::string login_ = "";
    std::string password_ = "";
    std::string ip_address_ = "";
    std::string port_ = "";
    std::string source_ = "";
    std::string rtsp_url_ = "";
    int stream_width_ = 0;
    int stream_height_ = 0;
    double stream_fps_ = 0.;
    std::string stream_full_url_ = "";

    cv::VideoCapture stream_;
    cv::Mat current_frame_;
    std::atomic<bool> running_{false};
    std::atomic<bool> connected_{false};
    std::mutex frame_mutex_;
    std::thread capture_thread_;
};
#endif  // INCLUDE_RTSPSTREAM_HPP_
