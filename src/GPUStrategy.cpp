#include "../include/GPUStrategy.hpp"
#include <iostream>

GPUGoldenFoundationStrategy::GPUGoldenFoundationStrategy(double risk_reward)
    : risk_reward_(risk_reward), precomputed_(false) {
}

GPUGoldenFoundationStrategy::~GPUGoldenFoundationStrategy() {
}

void GPUGoldenFoundationStrategy::precomputeSignals(const std::vector<OHLCV>& data) {
    if (data.empty()) return;
    
    int n = static_cast<int>(data.size());
    
    // Extract close prices
    std::vector<double> prices(n);
    for (int i = 0; i < n; i++) {
        prices[i] = data[i].close;
    }
    
    // Allocate result arrays
    sma_values_.resize(n);
    rsi_values_.resize(n);
    signals_.resize(n);
    stops_.resize(n);
    targets_.resize(n);
    
    // GPU parameters
    const int sma_period = 50;
    const int rsi_period = 14;
    const double rsi_oversold = 30.0;
    
    std::cout << "Computing indicators on GPU for " << n << " bars..." << std::endl;
    
    // Calculate indicators on GPU
    gpu_calculate_indicators(
        prices.data(), n,
        sma_values_.data(), rsi_values_.data(),
        sma_period, rsi_period
    );
    
    // Generate signals on GPU
    gpu_generate_signals(
        prices.data(), sma_values_.data(), rsi_values_.data(),
        n, rsi_oversold, risk_reward_,
        signals_.data(), stops_.data(), targets_.data()
    );
    
    // Check if GPU generated any signals
    int signal_count = 0;
    for (int i = 0; i < n; i++) {
        if (signals_[i] == 1) signal_count++;
    }
    
    std::cout << "GPU generated " << signal_count << " signals" << std::endl;
    
    // If no signals from GPU, fall back to CPU signal generation
    if (signal_count == 0) {
        std::cout << "No GPU signals detected, falling back to CPU signal generation..." << std::endl;
        
        // Use CPU logic for signal generation
        for (int i = 0; i < n; i++) {
            if (i < std::max(sma_period, rsi_period)) {
                signals_[i] = 0;
                continue;
            }
            
            // Check conditions for buy signal
            bool uptrend = prices[i] > sma_values_[i];
            bool oversold = rsi_values_[i] < rsi_oversold;
            
            // Simple FVG detection (gap up/down)
            bool fvg = false;
            if (i >= 1) {
                fvg = (prices[i] > prices[i-1] * 1.01) || (prices[i] < prices[i-1] * 0.99);
            }
            
            if (uptrend && oversold && fvg) {
                signals_[i] = 1; // BUY signal
                double entry = prices[i];
                double stop_loss_pct = 0.005 / risk_reward_;
                stops_[i] = entry - (entry * stop_loss_pct);
                targets_[i] = entry + (entry - stops_[i]) * risk_reward_;
            } else {
                signals_[i] = 0; // NO signal
                stops_[i] = 0.0;
                targets_[i] = 0.0;
            }
        }
        
        // Count CPU signals
        signal_count = 0;
        for (int i = 0; i < n; i++) {
            if (signals_[i] == 1) signal_count++;
        }
        std::cout << "CPU generated " << signal_count << " signals" << std::endl;
    }
    
    precomputed_ = true;
    std::cout << "Signal computation complete!" << std::endl;
}

TradeSignal GPUGoldenFoundationStrategy::generateSignal(const std::vector<OHLCV>& data, size_t current_index) {
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
            "GPU+CPU: Uptrend, RSI<30, FVG"
        };
    }
    
    return {SignalType::NONE, current_index, 0.0, 0.0, "GPU+CPU: No setup"};
}

// Factory function implementation
Strategy* createGPUGoldenFoundationStrategy(double risk_reward) {
    return new GPUGoldenFoundationStrategy(risk_reward);
} 