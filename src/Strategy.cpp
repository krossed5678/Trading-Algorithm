#include "../include/Strategy.hpp"
#include "../include/MovingAverage.hpp"
#include <algorithm>
#include <iostream>
#include <vector>
#include <iomanip>
#include <sstream>

// Helper function to parse timestamp string to time_point
std::chrono::system_clock::time_point Strategy::parseTimestamp(const std::string& timestamp) {
    std::tm tm = {};
    std::istringstream ss(timestamp);
    ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
    return std::chrono::system_clock::from_time_t(std::mktime(&tm));
}

// Helper function to calculate days between two timestamps
double Strategy::calculateDaysBetween(const std::string& start_timestamp, const std::string& end_timestamp) {
    auto start_time = parseTimestamp(start_timestamp);
    auto end_time = parseTimestamp(end_timestamp);
    auto duration = std::chrono::duration_cast<std::chrono::hours>(end_time - start_time);
    return duration.count() / 24.0; // Convert hours to days
}

// Calculate dynamic SMA and RSI periods based on data date range
std::pair<size_t, size_t> Strategy::calculateDynamicPeriods(const std::vector<OHLCV>& data) {
    if (data.size() < 2) {
        std::cout << "Not enough data for dynamic period calculation, using defaults" << std::endl;
        return {50, 14}; // Default periods
    }
    
    // Get start and end dates
    std::string start_date = data.front().timestamp;
    std::string end_date = data.back().timestamp;
    
    // Calculate total days in the dataset
    double total_days = calculateDaysBetween(start_date, end_date);
    
    std::cout << "Data spans from " << start_date << " to " << end_date << std::endl;
    std::cout << "Total days in dataset: " << total_days << std::endl;
    
    // Calculate dynamic periods based on data span
    // For SMA: Use 1/3 of the total trading days (assuming ~252 trading days per year)
    // For RSI: Use 1/20 of the total trading days
    size_t sma_period = static_cast<size_t>(std::max(20.0, total_days / 3.0));
    size_t rsi_period = static_cast<size_t>(std::max(7.0, total_days / 20.0));
    
    // Cap periods to reasonable limits
    sma_period = std::min(sma_period, static_cast<size_t>(200));
    rsi_period = std::min(rsi_period, static_cast<size_t>(50));
    
    std::cout << "Calculated dynamic periods - SMA: " << sma_period << ", RSI: " << rsi_period << std::endl;
    
    return {sma_period, rsi_period};
}

class GoldenFoundationStrategy : public Strategy {
public:
    GoldenFoundationStrategy(double risk_reward = 3.0) : risk_reward_(risk_reward), precomputed_(false) {}
    TradeSignal generateSignal(const std::vector<OHLCV>& data, size_t current_index) override;
    
    // Pre-compute all signals for better performance
    void precomputeSignals(const std::vector<OHLCV>& data);
    
private:
    double risk_reward_;
    std::vector<double> sma_values_;
    std::vector<double> rsi_values_;
    std::vector<int> signals_;
    std::vector<double> stops_;
    std::vector<double> targets_;
    bool precomputed_;
    size_t sma_period_;
    size_t rsi_period_;
};

void GoldenFoundationStrategy::precomputeSignals(const std::vector<OHLCV>& data) {
    if (data.empty()) return;
    
    int n = static_cast<int>(data.size());
    
    // Calculate dynamic periods based on data date range
    auto periods = calculateDynamicPeriods(data);
    sma_period_ = periods.first;
    rsi_period_ = periods.second;
    
    const double rsi_oversold = 30.0;
    
    // Allocate arrays
    sma_values_.resize(n);
    rsi_values_.resize(n);
    signals_.resize(n);
    stops_.resize(n);
    targets_.resize(n);
    
    std::cout << "Computing indicators on CPU for " << n << " bars with dynamic periods..." << std::endl;
    std::cout << "Using SMA period: " << sma_period_ << ", RSI period: " << rsi_period_ << std::endl;
    
    // Pre-compute all indicators
    for (int i = 0; i < n; i++) {
        sma_values_[i] = Indicators::SMA(data, i, sma_period_);
        rsi_values_[i] = Indicators::RSI(data, i, rsi_period_);
    }
    
    // Pre-compute all signals
    int signal_count = 0;
    for (int i = 0; i < n; i++) {
        if (i < std::max(sma_period_, rsi_period_)) {
            signals_[i] = 0;
            stops_[i] = 0.0;
            targets_[i] = 0.0;
            continue;
        }
        
        // Check conditions for buy signal
        bool uptrend = data[i].close > sma_values_[i];
        bool oversold = rsi_values_[i] < rsi_oversold;
        
        // FVG detection
        bool fvg = Indicators::detectFVG(data, i);
        
        if (uptrend && oversold && fvg) {
            signals_[i] = 1; // BUY signal
            double entry = data[i].close;
            double stop_loss_pct = 0.005 / risk_reward_;
            stops_[i] = entry - (entry * stop_loss_pct);
            targets_[i] = entry + (entry - stops_[i]) * risk_reward_;
            signal_count++;
        } else {
            signals_[i] = 0; // NO signal
            stops_[i] = 0.0;
            targets_[i] = 0.0;
        }
    }
    
    std::cout << "CPU generated " << signal_count << " signals using dynamic periods" << std::endl;
    precomputed_ = true;
}

TradeSignal GoldenFoundationStrategy::generateSignal(const std::vector<OHLCV>& data, size_t current_index) {
    if (!precomputed_) {
        precomputeSignals(data);
    }
    
    if (current_index >= signals_.size()) {
        return {SignalType::NONE, current_index, 0.0, 0.0, "Index out of range"};
    }
    
    if (signals_[current_index] == 1) {
        return {
            SignalType::BUY,
            current_index,
            stops_[current_index],
            targets_[current_index],
            "CPU: Uptrend, RSI<30, FVG (Dynamic periods)"
        };
    }
    
    return {SignalType::NONE, current_index, 0.0, 0.0, "CPU: No setup"};
}

// Factory function implementation
Strategy* createGoldenFoundationStrategy(double risk_reward) {
    return new GoldenFoundationStrategy(risk_reward);
}