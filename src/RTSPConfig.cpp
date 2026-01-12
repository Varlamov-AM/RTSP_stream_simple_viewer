#include "RTSPConfig.hpp"

#include <stdexcept>

RTSPConfig::RTSPConfig() {}

RTSPConfig::RTSPConfig(const std::string& config_path) {
    SetConfigPath(config_path);
}

RTSPConfig::~RTSPConfig() {}

std::string RTSPConfig::GetConfigPath() { return config_path_; }

void RTSPConfig::SetConfigPath(const std::string& config_path) {
    config_path_ = config_path;
}

bool RTSPConfig::Initialize() {
    std::ifstream config_file(config_path_);
    try {
        config_data_ = nlohmann::json::parse(config_file);

        std::cout
            << "Starting the config file structure verification procedure..."
            << std::endl;
        bool successful_verification = true;
        try {
            VerifyConfigStructure(config_data_);
        } catch (RTSPConfigStructureException config_structure_exception) {
            successful_verification = false;
            std::cout << "Verification is failed!" << std::endl;
            std::cout << config_structure_exception.what() << std::endl;
            return false;
        }

        if (successful_verification) {
            std::cout << std::string("The config file verification ") +
                             "procedure has been successfully completed!\n"
                      << "---------------------------------------------"
                      << std::endl;
            for (auto data : config_data_.items()) {
                if (data.key() == "rtsp_streams") {
                    for (auto streams : data.value().items()) {
                        for (auto stream : streams.value().items()) {
                            std::unordered_map<std::string, std::string>
                                stream_map;
                            for (auto stream_credentials :
                                 stream.value().items()) {
                                stream_credentials.value();
                                stream_map.insert(
                                    std::pair<std::string, std::string>(
                                        stream_credentials.key(),
                                        stream_credentials.value()));
                            }
                            streams_.push_back(stream_map);
                        }
                    }
                }
            }
        }
    } catch (nlohmann::json_abi_v3_11_3::detail::parse_error exception) {
        std::cout << exception.what() << std::endl;
        return false;
    }
    return true;
};

void RTSPConfig::VerifyConfigStructure(const nlohmann::json& config_file) {
    bool rtsp_streams_block{false};
    bool video_recorder_block{false};
    bool display_block{false};

    for (auto d : config_data_.items()) {
        if (d.key() == "rtsp_streams") {
            rtsp_streams_block = true;
            auto streams = d.value();
            for (auto stream : streams.items()) {
                auto stream_properties = stream.value();
                for (auto stream_prop : stream_properties.items()) {
                    if (stream_prop.key() == "network") {
                        bool login{false}, password{false}, ip_address{false},
                            port{false}, source{false};

                        auto stream_network_data = stream_prop.value();

                        for (auto stream_network_prop :
                             stream_network_data.items()) {
                            if (stream_network_prop.key() == "login") {
                                login = true;
                            } else if (stream_network_prop.key() ==
                                       "password") {
                                password = true;
                            } else if (stream_network_prop.key() ==
                                       "ip_address") {
                                ip_address = true;
                            } else if (stream_network_prop.key() == "port") {
                                port = true;
                            } else if (stream_network_prop.key() == "source") {
                                source = true;
                            } else {
                                throw RTSPConfigStructureException(
                                    std::string("ERROR: ") +
                                    "The structure of the configuration " +
                                    "file is incorrect:\nExtra field in " +
                                    "properties of the network for stream: " +
                                    stream_network_prop.key() +
                                    "\nCheck stream " + stream.key() +
                                    " network block!");
                            }
                        }

                        if (!(login && password && ip_address && port &&
                              source)) {
                            throw RTSPConfigStructureException(
                                std::string("ERROR: ") +
                                "The structure of the configuration file is " +
                                "incorrect:\nCheck stream " + stream.key() +
                                " network block!");
                        }
                    } else {
                        throw RTSPConfigStructureException(
                            std::string("ERROR: ") +
                            "The structure of the configuration file is " +
                            "incorrect:\nExtra fields in properties of the " +
                            "stream: " + stream_prop.key() + " block!");
                    }
                }
            }
        } else if (d.key() == "video_recorder") {
            video_recorder_block = true;
            bool record_video_flag{false}, record_path_flag{false};
            auto video_recorder_data = d.value().items();
            for (auto video_recorder_prop : video_recorder_data) {
                if (video_recorder_prop.key() == "record_video") {
                    record_video_flag = true;
                } else if (video_recorder_prop.key() == "video_path") {
                    record_path_flag = true;
                } else {
                    throw RTSPConfigStructureException(
                        std::string("ERROR: ") +
                        "The structure of the configuration file is " +
                        "incorrect:\nExtra fields in properties of the " +
                        "video_recorder block: " + video_recorder_prop.key());
                }
            }

            if (!(record_video_flag && record_path_flag)) {
                throw RTSPConfigStructureException(
                    std::string("ERROR: ") +
                    "Check structure of video recorder block!");
            }

        } else if (d.key() == "display") {
            display_block = true;
            bool display_streams_flag{false}, window_flag{false};
            auto display_properties = d.value().items();

            for (auto display_prop : display_properties) {
                if (display_prop.key() == "display_streams") {
                    display_streams_flag = true;
                } else if (display_prop.key() == "window") {
                    window_flag = true;
                    bool width_flag{false}, height_flag{false},
                        grid_flag{false};
                    auto window_properties = display_prop.value().items();

                    for (auto window_prop : window_properties) {
                        if (window_prop.key() == "width") {
                            width_flag = true;
                        } else if (window_prop.key() == "height") {
                            height_flag = true;
                        } else if (window_prop.key() == "grid") {
                            grid_flag = true;
                            bool col_flag{false}, row_flag{false};
                            auto grid_properties = window_prop.value().items();

                            for (auto grid_prop : grid_properties) {
                                if (grid_prop.key() == "col") {
                                    col_flag = true;
                                } else if (grid_prop.key() == "row") {
                                    row_flag = true;
                                } else {
                                    throw RTSPConfigStructureException(
                                        std::string("ERROR: ") +
                                        "The structure of the configuration " +
                                        "file is incorrect:\n" +
                                        "Extra fields in properties of the " +
                                        "window grid block: " +
                                        grid_prop.key());
                                }
                            }

                            if (!(col_flag && row_flag)) {
                                throw RTSPConfigStructureException(
                                    std::string("ERROR: ") +
                                    "The structure of the configuration file" +
                                    " is incorrect:\n" +
                                    "Check display window block!");
                            }
                        } else {
                            throw RTSPConfigStructureException(
                                std::string("ERROR: ") +
                                "The structure of the configuration " +
                                "file is incorrect:\n" +
                                "Extra fields in properties of the " +
                                "window block: " + window_prop.key());
                        }
                    }

                    if (!(width_flag && height_flag && grid_flag)) {
                        throw RTSPConfigStructureException(
                            std::string("ERROR: ") +
                            "The structure of the configuration " +
                            "file is incorrect:\n" + "Check window block!");
                    }
                } else {
                    throw RTSPConfigStructureException(
                        std::string("ERROR: ") +
                        "The structure of the configuration file is " +
                        "incorrect:\n" + "Extra fields in properties of " +
                        "display block: " + display_prop.key());
                }
            }

            if (!(display_streams_flag && window_flag)) {
                std::cout << display_streams_flag << " " << window_flag
                          << std::endl;
                throw RTSPConfigStructureException(
                    std::string("ERROR: ") +
                    "The structure of the configuration file is " +
                    "incorrect:\n" + "Check display block");
            }
        } else {
            std::cout << "WARNING: An unknown option " << d.key() << " has "
                      << "been detected in the configuration file and will be "
                      << "ignored during " << "configuration !" << std::endl;
        }
    };

    if (!rtsp_streams_block) {
        throw RTSPConfigStructureException(
            std::string("ERROR: No information about rtsp streams was ") +
            "found in the configuration file.");
    }
    if (!video_recorder_block) {
        std::cout << "WARNING: No information about video recorder was"
                  << "found in the configuration file. Videorecorder is not "
                  << "created by default!" << std::endl;
    }
    if (!display_block) {
        std::cout << "WARNING: No information about display was "
                  << "found in the configuration file. The program does not "
                  << "display the streams "
                  << "by default !" << std::endl;
    }
}

std::vector<std::unordered_map<std::string, std::string>>
RTSPConfig::GetStreamCredentials() {
    return streams_;
}