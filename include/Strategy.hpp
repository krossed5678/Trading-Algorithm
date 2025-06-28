#pragma once
#include <vector>
#include <string>
#include <chrono>
#include <ctime>
#include "DataLoader.hpp"

enum class SignalType {
    NONE,
    BUY,
    SELL
};

struct TradeSignal {
    SignalType type;
    size_t index;
    double stop_loss;
    double take_profit;
    std::string reason; // For logging/analysis
};

class Strategy {
public:
    virtual ~Strategy() = default;
    // Generate a signal for the current bar
    virtual TradeSignal generateSignal(const std::vector<OHLCV>& data, size_t current_index) = 0;
    
    // New method to calculate dynamic SMA periods based on data date range
    static std::pair<size_t, size_t> calculateDynamicPeriods(const std::vector<OHLCV>& data);
    
protected:
    // Helper method to parse timestamp and calculate time differences
    static std::chrono::system_clock::time_point parseTimestamp(const std::string& timestamp);
    static double calculateDaysBetween(const std::string& start_timestamp, const std::string& end_timestamp);
};

// Forward declaration for the advanced strategy
class GoldenFoundationStrategy;

// Factory function for GUI
Strategy* createGoldenFoundationStrategy(double risk_reward = 3.0);
