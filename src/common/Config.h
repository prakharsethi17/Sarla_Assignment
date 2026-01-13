#pragma once
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <string>

using json = nlohmann::json;

class Config {
public:
    static json load(const std::string& filepath = "config.json") {
        try {
            std::ifstream f(filepath);
            if (!f.is_open()) {
                std::cerr << "[Config] Warning: config.json not found. Using defaults.\n";
                return default_config();
            }
            json j;
            f >> j;
            return j;
        } catch (...) {
            return default_config();
        }
    }
    
    static json default_config() {
        return {
            {"server", {{"port", 9002}}},
            {"producer", {{"host", "127.0.0.1"}, {"port", 9002}}},
            {"consumer", {{"host", "127.0.0.1"}, {"port", 9002}}}
        };
    }
};
