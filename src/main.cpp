#include <iostream>
#include <filesystem>
#include <signal.h>
#include <atomic>
#include <opencv2/opencv.hpp>

namespace fs = std::filesystem;

std::atomic<bool> stop_processing(false);

void signalHandler(int signal) {
    std::cout << "\nReceived interrupt signal. Stopping..." << std::endl;
    stop_processing = true;
}

void printUsage() {
    std::cout << "Usage: RTSPProcessor [options]" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  --rtsp URL           RTSP stream URL (required)" << std::endl;
    std::cout << "  --output PATH        Path to output video file (optional)" << std::endl;
    std::cout << "  --no-display         Disable video display" << std::endl;
    std::cout << "  --fps FPS            Target FPS for processing (default: 30)" << std::endl;
    std::cout << "  --help               Show this help message" << std::endl;
    std::cout << "\nExample:" << std::endl;
    std::cout << "  ./RTSPProcessor --rtsp \"rtsp://username:password@192.168.1.100:554/stream\"" << std::endl;
}

int main(int argc, char* argv[]) {
    std::string rtsp_url = "";
    std::string output_path = "";
    bool display = true;;
    int target_fps = 25;
    
    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--rtsp" && i + 1 < argc) {
            rtsp_url = argv[++i];
        } else if (arg == "--output" && i + 1 < argc) {
            output_path = argv[++i];
        } else if (arg == "--no-display") {
            display = false;
        } else if (arg == "--help") {
            printUsage();
            return 0;
        }
    }
    
    // Validate required parameters
    if (rtsp_url.empty()) {
        std::cerr << "Error: RTSP URL is required!" << std::endl;
        printUsage();
        return -1;
    }
    
    // Setup signal handler
    signal(SIGINT, signalHandler);
    
    std::cout << "ONNX RTSP Stream Processor" << std::endl;
    std::cout << "RTSP URL: " << rtsp_url << std::endl;
    std::cout << "Press Ctrl+C to stop processing" << std::endl;
    std::cout << "------------------------" << std::endl;
    
    try {
        // Open RTSP stream
        cv::VideoCapture cap(rtsp_url, cv::CAP_FFMPEG);
        if (!cap.isOpened()) {
            std::cerr << "Error: Cannot open RTSP stream: " << rtsp_url << std::endl;
            std::cerr << "Make sure the URL is correct and the camera is accessible." << std::endl;
            return -1;
        }
        
        // Get stream properties
        int frame_width = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
        int frame_height = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));
        double stream_fps = cap.get(cv::CAP_PROP_FPS);
        
        std::cout << "Stream opened successfully!" << std::endl;
        std::cout << "Resolution: " << frame_width << "x" << frame_height << std::endl;
        std::cout << "Stream FPS: " << stream_fps << std::endl;
        
        // Setup video writer if output path is specified
        cv::VideoWriter video_writer;
        if (!output_path.empty()) {
            video_writer.open(output_path, 
                            cv::VideoWriter::fourcc('X', '2', '6', '4'),
                            target_fps, 
                            cv::Size(frame_width, frame_height));
            if (!video_writer.isOpened()) {
                std::cerr << "Warning: Could not open video writer. Continuing without recording." << std::endl;
            }
        }
        
        cv::Mat frame;
        cv::Mat nframe;
        int frame_count = 0;
        double total_inference_time = 0;
        auto start_time = std::chrono::steady_clock::now();
        
        // Calculate delay between frames based on target FPS
        int delay_ms = 1000 / target_fps;
        
        while (!stop_processing) {
            // Read frame from RTSP stream
            if (!cap.read(frame)) {
                std::cerr << "Error: Failed to read frame from RTSP stream!" << std::endl;
                break;
            }
            
            if (frame.empty()) {
                std::cerr << "Warning: Empty frame received!" << std::endl;
                continue;
            }
            
            // Write frame to output video if recording
            if (video_writer.isOpened()) {
                video_writer.write(frame);
            }
            
            // Display frame
            if (display) {
                cv::resize(frame, nframe, cv::Size(80*16, 80*9));
                cv::imshow("RTSP Stream", nframe);
                
                // Check for ESC key or window close
                int key = cv::waitKey(delay_ms) & 0xFF;
                if (key == 27 || key == 'q') { // ESC or 'q'
                    stop_processing = true;
                }
                
                // Check if window was closed
                if (cv::getWindowProperty("RTSP Stream", cv::WND_PROP_VISIBLE) < 1) {
                    stop_processing = true;
                }
            }
            
            frame_count++;
            
            // Print statistics every 100 frames
            if (frame_count % 100 == 0) {
                auto current_time = std::chrono::steady_clock::now();
                double elapsed_seconds = std::chrono::duration<double>(current_time - start_time).count();
                double actual_fps = frame_count / elapsed_seconds;
                
                std::cout << "Processed " << frame_count << " frames | " 
                         << "FPS: " << actual_fps << " | " << std::endl;
            }
        }
        
        // Calculate final statistics
        auto end_time = std::chrono::steady_clock::now();
        double total_seconds = std::chrono::duration<double>(end_time - start_time).count();
        double avg_fps = frame_count / total_seconds;
        double avg_inference_time = total_inference_time / frame_count;
        
        std::cout << "\n=== Processing Summary ===" << std::endl;
        std::cout << "Total frames processed: " << frame_count << std::endl;
        std::cout << "Total time: " << total_seconds << " seconds" << std::endl;
        std::cout << "Average FPS: " << avg_fps << std::endl;
        
        // Cleanup
        cap.release();
        if (video_writer.isOpened()) {
            video_writer.release();
            std::cout << "Video saved to: " << output_path << std::endl;
        }
        if (display) {
            cv::destroyAllWindows();
        }
        
        std::cout << "Processing stopped gracefully." << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return -1;
    }
    
    return 0;
}