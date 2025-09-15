#include "../include/DataLoader.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <exception>

//debug macros
#define LOG(msg) std::cout << "[LOG] " << msg << std::endl;
#define DEBUG(msg) std::cout << "[DEBUG] " << msg << std::endl;
#define ERROR(msg) std::cerr << "[ERROR] " << msg << std::endl;

static inline std::string trim(const std::string& s) {
    std::string result = s;
    result.erase(result.begin(),
                 std::find_if(result.begin(), result.end(),
                              [](unsigned char ch){ return !std::isspace(ch); }));
    result.erase(std::find_if(result.rbegin(), result.rend(),
                              [](unsigned char ch){ return !std::isspace(ch); }).base(),
                 result.end());
    return result;
}

std::vector<OHLCV> DataLoader::loadCSV(const std::string& filename) {
    std::vector<OHLCV> data;
    LOG("Attempting to open file: " << filename);
    std::ifstream file(filename);
    if (!file.is_open()) {
        ERROR("Could not open file: " << filename);
        return data;
    }
    std::string line;
    // Skip header
    if (!std::getline(file, line)) {
        ERROR("File is empty or missing header: " << filename);
        return data;
    }
    LOG("Header found, starting to parse rows");
    size_t line_num = 1;
    size_t bad_lines = 0;
    while (std::getline(file, line)) {
        ++line_num;
        std::stringstream ss(line);
        std::string item;
        OHLCV bar;
        try {
            // Timestamps
            if (!std::getline(ss, bar.timestamp, ',')) throw std::runtime_error("Missing timestamp");
            bar.timestamp = trim(bar.timestamp);

            //O
            if (!std::getline(ss, item, ',')) throw std::runtime_error("Missing open");
            bar.open = std::stod(trim(item));

            //H
            if (!std::getline(ss, item, ',')) throw std::runtime_error("Missing high");
            bar.high = std::stod(trim(item));

            //L
            if (!std::getline(ss, item, ',')) throw std::runtime_error("Missing low");
            bar.low = std::stod(trim(item));

            //C
            if (!std::getline(ss, item, ',')) throw std::runtime_error("Missing close");
            bar.close = std::stod(trim(item));

            //Vol
            if (!std::getline(ss, item, ',')) throw std::runtime_error("Missing volume");
            bar.volume = std::stod(trim(item));

            data.push_back(bar);

            if (line_num % 10000 == 0) {
                DEBUG("Parsed " << line_num << " lines so far...");
            }
        } catch (const std::exception& e) {
            ++bad_lines;
            ERROR("Parse error on line " << line_num << ": " << e.what());
            ERROR("  Line content: " << line);
            // Skip bad line
            continue;
        }
    }

    LOG("Finished loading CSV. Total bars: " << data.size() << 
        ", Skipped bad lines: " << bad_lines);

    return data;
}