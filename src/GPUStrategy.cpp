#include "../include/GPUStrategy.hpp"
#include "../include/MovingAverage.hpp"
#include <iostream>
#include <chrono>
#include <iomanip>

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
    if (data.empty()) {
        std::cerr << "[ERROR] Data is empty. Aborting GPU signal computation." << std::endl;
        return;
    }
    
    auto start_time = std::chrono::high_resolution_clock::now();
    int n = static_cast<int>(data.size());
    
    // Calculate dynamic periods based on data date range
    auto periods = Strategy::calculateDynamicPeriods(data);
    size_t sma_period = periods.first;
    size_t rsi_period = periods.second;
    const double rsi_oversold = 30.0;
    
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
    
    // --- Argument validation and logging ---
    std::cout << "\n[INFO] === GPU Signal Calculation ===" << std::endl;
    std::cout << "[INFO] Bars: " << n << ", SMA period: " << sma_period << ", RSI period: " << rsi_period << std::endl;
    std::cout << "[INFO] rsi_oversold: " << rsi_oversold << ", risk_reward: " << risk_reward_ << std::endl;
    if (n <= 0 || sma_period < 2 || rsi_period < 2 || sma_period > n || rsi_period > n) {
        std::cerr << "[ERROR] Invalid arguments for GPU kernel. Skipping GPU calculation." << std::endl;
        std::cerr << "[DEBUG] n=" << n << ", sma_period=" << sma_period << ", rsi_period=" << rsi_period << std::endl;
        std::cerr << "[DEBUG] Data size: " << data.size() << std::endl;
        goto cpu_fallback;
    }
    if (!prices.data() || !sma_values_.data() || !rsi_values_.data() || !signals_.data() || !stops_.data() || !targets_.data()) {
        std::cerr << "[ERROR] Null pointer in GPU kernel arguments. Skipping GPU calculation." << std::endl;
        goto cpu_fallback;
    }
    
    std::cout << "[INFO] Launching fused CUDA kernel..." << std::endl;
    gpu_calculate_all_indicators_and_signals(
        prices.data(), n,
        sma_values_.data(), rsi_values_.data(),
        signals_.data(), stops_.data(), targets_.data(),
        static_cast<int>(sma_period), static_cast<int>(rsi_period), rsi_oversold, risk_reward_
    );
    
    // Count signals generated
    int signal_count = 0;
    for (int i = 0; i < n; i++) {
        if (signals_[i] == 1) signal_count++;
    }
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    std::cout << "[INFO] GPU generated " << signal_count << " signals in " << duration.count() << "ms using dynamic periods" << std::endl;
    
    if (signal_count == 0) {
        std::cerr << "[WARNING] GPU generated 0 signals. Falling back to CPU calculation." << std::endl;
        goto cpu_fallback;
    }
    precomputed_ = true;
    std::cout << "[INFO] Signal computation complete!\n" << std::endl;
    return;

cpu_fallback:
    // --- CPU fallback ---
    std::cout << "[INFO] === CPU Signal Calculation (Fallback) ===" << std::endl;
    Indicators::calculateBatchIndicators(data, sma_values_, rsi_values_, sma_period, rsi_period);
    for (int i = 0; i < n; i++) {
        if (i < std::max(sma_period, rsi_period)) {
            signals_[i] = 0;
            stops_[i] = 0.0;
            targets_[i] = 0.0;
            continue;
        }
        bool uptrend = prices[i] > sma_values_[i];
        bool oversold = rsi_values_[i] < rsi_oversold;
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
    int cpu_signal_count = 0;
    for (int i = 0; i < n; i++) {
        if (signals_[i] == 1) cpu_signal_count++;
    }
    std::cout << "[INFO] CPU generated " << cpu_signal_count << " signals using dynamic periods" << std::endl;
    precomputed_ = true;
    std::cout << "[INFO] Signal computation complete!\n" << std::endl;
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
            "GPU+CPU: Uptrend, RSI<30, FVG (Dynamic periods)"
        };
    }
    return {SignalType::NONE, current_index, 0.0, 0.0, "GPU+CPU: No setup"};
}

// Factory function implementation
Strategy* createGPUGoldenFoundationStrategy(double risk_reward) {
    return new GPUGoldenFoundationStrategy(risk_reward);
} 