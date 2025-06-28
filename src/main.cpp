#include "../include/DataLoader.hpp"
#include "../include/Backtester.hpp"
#include <iostream>
#include <filesystem>

// Factory function declaration
Strategy* createGoldenFoundationStrategy(double risk = 1.0);

int main() {
    std::string data_path = "data/SPY_1m.csv";
    if (!std::filesystem::exists(data_path)) {
        std::cerr << "ERROR: File not found: " << data_path << std::endl;
        return 1;
    }
    auto data = DataLoader::loadCSV(data_path);
    std::cout << "Loaded " << data.size() << " bars from SPY_1m.csv" << std::endl;
    if (data.empty()) {
        std::cerr << "ERROR: No data loaded from " << data_path << std::endl;
        return 1;
    }
    std::cout << "First bar: " << data.front().timestamp << ", Last bar: " << data.back().timestamp << std::endl;

    std::cout << "Creating strategy...\n";
    Strategy* strategy = createGoldenFoundationStrategy(1.0);
    std::cout << "Creating backtester...\n";
    Backtester backtester(data, strategy, 1000.0);
    std::cout << "Running backtest...\n";
    backtester.run();
    std::cout << "Backtest complete.\n";
    backtester.printYearlyPnL();
    backtester.printTotalGain();

    delete strategy;
    return 0;
}