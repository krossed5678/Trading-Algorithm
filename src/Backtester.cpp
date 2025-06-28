#include "../include/Backtester.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <chrono>

Backtester::Backtester(const std::vector<OHLCV>& data, Strategy* strategy, double initial_equity)
    : data_(data), strategy_(strategy), initial_equity_(initial_equity), equity_(initial_equity) {}

void Backtester::run() {
    bool in_position = false;
    double entry_price = 0.0;
    double stop_loss = 0.0;
    double take_profit = 0.0;
    std::string entry_date;
    
    for (size_t i = 1; i < data_.size(); ++i) {
        if (!in_position) {
            TradeSignal signal = strategy_->generateSignal(data_, i);
            if (signal.type == SignalType::BUY) {
                in_position = true;
                entry_price = data_[i].close;
                stop_loss = signal.stop_loss;
                take_profit = signal.take_profit;
                entry_date = data_[i].timestamp;
            }
        } else {
            // Check stop loss or take profit
            if (data_[i].low <= stop_loss) {
                double pnl = (stop_loss - entry_price);
                equity_ += pnl;
                addToYearlyPnL(entry_date, pnl);
                in_position = false;
            } else if (data_[i].high >= take_profit) {
                double pnl = (take_profit - entry_price);
                equity_ += pnl;
                addToYearlyPnL(entry_date, pnl);
                in_position = false;
            }
        }
        equity_curve_.push_back(equity_);
    }
    // If still in position at end, close at last price
    if (in_position) {
        double pnl = (data_.back().close - entry_price);
        equity_ += pnl;
        addToYearlyPnL(entry_date, pnl);
    }
}

void Backtester::addToYearlyPnL(const std::string& entry_date, double pnl) {
    // Parse year from entry date (assumes format YYYY-MM-DD or YYYY-MM-DD HH:MM:SS)
    int year = 0;
    std::istringstream iss(entry_date.substr(0, 4));
    iss >> year;
    if (year > 0) {
        yearly_pnl_[year] += pnl;
    }
}

void Backtester::printYearlyPnL() const {
    std::cout << "Yearly P&L:\n";
    
    // Calculate date range from CSV
    if (data_.empty()) {
        std::cout << "No data available\n";
        return;
    }
    
    // Parse start and end dates
    std::string start_date = data_.front().timestamp;
    std::string end_date = data_.back().timestamp;
    
    // Extract year from start date
    int start_year = 0;
    std::istringstream start_iss(start_date.substr(0, 4));
    start_iss >> start_year;
    
    // Extract year from end date
    int end_year = 0;
    std::istringstream end_iss(end_date.substr(0, 4));
    end_iss >> end_year;
    
    std::cout << "Data range: " << start_date << " to " << end_date << "\n";
    std::cout << "Years covered: " << start_year << " to " << end_year << "\n\n";
    
    // Calculate days in the dataset
    int total_days = calculateDaysInDataset();
    std::cout << "Total days in dataset: " << total_days << "\n";
    
    // Show actual yearly P&L
    for (const auto& kv : yearly_pnl_) {
        std::cout << kv.first << ": $" << std::fixed << std::setprecision(2) << kv.second;
        
        // Extrapolate to full year
        if (total_days > 0) {
            double extrapolated = kv.second * (365.0 / total_days);
            std::cout << " (extrapolated to full year: $" << std::fixed << std::setprecision(2) << extrapolated << ")";
        }
        std::cout << "\n";
    }
}

void Backtester::printTotalGain() const {
    double total_gain = equity_ - initial_equity_;
    double percentage_gain = (total_gain / initial_equity_) * 100.0;
    
    std::cout << "=== FINAL RESULTS ===\n";
    std::cout << "Initial Capital: $" << std::fixed << std::setprecision(2) << initial_equity_ << "\n";
    std::cout << "Final Equity: $" << std::fixed << std::setprecision(2) << equity_ << "\n";
    std::cout << "Total Gain: $" << std::fixed << std::setprecision(2) << total_gain << " (" << std::fixed << std::setprecision(2) << percentage_gain << "%)\n";
    
    // Calculate annualized return if we have date information
    if (!data_.empty()) {
        int total_days = calculateDaysInDataset();
        if (total_days > 0) {
            double annualized_return = (percentage_gain / total_days) * 365.0;
            std::cout << "Annualized Return: " << std::fixed << std::setprecision(2) << annualized_return << "%\n";
        }
    }
}

int Backtester::calculateDaysInDataset() const {
    if (data_.size() < 2) return 0;
    
    // Parse start and end dates
    std::string start_date = data_.front().timestamp;
    std::string end_date = data_.back().timestamp;
    
    // Extract date part (YYYY-MM-DD)
    std::string start_date_only = start_date.substr(0, 10);
    std::string end_date_only = end_date.substr(0, 10);
    
    // Simple date parsing (assumes YYYY-MM-DD format)
    int start_year, start_month, start_day;
    int end_year, end_month, end_day;
    
    std::istringstream start_ss(start_date_only);
    std::istringstream end_ss(end_date_only);
    char dash1, dash2, dash3, dash4;
    
    start_ss >> start_year >> dash1 >> start_month >> dash2 >> start_day;
    end_ss >> end_year >> dash3 >> end_month >> dash4 >> end_day;
    
    // Calculate days between dates (simplified)
    int days = 0;
    if (start_year == end_year) {
        // Same year - rough calculation
        days = (end_month - start_month) * 30 + (end_day - start_day);
    } else {
        // Different years - rough calculation
        days = (end_year - start_year) * 365 + (end_month - start_month) * 30 + (end_day - start_day);
    }
    
    return std::max(1, days); // At least 1 day
}

const std::map<int, double>& Backtester::getYearlyPnL() const { return yearly_pnl_; }
double Backtester::getFinalEquity() const { return equity_; }