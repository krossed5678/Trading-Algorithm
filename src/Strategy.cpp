#include "../include/Strategy.hpp"
#include "../include/MovingAverage.hpp"
#include <algorithm>
#include <iostream>

class GoldenFoundationStrategy : public Strategy {
public:
    TradeSignal generateSignal(const std::vector<OHLCV>& data, size_t current_index) override {
        // Parameters
        const size_t sma_period = 50;
        const size_t rsi_period = 14;
        const double rsi_oversold = 30.0;
        const double risk_reward = 3.0;
        // risk_ is set by GUI (default 1.0)

        if (current_index < std::max(sma_period, rsi_period))
            return {SignalType::NONE, current_index, 0.0, 0.0, "Not enough data"};

        // 1. Trend filter: SMA
        double sma = Indicators::SMA(data, current_index, sma_period);
        bool uptrend = data[current_index].close > sma;

        // 2. RSI filter
        double rsi = Indicators::RSI(data, current_index, rsi_period);
        bool oversold = rsi < rsi_oversold;

        // 3. FVG detection
        bool fvg = Indicators::detectFVG(data, current_index);

        // Debug output
        std::cerr << "[DEBUG] generateSignal: index=" << current_index
                  << " close=" << data[current_index].close
                  << " SMA=" << sma
                  << " RSI=" << rsi
                  << " FVG=" << fvg << std::endl;

        // 4. Entry logic: Only buy in uptrend, oversold, and FVG
        if (uptrend && oversold && fvg) {
            double entry = data[current_index].close;
            double stop = entry - (entry * 0.005 * risk_); // risk_ scales stop loss
            double target = entry + (entry - stop) * risk_reward;
            return {SignalType::BUY, current_index, stop, target, "Uptrend, RSI<30, FVG"};
        }
        return {SignalType::NONE, current_index, 0.0, 0.0, "No setup"};
    }
};

// Factory function for main.cpp
Strategy* createGoldenFoundationStrategy() {
    return new GoldenFoundationStrategy();
}