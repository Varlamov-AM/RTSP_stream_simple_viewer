#pragma once
#include <atomic>
#include <mutex>
#include <opencv2/opencv.hpp>
#include <stdexcept>
#include <thread>

class RTSPRecorderException : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

class RTSPRecorder {
public:
    RTSPRecorder();

    ~RTSPRecorder();

    void SetOutputPath(const std::string&);

    void SetTargetFPS(int);

    void SetFrameSize(const cv::Size&);

    bool Initialize();

    void SetFrame(const cv::Mat&);

    void RecordLoop();

private:
    std::atomic<bool> connected_{false};
    std::thread capture_thread_;
    std::mutex frame_mutex_;
    cv::VideoWriter video_writer_;
    cv::Mat frame_;
    std::string output_path_;
    int target_fps_ = 0;
    cv::Size frame_size_ = cv::Size(0, 0);
};