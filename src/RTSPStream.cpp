#include "RTSPStream.hpp"

RTSPStream::RTSPStream() {}

RTSPStream::~RTSPStream() {
    running_ = false;
    if (capture_thread_.joinable()) {
        capture_thread_.join();
        std::cout << "Try to close stream from " + ip_address_ + ":" + port_
                  << std::endl;
        stream_.release();
        std::cout << "Stream succesfully closed!" << std::endl;
    }
}

void RTSPStream::SetLogin(const std::string& login) { login_ = login; }

void RTSPStream::SetPassword(const std::string& password) {
    password_ = password;
}

void RTSPStream::SetIpAddress(const std::string& ip_address) {
    ip_address_ = ip_address;
}

void RTSPStream::SetPort(const std::string& port) { port_ = port; }

void RTSPStream::SetSource(const std::string& source) { source_ = source; }

bool RTSPStream::Initialize() {
    if (!login_.empty() && !password_.empty() && !ip_address_.empty() &&
        !port_.empty() && !source_.empty()) {
        stream_full_url_ += "rtsp://" + login_ + ":" + password_ + "@" +
                            ip_address_ + ":" + port_ + "/" + source_;
        std::cout << "Try to create a rtsp stream from " + ip_address_ + ":" +
                         port_ + "/" + source_
                  << std::endl;

        putenv(std::string("OPENCV_FFMPEG_CAPTURE_OPTIONS=rtsp_transport;tcp")
                   .data());

        if (Connect()) {
            std::cout << "Succesfully create rtsp stream from " + ip_address_ +
                             ":" + port_ + "/" + source_
                      << std::endl;

            stream_width_ =
                static_cast<int>(stream_.get(cv::CAP_PROP_FRAME_WIDTH));
            stream_height_ =
                static_cast<int>(stream_.get(cv::CAP_PROP_FRAME_HEIGHT));
            stream_fps_ = stream_.get(cv::CAP_PROP_FPS);

            std::cout << "Stream frame params: "
                      << "Frame size = (" << stream_width_ << "x"
                      << stream_height_ << ") and FPS = " << stream_fps_
                      << std::endl;

            cv::Mat test_frame;
            if (stream_.read(test_frame)) {
                running_ = true;
                connected_ = true;
                current_frame_ = test_frame;

                capture_thread_ = std::thread(&RTSPStream::CaptureLoop, this);
                return true;
            } else {
                std::cout << "Failed to read test frame of stream from " +
                                 ip_address_ + ":" + port_
                          << std::endl;
                return false;
            }
        } else {
            std::cout << "Failed to connect to stream from " + ip_address_ +
                             ":" + port_
                      << std::endl;
            return false;
        }

    } else {
        std::cout << std::string(
                         "Error initializing the stream! Some stream data is "
                         "incorrect!\n") +
                         std::string(
                             "Check stream credentials and network params!")

                  << std::endl;
        return false;
    }
}

bool RTSPStream::Connect(int timeout_ms) {
    try {
        stream_ = cv::VideoCapture(stream_full_url_, cv::CAP_FFMPEG);
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
    }

    if (!stream_.isOpened()) {
        return false;
    }
    connected_ = true;
    return true;
};

void RTSPStream::Reconnect() {
    std::lock_guard<std::mutex> lock(frame_mutex_);
    connected_ = false;

    if (stream_.isOpened()) {
        stream_.release();
    }

    std::cout << "Try to reconnect to stream from " << ip_address_ + ":" + port_
              << std::endl;

    for (int attempt = 0; attempt < reconnect_attempts_; ++attempt) {
        std::this_thread::sleep_for(
            std::chrono::milliseconds(reconnect_times_[attempt]));

        try {
            stream_.open(stream_full_url_, cv::CAP_FFMPEG);
            stream_.set(cv::CAP_PROP_OPEN_TIMEOUT_MSEC, open_timeout_ms_);
            stream_.set(cv::CAP_PROP_READ_TIMEOUT_MSEC, read_timeout_ms_);

            if (stream_.isOpened()) {
                std::cout << "Reconnected successfully to "
                          << ip_address_ + ":" + port_ << " on attempt "
                          << attempt + 1 << std::endl;
                connected_ = true;
                return;
            }
        } catch (const std::exception& e) {
            std::cerr << "Reconnect attempt " << attempt + 1
                      << " failed: " << e.what() << std::endl;
        }
    }

    if (stream_.isOpened()) {
        std::cout << "Successfully reconnected to: "
                  << ip_address_ + ":" + port_ << std::endl;
        connected_ = true;
    } else {
        std::cout << "Reconnect failed..." << std::endl;
    }
}

void RTSPStream::RequestReconnect() { reconnect_requested_ = true; }

void RTSPStream::CaptureLoop() {
    while (running_) {
        if (reconnect_requested_) {
            reconnect_requested_ = false;
            Reconnect();
        }
        cv::Mat frame;
        if (stream_.read(frame)) {
            std::lock_guard<std::mutex> lock(frame_mutex_);
            current_frame_ = frame.clone();
        } else {
            connected_ = false;
            Reconnect();
        }
    }
}

bool RTSPStream::IsRunning() { return running_; }

bool RTSPStream::IsConnected() { return connected_; }

cv::Mat RTSPStream::GetFrame() {
    std::lock_guard<std::mutex> lock(frame_mutex_);
    if (current_frame_.empty()) {
        return cv::Mat::zeros(stream_width_, stream_height_, CV_8UC3);
    }
    return current_frame_.clone();
}
