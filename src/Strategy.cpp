#include "../include/Strategy.hpp"
#include "../include/MovingAverage.hpp"
#include <algorithm>
#include <iostream>
#include <vector>

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
};

void GoldenFoundationStrategy::precomputeSignals(const std::vector<OHLCV>& data) {
    if (data.empty()) return;
    
    int n = static_cast<int>(data.size());
    const int sma_period = 50;
    const int rsi_period = 14;
    const double rsi_oversold = 30.0;
    
    // Allocate arrays
    sma_values_.resize(n);
    rsi_values_.resize(n);
    signals_.resize(n);
    stops_.resize(n);
    targets_.resize(n);
    
    std::cout << "Computing indicators on CPU for " << n << " bars..." << std::endl;
    
    // Pre-compute all indicators
    for (int i = 0; i < n; i++) {
        sma_values_[i] = Indicators::SMA(data, i, sma_period);
        rsi_values_[i] = Indicators::RSI(data, i, rsi_period);
    }
    
    // Pre-compute all signals
    int signal_count = 0;
    for (int i = 0; i < n; i++) {
        if (i < std::max(sma_period, rsi_period)) {
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
    
    std::cout << "CPU generated " << signal_count << " signals" << std::endl;
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
            "CPU: Uptrend, RSI<30, FVG"
        };
    }
    
    return {SignalType::NONE, current_index, 0.0, 0.0, "CPU: No setup"};
}

// Factory function implementation
Strategy* createGoldenFoundationStrategy(double risk_reward) {
    return new GoldenFoundationStrategy(risk_reward);
}