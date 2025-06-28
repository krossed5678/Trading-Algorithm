#include "../include/Backtester.hpp"
#include <iostream>
#include <sstream>

Backtester::Backtester(const std::vector<OHLCV>& data, Strategy* strategy, double initial_equity)
    : data_(data), strategy_(strategy), initial_equity_(initial_equity), equity_(initial_equity) {}

void Backtester::run() {
    bool in_position = false;
    double entry_price = 0.0;
    double stop_loss = 0.0;
    double take_profit = 0.0;
    int entry_year = 0;
    for (size_t i = 1; i < data_.size(); ++i) {
        // Parse year from timestamp (assumes format YYYY-MM-DD ...)
        int year = 0;
        std::istringstream iss(data_[i].timestamp.substr(0, 4));
        iss >> year;
        if (!in_position) {
            TradeSignal signal = strategy_->generateSignal(data_, i);
            if (signal.type == SignalType::BUY) {
                in_position = true;
                entry_price = data_[i].close;
                stop_loss = signal.stop_loss;
                take_profit = signal.take_profit;
                entry_year = year;
            }
        } else {
            // Check stop loss or take profit
            if (data_[i].low <= stop_loss) {
                double pnl = (stop_loss - entry_price);
                equity_ += pnl;
                yearly_pnl_[entry_year] += pnl;
                in_position = false;
            } else if (data_[i].high >= take_profit) {
                double pnl = (take_profit - entry_price);
                equity_ += pnl;
                yearly_pnl_[entry_year] += pnl;
                in_position = false;
            }
        }
        equity_curve_.push_back(equity_);
    }
    // If still in position at end, close at last price
    if (in_position) {
        double pnl = (data_.back().close - entry_price);
        equity_ += pnl;
        int year = 0;
        std::istringstream iss(data_.back().timestamp.substr(0, 4));
        iss >> year;
        yearly_pnl_[year] += pnl;
    }
}

void Backtester::printYearlyPnL() const {
    std::cout << "Yearly P&L:\n";
    for (const auto& kv : yearly_pnl_) {
        std::cout << kv.first << ": " << kv.second << std::endl;
    }
}

void Backtester::printTotalGain() const {
    std::cout << "Total gain: " << (equity_ - initial_equity_) << std::endl;
    std::cout << "Final equity: " << equity_ << std::endl;
}