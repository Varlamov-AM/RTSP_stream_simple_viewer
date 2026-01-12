#include <signal.h>

#include <atomic>
#include <filesystem>
#include <iostream>

#include "RTSPConfig.hpp"
#include "RTSPRecorder.hpp"
#include "RTSPStream.hpp"

std::atomic<bool> stop_processing(false);

void SignalHandler(int signal) {
    std::cout << "\nReceived interrupt signal. Stopping..." << std::endl;
    stop_processing = true;
}

void PrintUsage() {
    std::cout << "Usage: RTSPProcessor [options]" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  --config CONFIG/PATH \
Path to config file of RTSP Processor."
              << std::endl;
    std::cout << "  --login LOGIN        \
RTSP stream login (required)"
              << std::endl;
    std::cout << "  --password PSSWORD   \
RTSP stream password (required)"
              << std::endl;
    std::cout << "  --ip_address IP      \
RTSP stream IP (required)"
              << std::endl;
    std::cout << "  --port PORT          \
RTSP stream port (required)"
              << std::endl;
    std::cout << "  --source SOURCE      \
RTSP stream source (required)"
              << std::endl;
    std::cout << "  --output PATH        \
Path to output video file (optional)"
              << std::endl;
    std::cout << "  --display            \
Enable video display on running"
              << std::endl;
    std::cout << "  --help               \
Show this help message"
              << std::endl;
}

int main(int argc, char* argv[]) {
    signal(SIGINT, SignalHandler);

    const int kCellW = 640;
    const int kCellH = 360;

    const int kFramePeriodMks = 49500;

    const int kEscCode = 27;

    std::cout << "RTSP Stream Processor" << std::endl;
    std::cout
        << "Press Ctrl+C in terminal, ESC or 'q' on window to stop processing."
        << std::endl
        << "Press 'r' to force reconnection for all streams." << std::endl;
    std::cout << "---------------------------------------------" << std::endl;

    std::string login = "";
    std::string password = "";
    std::string ip_address = "";
    std::string port = "";
    std::string source = "";
    std::string output_path = "";
    bool display = false;
    RTSPConfig config;

    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--login" && i + 1 < argc) {
            login = argv[++i];
        } else if (arg == "--password" && i + 1 < argc) {
            password = argv[++i];
        } else if (arg == "--ip_address" && i + 1 < argc) {
            ip_address = argv[++i];
        } else if (arg == "--port" && i + 1 < argc) {
            port = argv[++i];
        } else if (arg == "--source" && i + 1 < argc) {
            source = argv[++i];
        } else if (arg == "--output" && i + 1 < argc) {
            output_path = argv[++i];
        } else if (arg == "--display") {
            display = true;
        } else if (arg == "--config" && i + 1 < argc) {
            config = RTSPConfig(std::string(argv[i + 1]));
            bool configuration_success = config.Initialize();
            if (!configuration_success) {
                return 0;
            }
        } else if (arg == "--help") {
            PrintUsage();
            return 0;
        }
    }

    std::vector<std::unordered_map<std::string, std::string>> streams =
        config.GetStreamCredentials();

    std::vector<std::unique_ptr<RTSPStream>> rtsp_streams;
    rtsp_streams.reserve(streams.size());

    for (auto stream : streams) {
        rtsp_streams.push_back(std::move(std::make_unique<RTSPStream>()));

        rtsp_streams.back().get()->SetLogin(stream["login"]);
        rtsp_streams.back().get()->SetPassword(stream["password"]);
        rtsp_streams.back().get()->SetIpAddress(stream["ip_address"]);
        rtsp_streams.back().get()->SetPort(stream["port"]);
        rtsp_streams.back().get()->SetSource(stream["source"]);

        rtsp_streams.back().get()->Initialize();
    }

    std::vector<cv::Mat> frames(rtsp_streams.size() + 1);
    frames.back() = cv::Mat(kCellW * 2, kCellH * 2, CV_8UC3);
    cv::resize(frames.back(), frames.back(), {kCellW * 2, kCellH * 2});

    int stream_idx = 0;

    while (!stop_processing) {
        auto start = std::chrono::high_resolution_clock::now();
        for (const auto& rtsp_stream : rtsp_streams) {
            if (rtsp_stream.get()->IsConnected()) {
                frames[stream_idx] = rtsp_streams[stream_idx].get()->GetFrame();
                if (rtsp_streams.size() > 1) {
                    cv::resize(frames[stream_idx], frames[stream_idx],
                               {kCellW, kCellH});
                    frames[stream_idx].copyTo(frames.back()(
                        cv::Rect(stream_idx / 2 * kCellW,
                                 stream_idx % 2 * kCellH, kCellW, kCellH)));
                } else if (rtsp_streams.size() == 1) {
                    cv::resize(frames[stream_idx], frames[stream_idx],
                               {1280, 720});
                }
            }
            ++stream_idx;
        }
        stream_idx = 0;

        cv::line(frames.back(), cv::Point(kCellW, 0),
                 cv::Point(kCellW, kCellH * 2), cv::Scalar(0, 0, 0), 2);
        cv::line(frames.back(), cv::Point(0, kCellH),
                 cv::Point(kCellW * 2, kCellH), cv::Scalar(0, 0, 0), 2);

        if (display) {
            if (!frames.back().empty()) {
                if (streams.size() == 1) {
                    cv::imshow("RTSP streams", frames[0]);
                } else {
                    cv::imshow("RTSP streams", frames.back());
                }
                int key = cv::waitKey(5) & 0xFF;
                if (key == kEscCode || key == 'q') {
                    stop_processing = true;
                } else if (key == 'r') {
                    for (const auto& rtsp_stream : rtsp_streams) {
                        rtsp_stream.get()->RequestReconnect();
                    }
                }

                // Check if window was closed
                if (cv::getWindowProperty("RTSP streams",
                                          cv::WND_PROP_VISIBLE) < 1) {
                    stop_processing = true;
                }
            }
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration =
            std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        if (duration.count() < kFramePeriodMks) {
            std::this_thread::sleep_for(
                std::chrono::microseconds(kFramePeriodMks - duration.count()));
        }
    }

    return 0;
}