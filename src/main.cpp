#include "../include/DataLoader.hpp"
#include "../include/Backtester.hpp"
#include "../include/Strategy.hpp"
#include "../include/FileUtils.hpp"
#include <iostream>
#include <filesystem>
#include <memory>

int main() {
    std::string data_path = FileUtils::findDataFile("SPY_1m.csv");
    if (!std::filesystem::exists(data_path)) {
        std::cerr << "ERROR: SPY_1m.csv not found!" << std::endl;
        std::cerr << "Tried looking in:" << std::endl;
        std::cerr << "  - Current directory" << std::endl;
        std::cerr << "  - ../ (one level up)" << std::endl;
        std::cerr << "  - ../../ (two levels up)" << std::endl;
        std::cerr << "  - data/ subdirectory" << std::endl;
        std::cerr << "  - ../data/ subdirectory" << std::endl;
        std::cerr << "  - ../../data/ subdirectory" << std::endl;
        std::cerr << std::endl;
        std::cerr << "Please ensure SPY_1m.csv exists in one of these locations." << std::endl;
        std::cerr << "You can run 'python fetch_spy_data.py' to download the data." << std::endl;
        return 1;
    }
    
    auto data = DataLoader::loadCSV(data_path);
    std::cout << "Loaded " << data.size() << " bars from " << data_path << std::endl;
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