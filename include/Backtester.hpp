#pragma once
#include <vector>
#include <string>
#include <map>
#include "DataLoader.hpp"
#include "Strategy.hpp"

class Backtester {
public:
    Backtester(const std::vector<OHLCV>& data, Strategy* strategy, double initial_equity = 1000.0);
    void run();
    void printYearlyPnL() const;
    void printTotalGain() const;
    const std::map<int, double>& getYearlyPnL() const { return yearly_pnl_; }
    double getFinalEquity() const { return equity_; }
private:
    int calculateDaysInDataset() const;
    void calculateAdditionalMetrics() const;
    const std::vector<OHLCV>& data_;
    Strategy* strategy_;
    double initial_equity_;
    double equity_;
    std::map<int, double> yearly_pnl_; // year -> P&L
    std::vector<double> equity_curve_;
};
