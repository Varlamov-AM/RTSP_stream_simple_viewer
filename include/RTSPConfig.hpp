#pragma once
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>

#include "nlohmann/json.hpp"

class RTSPConfigStructureException : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

class RTSPConfig {
public:
    RTSPConfig();

    RTSPConfig(const std::string&);

    ~RTSPConfig();

    std::string GetConfigPath();

    void SetConfigPath(const std::string&);

    bool Initialize();

    void VerifyConfigStructure(const nlohmann::json&);

    std::vector<std::unordered_map<std::string, std::string>>
    GetStreamCredentials();

private:
    std::string config_path_;
    nlohmann::json config_data_;
    std::vector<std::unordered_map<std::string, std::string>> streams_;
};