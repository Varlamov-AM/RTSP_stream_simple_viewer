#include "RTSPRecorder.hpp"

RTSPRecorder::RTSPRecorder() {}

RTSPRecorder::~RTSPRecorder() {
    connected_ = false;
    if (capture_thread_.joinable()) {
        capture_thread_.join();
        if (!video_writer_.isOpened()) {
            video_writer_.release();
        }
    }
};

void RTSPRecorder::SetOutputPath(const std::string& output) {
    output_path_ = output;
}

void RTSPRecorder::SetTargetFPS(int targer_fps) { target_fps_ = targer_fps; }

void RTSPRecorder::SetFrameSize(const cv::Size& frame_size) {
    frame_size_ = frame_size;
}

bool RTSPRecorder::Initialize() {
    try {
        if (output_path_.empty()) {
            throw RTSPRecorderException("Output path is empty!");
        }
        if (target_fps_ == 0) {
            throw RTSPRecorderException("Target fps is not set!");
        }
        if (frame_size_ == cv::Size(0, 0)) {
            throw RTSPRecorderException("Frame size is not set!");
        }
        video_writer_.open(output_path_,
                           cv::VideoWriter::fourcc('a', 'v', 'c', '1'),
                           target_fps_, frame_size_);
        if (!video_writer_.isOpened()) {
            throw RTSPRecorderException("failed to create a video recorder!");
        }
        capture_thread_ = std::thread(&RTSPRecorder::RecordLoop, this);
        connected_ = true;
        return connected_;
    } catch (const RTSPRecorderException& e) {
        std::cerr << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Something went wrong..." << std::endl;
    }
    return false;
}

void RTSPRecorder::SetFrame(const cv::Mat& frame) { frame_ = frame.clone(); }

void RTSPRecorder::RecordLoop() {
    while (connected_) {
        if (video_writer_.isOpened()) {
            std::lock_guard<std::mutex> lock(frame_mutex_);
            video_writer_.write(frame_);
        } else {
            connected_ = false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(45));  // ~20 FPS
    }
}