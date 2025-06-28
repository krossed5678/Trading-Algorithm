#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include <iostream>

namespace FileUtils {
    inline std::string findDataFile(const std::string& filename) {
        // Try multiple possible paths relative to executable location
        std::vector<std::string> possible_paths = {
            filename,                                    // Current directory
            "../" + filename,                           // One level up
            "../../" + filename,                        // Two levels up (build/Release/ -> project root)
            "../../../" + filename,                     // Three levels up (just in case)
            "data/" + filename,                         // data subdirectory
            "../data/" + filename,                      // data subdirectory one level up
            "../../data/" + filename,                   // data subdirectory two levels up
        };
        
        for (const auto& path : possible_paths) {
            if (std::filesystem::exists(path)) {
                std::cout << "Found data file at: " << path << std::endl;
                return path;
            }
        }
        
        // If not found, return the original path for error reporting
        return filename;
    }
} 