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

// Factory function for GUI
Strategy* createGoldenFoundationStrategy(double risk_reward = 3.0);

class GoldenFoundationStrategy : public Strategy {
public:
    GoldenFoundationStrategy(double risk_reward = 3.0);
    void setSMA(int period) { sma_period_ = period; }
    void setRSI(int period, double oversold) { rsi_period_ = period; rsi_oversold_ = oversold; }
    TradeSignal generateSignal(const std::vector<OHLCV>& data, size_t current_index) override;
    void precomputeSignals(const std::vector<OHLCV>& data);
private:
    double risk_reward_ = 3.0;
    std::vector<double> sma_values_;
    std::vector<double> rsi_values_;
    std::vector<int> signals_;
    std::vector<double> stops_;
    std::vector<double> targets_;
    bool precomputed_ = false;
    size_t sma_period_ = 20;
    size_t rsi_period_ = 7;
    double rsi_oversold_ = 30.0;
};
