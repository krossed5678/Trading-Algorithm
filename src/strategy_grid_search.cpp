#include "../include/DataLoader.hpp"
#include "../include/Backtester.hpp"
#include "../include/Strategy.hpp"
#include "../include/Strategy.hpp" // For GoldenFoundationStrategy
#include <fstream>
#include <iostream>
#include <vector>
#include <iomanip>
#include <memory>

// Helper: Run a backtest and return results
struct GridResult {
    int sma_period;
    int rsi_period;
    double rsi_threshold;
    double risk_reward;
    double final_equity;
    int total_trades;
    double win_rate;
};

int main() {
    // Try multiple possible data file paths
    std::vector<std::string> possible_paths = {
        "data/SPY_1m.csv",
        "../data/SPY_1m.csv", 
        "../../data/SPY_1m.csv",
        "../../../data/SPY_1m.csv"
    };
    
    std::string data_path;
    auto data = std::vector<OHLCV>();
    
    for (const auto& path : possible_paths) {
        std::cout << "[INFO] Trying data path: " << path << std::endl;
        data = DataLoader::loadCSV(path);
        if (!data.empty()) {
            data_path = path;
            std::cout << "[INFO] Successfully loaded data from: " << path << std::endl;
            break;
        }
    }
    
    if (data.empty()) {
        std::cerr << "[ERROR] Could not find SPY_1m.csv in any of the expected locations:" << std::endl;
        for (const auto& path : possible_paths) {
            std::cerr << "  - " << path << std::endl;
        }
        std::cerr << "[ERROR] Please ensure SPY_1m.csv exists in the data directory." << std::endl;
        return 1;
    }
    
    std::cout << "[INFO] Loaded " << data.size() << " bars from " << data_path << std::endl;

    // Parameter ranges
    std::vector<int> sma_periods = {5, 10, 20, 50, 100};
    std::vector<int> rsi_periods = {7, 14, 21};
    std::vector<double> rsi_thresholds = {20.0, 30.0, 40.0};
    std::vector<double> risk_rewards = {1.5, 2.0, 3.0, 5.0};

    std::ofstream csv("grid_search_results.csv");
    csv << "SMA,RSI,RSI_Threshold,RR,FinalEquity,TotalTrades,WinRate\n";

    int test_count = 0;
    for (int sma : sma_periods) {
        for (int rsi : rsi_periods) {
            for (double rsi_th : rsi_thresholds) {
                for (double rr : risk_rewards) {
                    std::unique_ptr<Strategy> strategy(createGoldenFoundationStrategy(rr));
                    auto* strat = dynamic_cast<GoldenFoundationStrategy*>(strategy.get());
                    if (strat) {
                        strat->setSMA(sma);
                        strat->setRSI(rsi, rsi_th);
                    }
                    Backtester backtester(data, strategy.get(), 10000.0);
                    backtester.run();
                    double final_equity = backtester.getFinalEquity();
                    int total_trades = backtester.getTotalTrades();
                    double win_rate = backtester.getWinRate();
                    csv << sma << "," << rsi << "," << rsi_th << "," << rr << ","
                        << std::fixed << std::setprecision(2) << final_equity << ","
                        << total_trades << ","
                        << std::setprecision(4) << win_rate << "\n";
                    test_count++;
                    std::cout << "Test " << test_count << ": SMA=" << sma << ", RSI=" << rsi << ", RSI_Th=" << rsi_th << ", RR=" << rr << " => Equity=" << final_equity << ", Trades=" << total_trades << ", WinRate=" << win_rate << std::endl;
                }
            }
        }
    }
    csv.close();
    std::cout << "Grid search complete. Results written to grid_search_results.csv" << std::endl;
    return 0;
}
