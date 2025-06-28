#pragma once
#include <vector>
#include <string>
#include "DataLoader.hpp"

enum class SignalType {
    NONE,
    BUY,
    SELL
};

struct TradeSignal {
    SignalType type;
    size_t bar_index; // Index in the OHLCV data
    double stop_loss;
    double take_profit;
    std::string reason; // For logging/analysis
};

// Forward declaration for the advanced strategy
class GoldenFoundationStrategy;

class Strategy {
public:
    virtual ~Strategy() = default;
    // Generate a signal for the current bar
    virtual TradeSignal generateSignal(const std::vector<OHLCV>& data, size_t current_index) = 0;
};
