#include "include/DataLoader.hpp"
#include "include/Strategy.hpp"
#include "include/Backtester.hpp"
#include "include/MovingAverage.hpp"
#include "include/GPUStrategy.hpp"
#include "include/FileUtils.hpp"
#include <iostream>
#include <chrono>
#include <vector>
#include <iomanip>

// Declare the GPU function at global scope
extern "C" void gpu_calculate_all_indicators_and_signals(
    const double* prices, int n,
    double* sma, double* rsi, int* signals, double* stops, double* targets,
    int sma_period, int rsi_period, double rsi_oversold, double risk_reward
);

// Performance benchmarking class
class PerformanceBenchmark {
public:
    static void runCPUvsGPUComparison(const std::vector<OHLCV>& data) {
        std::cout << "=== PERFORMANCE BENCHMARK ===\n";
        std::cout << "Dataset size: " << data.size() << " bars\n\n";
        
        // Test CPU-only indicators
        testCPUIndicators(data);
        
        // Test GPU indicators (if available)
        #ifdef USE_CUDA
        testGPUIndicators(data);
        #else
        std::cout << "GPU testing skipped - CUDA not available\n";
        #endif
        
        // Test full backtest performance
        testBacktestPerformance(data);
    }
    
private:
    static void testCPUIndicators(const std::vector<OHLCV>& data) {
        std::cout << "--- CPU Indicators Test ---\n";
        
        const int sma_period = 50;
        const int rsi_period = 14;
        const int test_iterations = 10;
        
        // Test individual indicator calculations
        auto start = std::chrono::high_resolution_clock::now();
        for (int iter = 0; iter < test_iterations; ++iter) {
            for (size_t i = sma_period; i < data.size(); ++i) {
                double sma = Indicators::SMA(data, i, sma_period);
                double rsi = Indicators::RSI(data, i, rsi_period);
                (void)sma; (void)rsi; // Prevent unused variable warnings
            }
        }
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::cout << "Individual indicators: " << duration.count() << "ms for " 
                  << test_iterations << " iterations\n";
        
        // Test batch indicator calculations
        std::vector<double> sma_values, rsi_values;
        start = std::chrono::high_resolution_clock::now();
        for (int iter = 0; iter < test_iterations; ++iter) {
            Indicators::calculateBatchIndicators(data, sma_values, rsi_values, sma_period, rsi_period);
        }
        end = std::chrono::high_resolution_clock::now();
        duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::cout << "Batch indicators: " << duration.count() << "ms for " 
                  << test_iterations << " iterations\n";
        
        double avg_time_per_iter = static_cast<double>(duration.count()) / test_iterations;
        double bars_per_second = (data.size() * 1000.0) / avg_time_per_iter;
        std::cout << "Performance: " << std::fixed << std::setprecision(0) 
                  << bars_per_second << " bars/second\n\n";
    }
    
    static void testGPUIndicators(const std::vector<OHLCV>& data) {
        std::cout << "--- GPU Indicators Test ---\n";
        
        const int sma_period = 50;
        const int rsi_period = 14;
        const int test_iterations = 10;
        
        // Extract prices
        std::vector<double> prices(data.size());
        for (size_t i = 0; i < data.size(); ++i) {
            prices[i] = data[i].close;
        }
        
        // Allocate result arrays
        std::vector<double> sma_values(data.size());
        std::vector<double> rsi_values(data.size());
        std::vector<int> signals(data.size());
        std::vector<double> stops(data.size());
        std::vector<double> targets(data.size());
        
        // Test GPU calculations
        auto start = std::chrono::high_resolution_clock::now();
        for (int iter = 0; iter < test_iterations; ++iter) {
            gpu_calculate_all_indicators_and_signals(
                prices.data(), static_cast<int>(data.size()),
                sma_values.data(), rsi_values.data(),
                signals.data(), stops.data(), targets.data(),
                sma_period, rsi_period, 30.0, 2.0
            );
        }
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::cout << "GPU fused kernel: " << duration.count() << "ms for " 
                  << test_iterations << " iterations\n";
        
        double avg_time_per_iter = static_cast<double>(duration.count()) / test_iterations;
        double bars_per_second = (data.size() * 1000.0) / avg_time_per_iter;
        std::cout << "Performance: " << std::fixed << std::setprecision(0) 
                  << bars_per_second << " bars/second\n\n";
    }
    
    static void testBacktestPerformance(const std::vector<OHLCV>& data) {
        std::cout << "--- Backtest Performance Test ---\n";
        
        // Test CPU strategy
        Strategy* cpu_strategy = createGoldenFoundationStrategy(2.0);
        Backtester cpu_backtester(data, cpu_strategy, 10000.0);
        
        auto start = std::chrono::high_resolution_clock::now();
        cpu_backtester.run();
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::cout << "CPU backtest: " << duration.count() << "ms\n";
        std::cout << "Final equity: $" << std::fixed << std::setprecision(2) 
                  << cpu_backtester.getFinalEquity() << "\n";
        
        delete cpu_strategy;
        
        // Test GPU strategy (if available)
        #ifdef USE_CUDA
        Strategy* gpu_strategy = createGPUGoldenFoundationStrategy(2.0);
        Backtester gpu_backtester(data, gpu_strategy, 10000.0);
        
        start = std::chrono::high_resolution_clock::now();
        gpu_backtester.run();
        end = std::chrono::high_resolution_clock::now();
        duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::cout << "GPU backtest: " << duration.count() << "ms\n";
        std::cout << "Final equity: $" << std::fixed << std::setprecision(2) 
                  << gpu_backtester.getFinalEquity() << "\n";
        
        delete gpu_strategy;
        #endif
        
        std::cout << "\n";
    }
};

int main() {
    std::cout << "Loading data for performance benchmark...\n";
    
    // Load test data
    std::string data_path = FileUtils::findDataFile("SPY_1m.csv");
    std::vector<OHLCV> data = DataLoader::loadCSV(data_path);
    
    if (data.empty()) {
        std::cerr << "Failed to load data. Please ensure SPY_1m.csv exists.\n";
        std::cerr << "Run 'python fetch_spy_data.py' to download the data.\n";
        return 1;
    }
    
    // Run performance benchmarks
    PerformanceBenchmark::runCPUvsGPUComparison(data);
    
    std::cout << "Performance benchmark completed!\n";
    return 0;
} 