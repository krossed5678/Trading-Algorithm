#include "../include/Backtester.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <immintrin.h> // For SIMD optimizations
#include <algorithm>

// Logging macros for extensive tracing
#define LOG(msg) std::cout << "[LOG] " << msg << std::endl;
#define DEBUG(msg) std::cout << "[DEBUG] " << msg << std::endl;
#define ERROR(msg) std::cerr << "[ERROR] " << msg << std::endl;

double equity_ = 100000.0;
double risk_per_trade = 0.05; //piece of a whole 0.01 = 1%
bool in_position = false;
double entry_price = 0.0;
double stop_loss = 0.0;
double take_profit = 0.0;
double position_size = 0.0;

Backtester::Backtester(const std::vector<OHLCV>& data, Strategy* strategy, double initial_equity)
    : data_(data), strategy_(strategy), initial_equity_(initial_equity), equity_(initial_equity) {}

void Backtester::run() {
    DEBUG("Backtester::run() started");
    auto start_time = std::chrono::high_resolution_clock::now();

    bool in_position = false;
    double entry_price = 0.0;
    double stop_loss = 0.0;
    double take_profit = 0.0;
    double position_size = 0.0;
    std::string entry_date;

    equity_curve_.reserve(data_.size());
    equity_curve_.push_back(initial_equity_);

    std::vector<double> close_prices(data_.size());
    std::vector<double> high_prices(data_.size());
    std::vector<double> low_prices(data_.size());
    for (size_t i = 0; i < data_.size(); ++i) {
        close_prices[i] = data_[i].close;
        high_prices[i] = data_[i].high;
        low_prices[i] = data_[i].low;
    }

    LOG("Starting main backtest loop over " << data_.size() << " bars");

    for (size_t i = 1; i < data_.size(); ++i) {
        if (!in_position) {
            TradeSignal signal = strategy_->generateSignal(data_, i);
            DEBUG("Bar " << i << ": Signal type = " << (int)signal.type);

            if (signal.type == SignalType::BUY) {
                entry_price = close_prices[i];
                stop_loss   = signal.stop_loss;
                take_profit = signal.take_profit;
                entry_date  = data_[i].timestamp;

                // --- position sizing ---
                double risk_amount   = equity_ * risk_per_trade; // risk fraction of equity
                double risk_per_unit = entry_price - stop_loss;  // $ risk per share
                if (risk_per_unit > 0) {
                    position_size = risk_amount / risk_per_unit;
                } else {
                    position_size = 0; // fail-safe
                }

                LOG("Trade opened at bar " << i << ", price: " << entry_price 
                    << ", SL: " << stop_loss << ", TP: " << take_profit
                    << ", Position size: " << position_size);

                in_position = true;
            } else {
                DEBUG("No trade opened at bar " << i);
            }
        } else {
            double current_low = low_prices[i];
            double current_high = high_prices[i];

            if (current_low <= stop_loss) {
                double pnl = (stop_loss - entry_price) * position_size;
                equity_ += pnl;
                addToYearlyPnL(entry_date, pnl);
                LOG("Stop loss hit at bar " << i << ", price: " << stop_loss
                    << ", PnL: " << pnl << ", New Equity: " << equity_);
                in_position = false;
            } else if (current_high >= take_profit) {
                double pnl = (take_profit - entry_price) * position_size;
                equity_ += pnl;
                addToYearlyPnL(entry_date, pnl);
                LOG("Take profit hit at bar " << i << ", price: " << take_profit
                    << ", PnL: " << pnl << ", New Equity: " << equity_);
                in_position = false;
            } else {
                DEBUG("Position held at bar " << i << ", price: " << close_prices[i]);
            }
        }

        equity_curve_.push_back(equity_);
    }

    if (in_position) {
        double pnl = (close_prices.back() - entry_price) * position_size;
        equity_ += pnl;
        addToYearlyPnL(entry_date, pnl);
        LOG("Closing remaining position at final bar, price: " 
            << close_prices.back() << ", PnL: " << pnl 
            << ", Final Equity: " << equity_);
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    LOG("Backtest completed in " << duration.count() << "ms");
}