#include "../include/DataLoader.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <exception>

std::vector<OHLCV> DataLoader::loadCSV(const std::string& filename) {
    std::vector<OHLCV> data;
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "[ERROR] Could not open file: " << filename << std::endl;
        return data;
    }
    std::string line;
    // Skip header
    if (!std::getline(file, line)) {
        std::cerr << "[ERROR] File is empty or missing header: " << filename << std::endl;
        return data;
    }
    size_t line_num = 1;
    while (std::getline(file, line)) {
        ++line_num;
        std::stringstream ss(line);
        std::string item;
        OHLCV bar;
        try {
            // Timestamp
            if (!std::getline(ss, bar.timestamp, ',')) throw std::runtime_error("Missing timestamp");
            // Open
            if (!std::getline(ss, item, ',')) throw std::runtime_error("Missing open");
            bar.open = std::stod(item);
            // High
            if (!std::getline(ss, item, ',')) throw std::runtime_error("Missing high");
            bar.high = std::stod(item);
            // Low
            if (!std::getline(ss, item, ',')) throw std::runtime_error("Missing low");
            bar.low = std::stod(item);
            // Close
            if (!std::getline(ss, item, ',')) throw std::runtime_error("Missing close");
            bar.close = std::stod(item);
            // Volume
            if (!std::getline(ss, item, ',')) throw std::runtime_error("Missing volume");
            bar.volume = std::stod(item);
            data.push_back(bar);
        } catch (const std::exception& e) {
            std::cerr << "[ERROR] Parse error on line " << line_num << ": " << e.what() << std::endl;
            std::cerr << "  Line content: " << line << std::endl;
            // Optionally: continue; or break;
        }
    }
    std::cerr << "[DEBUG] Finished loading CSV. Total bars: " << data.size() << std::endl;
    return data;
}