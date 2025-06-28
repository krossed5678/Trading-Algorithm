#include "../include/DataLoader.hpp"
#include "../include/Backtester.hpp"
#include "../include/Strategy.hpp"
#include <iostream>
#include <filesystem>
#include <memory>

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
    std::unique_ptr<Strategy> strategy(createGoldenFoundationStrategy(1.0));
    std::cout << "Creating backtester...\n";
    Backtester backtester(data, strategy.get(), 1000.0);
    std::cout << "Running backtest...\n";
    backtester.run();
    std::cout << "Backtest complete.\n";
    backtester.printYearlyPnL();
    backtester.printTotalGain();

    return 0;
}