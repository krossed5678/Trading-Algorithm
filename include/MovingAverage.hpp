#pragma once
#include <vector>
#include "DataLoader.hpp"

namespace Indicators {
    // Simple Moving Average
    double SMA(const std::vector<OHLCV>& data, size_t end_index, size_t period);

    // Relative Strength Index (RSI)
    double RSI(const std::vector<OHLCV>& data, size_t end_index, size_t period);

    // Fair Value Gap (FVG) detection: returns true if a FVG is detected at end_index
    bool detectFVG(const std::vector<OHLCV>& data, size_t end_index);
}
