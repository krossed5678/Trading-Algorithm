#pragma once
#include "Strategy.hpp"
#include <vector>

// GPU function declarations
extern "C" {
    void gpu_calculate_indicators(
        const double* prices, int n,
        double* sma, double* rsi,
        int sma_period, int rsi_period
    );
    
    void gpu_generate_signals(
        const double* prices, const double* sma, const double* rsi,
        int n, double rsi_oversold, double risk_reward,
        int* signals, double* stops, double* targets
    );
}

class GPUGoldenFoundationStrategy : public Strategy {
public:
    GPUGoldenFoundationStrategy(double risk_reward = 3.0);
    ~GPUGoldenFoundationStrategy();
    
    TradeSignal generateSignal(const std::vector<OHLCV>& data, size_t current_index) override;
    
    // Pre-calculate all indicators and signals for the entire dataset
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

// Factory function for GPU strategy
Strategy* createGPUGoldenFoundationStrategy(double risk_reward = 3.0); 