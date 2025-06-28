#include "../include/MovingAverage.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>

namespace Indicators {
    double SMA(const std::vector<OHLCV>& data, size_t end_index, size_t period) {
        if (end_index + 1 < period) {
            std::cerr << "[DEBUG] SMA: Not enough data for period " << period << " at index " << end_index << std::endl;
            return 0.0;
        }
        double sum = 0.0;
        for (size_t i = end_index + 1 - period; i <= end_index; ++i) {
            sum += data[i].close;
        }
        return sum / period;
    }

    double RSI(const std::vector<OHLCV>& data, size_t end_index, size_t period) {
        if (end_index < period) {
            std::cerr << "[DEBUG] RSI: Not enough data for period " << period << " at index " << end_index << std::endl;
            return 50.0;
        }
        double gain = 0.0, loss = 0.0;
        for (size_t i = end_index - period + 1; i <= end_index; ++i) {
            if (i == 0) continue; // Prevent out-of-bounds
            double change = data[i].close - data[i - 1].close;
            if (change > 0) gain += change;
            else loss -= change;
        }
        if (gain + loss == 0) return 50.0;
        double rs = gain / (loss == 0 ? 1e-10 : loss);
        return 100.0 - (100.0 / (1.0 + rs));
    }

    bool detectFVG(const std::vector<OHLCV>& data, size_t end_index) {
        if (end_index < 2) {
            std::cerr << "[DEBUG] FVG: Not enough data at index " << end_index << std::endl;
            return false;
        }
        const auto& prev = data[end_index - 1];
        const auto& curr = data[end_index];
        if (curr.low > prev.high) return true;
        if (curr.high < prev.low) return true;
        return false;
    }
}