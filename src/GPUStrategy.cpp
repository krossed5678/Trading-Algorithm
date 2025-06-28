#include "../include/GPUStrategy.hpp"
#include <iostream>
#include <chrono>

// Declare the new optimized CUDA function
extern "C" void gpu_calculate_all_indicators_and_signals(
    const double* prices, int n,
    double* sma, double* rsi, int* signals, double* stops, double* targets,
    int sma_period, int rsi_period, double rsi_oversold, double risk_reward
);

GPUGoldenFoundationStrategy::GPUGoldenFoundationStrategy(double risk_reward)
    : risk_reward_(risk_reward), precomputed_(false) {
}

GPUGoldenFoundationStrategy::~GPUGoldenFoundationStrategy() {
}

void GPUGoldenFoundationStrategy::precomputeSignals(const std::vector<OHLCV>& data) {
    if (data.empty()) return;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    int n = static_cast<int>(data.size());
    
    // Extract close prices for better memory locality
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
    
    // Use the new optimized fused kernel that does everything in one GPU call
    gpu_calculate_all_indicators_and_signals(
        prices.data(), n,
        sma_values_.data(), rsi_values_.data(),
        signals_.data(), stops_.data(), targets_.data(),
        sma_period, rsi_period, rsi_oversold, risk_reward_
    );
    
    // Count signals generated
    int signal_count = 0;
    for (int i = 0; i < n; i++) {
        if (signals_[i] == 1) signal_count++;
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "GPU generated " << signal_count << " signals in " << duration.count() << "ms" << std::endl;
    
    // If no signals from GPU, fall back to optimized CPU signal generation
    if (signal_count == 0) {
        std::cout << "No GPU signals detected, falling back to optimized CPU signal generation..." << std::endl;
        
        // Use optimized CPU batch calculations
        Indicators::calculateBatchIndicators(data, sma_values_, rsi_values_, sma_period, rsi_period);
        
        // Generate signals using optimized CPU logic
        for (int i = 0; i < n; i++) {
            if (i < std::max(sma_period, rsi_period)) {
                signals_[i] = 0;
                continue;
            }
            
            // Check conditions for buy signal
            bool uptrend = prices[i] > sma_values_[i];
            bool oversold = rsi_values_[i] < rsi_oversold;
            
            // Enhanced FVG detection
            bool fvg = false;
            if (i >= 1) {
                double gap_threshold = 0.01; // 1% threshold
                fvg = (prices[i] > prices[i-1] * (1.0 + gap_threshold)) || 
                      (prices[i] < prices[i-1] * (1.0 - gap_threshold));
            }
            
            if (uptrend && oversold && fvg) {
                signals_[i] = 1; // BUY signal
                double entry = prices[i];
                double stop_loss_pct = 0.005 / risk_reward_;
                stops_[i] = entry * (1.0 - stop_loss_pct);
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