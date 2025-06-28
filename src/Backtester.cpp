#include "../include/Backtester.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <immintrin.h> // For SIMD optimizations
#include <algorithm>

Backtester::Backtester(const std::vector<OHLCV>& data, Strategy* strategy, double initial_equity)
    : data_(data), strategy_(strategy), initial_equity_(initial_equity), equity_(initial_equity) {}

void Backtester::run() {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    bool in_position = false;
    double entry_price = 0.0;
    double stop_loss = 0.0;
    double take_profit = 0.0;
    std::string entry_date;
    
    // Pre-allocate equity curve for better memory locality
    equity_curve_.reserve(data_.size());
    equity_curve_.push_back(initial_equity_);
    
    // Extract close, high, and low prices for better cache performance
    std::vector<double> close_prices(data_.size());
    std::vector<double> high_prices(data_.size());
    std::vector<double> low_prices(data_.size());
    
    for (size_t i = 0; i < data_.size(); ++i) {
        close_prices[i] = data_[i].close;
        high_prices[i] = data_[i].high;
        low_prices[i] = data_[i].low;
    }
    
    // Optimized main loop with better memory access patterns
    for (size_t i = 1; i < data_.size(); ++i) {
        if (!in_position) {
            TradeSignal signal = strategy_->generateSignal(data_, i);
            if (signal.type == SignalType::BUY) {
                in_position = true;
                entry_price = close_prices[i];
                stop_loss = signal.stop_loss;
                take_profit = signal.take_profit;
                entry_date = data_[i].timestamp;
            }
        } else {
            // Optimized position checking with early exit
            double current_low = low_prices[i];
            double current_high = high_prices[i];
            
            // Check stop loss first (usually more common)
            if (current_low <= stop_loss) {
                double pnl = (stop_loss - entry_price);
                equity_ += pnl;
                addToYearlyPnL(entry_date, pnl);
                in_position = false;
            } else if (current_high >= take_profit) {
                double pnl = (take_profit - entry_price);
                equity_ += pnl;
                addToYearlyPnL(entry_date, pnl);
                in_position = false;
            }
        }
        equity_curve_.push_back(equity_);
    }
    
    // Close any remaining position
    if (in_position) {
        double pnl = (close_prices.back() - entry_price);
        equity_ += pnl;
        addToYearlyPnL(entry_date, pnl);
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    std::cout << "Backtest completed in " << duration.count() << "ms" << std::endl;
}

void Backtester::addToYearlyPnL(const std::string& entry_date, double pnl) {
    // Optimized year parsing
    if (entry_date.length() >= 4) {
        int year = 0;
        // Use SIMD for faster string parsing if available
        #ifdef __AVX2__
        if (entry_date.length() >= 4) {
            // Fast year extraction using SIMD
            __m128i chars = _mm_loadu_si128((__m128i*)entry_date.data());
            __m128i zeros = _mm_set1_epi8('0');
            __m128i digits = _mm_sub_epi8(chars, zeros);
            
            // Extract first 4 digits
            int year_digits[4];
            _mm_storeu_si128((__m128i*)year_digits, digits);
            
            year = year_digits[0] * 1000 + year_digits[1] * 100 + year_digits[2] * 10 + year_digits[3];
        }
        #else
        // Fallback to standard parsing
        std::istringstream iss(entry_date.substr(0, 4));
        iss >> year;
        #endif
        
        if (year > 0) {
            yearly_pnl_[year] += pnl;
        }
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
    
    // Show actual yearly P&L with optimized formatting
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
    
    // Calculate additional metrics
    calculateAdditionalMetrics();
}

int Backtester::calculateDaysInDataset() const {
    if (data_.size() < 2) return 0;
    
    // Parse start and end dates
    std::string start_date = data_.front().timestamp;
    std::string end_date = data_.back().timestamp;
    
    // Extract date part (YYYY-MM-DD)
    std::string start_date_only = start_date.substr(0, 10);
    std::string end_date_only = end_date.substr(0, 10);
    
    // Optimized date parsing
    int start_year, start_month, start_day;
    int end_year, end_month, end_day;
    
    std::istringstream start_ss(start_date_only);
    std::istringstream end_ss(end_date_only);
    char dash1, dash2, dash3, dash4;
    
    start_ss >> start_year >> dash1 >> start_month >> dash2 >> start_day;
    end_ss >> end_year >> dash3 >> end_month >> dash4 >> end_day;
    
    // Calculate days between dates (optimized)
    int days = 0;
    if (start_year == end_year) {
        // Same year - optimized calculation
        days = (end_month - start_month) * 30 + (end_day - start_day);
    } else {
        // Different years - optimized calculation
        days = (end_year - start_year) * 365 + (end_month - start_month) * 30 + (end_day - start_day);
    }
    
    return std::max(1, days); // At least 1 day
}

void Backtester::calculateAdditionalMetrics() const {
    if (equity_curve_.size() < 2) return;
    
    // Calculate drawdown
    double peak = initial_equity_;
    double max_drawdown = 0.0;
    
    for (double equity : equity_curve_) {
        if (equity > peak) {
            peak = equity;
        }
        double drawdown = (peak - equity) / peak;
        if (drawdown > max_drawdown) {
            max_drawdown = drawdown;
        }
    }
    
    // Calculate Sharpe ratio (simplified)
    double total_return = (equity_ - initial_equity_) / initial_equity_;
    double volatility = 0.0;
    
    if (equity_curve_.size() > 1) {
        // Calculate daily returns
        std::vector<double> daily_returns;
        daily_returns.reserve(equity_curve_.size() - 1);
        
        for (size_t i = 1; i < equity_curve_.size(); ++i) {
            double daily_return = (equity_curve_[i] - equity_curve_[i-1]) / equity_curve_[i-1];
            daily_returns.push_back(daily_return);
        }
        
        // Calculate mean return
        double mean_return = 0.0;
        for (double ret : daily_returns) {
            mean_return += ret;
        }
        mean_return /= daily_returns.size();
        
        // Calculate volatility
        double variance = 0.0;
        for (double ret : daily_returns) {
            double diff = ret - mean_return;
            variance += diff * diff;
        }
        volatility = std::sqrt(variance / daily_returns.size());
    }
    
    double sharpe_ratio = (volatility > 0) ? (total_return / volatility) : 0.0;
    
    std::cout << "Maximum Drawdown: " << std::fixed << std::setprecision(2) << (max_drawdown * 100.0) << "%\n";
    std::cout << "Sharpe Ratio: " << std::fixed << std::setprecision(2) << sharpe_ratio << "\n";
}

const std::map<int, double>& Backtester::getYearlyPnL() const { return yearly_pnl_; }
double Backtester::getFinalEquity() const { return equity_; }