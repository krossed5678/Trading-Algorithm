#pragma once
#include <vector>
#include <string>
#include <cstdint>

struct OHLCV {
    std::string timestamp;
    double open;
    double high;
    double low;
    double close;
    double volume;
};

class DataLoader {
public:
    // Loads OHLCV data from a CSV file. Returns a vector of OHLCV bars.
    static std::vector<OHLCV> loadCSV(const std::string& filename);
};
