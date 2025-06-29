#include "../include/GeneticStrategy.hpp"
#include "../include/DataLoader.hpp"
#include <iostream>
#include <fstream>
#include <chrono>
#include <cstdio>

int main() {
    std::cout << "=== Trading Strategy Genetic Evolution ===" << std::endl;
    
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
            break;
        }
    }
    
    if (data.empty()) {
        std::cerr << "[ERROR] Could not load data from any path!" << std::endl;
        return 1;
    }
    
    std::cout << "[SUCCESS] Loaded " << data.size() << " bars from " << data_path << std::endl;
    
    if (!data.empty()) {
        auto start_date = data.front().timestamp;
        auto end_date = data.back().timestamp;
        std::cout << "[INFO] Data range: " << start_date << " to " << end_date << std::endl;
    }
    
    int population_size = 200;
    int generations = 200;
    double mutation_rate = 0.1;
    double crossover_rate = 0.8;
    
    std::cout << "\n[INFO] Genetic Algorithm Parameters:" << std::endl;
    std::cout << "  Population Size: " << population_size << std::endl;
    std::cout << "  Generations: " << generations << std::endl;
    std::cout << "  Mutation Rate: " << mutation_rate << std::endl;
    std::cout << "  Crossover Rate: " << crossover_rate << std::endl;
    std::cout << "  Estimated Time: ~" << (generations * population_size / 100) << " minutes" << std::endl;
    std::cout << "  Mode: Overnight Training" << std::endl;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    GeneticAlgorithm ga(data, population_size, generations, mutation_rate, crossover_rate);
    std::vector<StrategyGene> final_population = ga.evolve();
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time);
    
    std::cout << "\n[INFO] Evolution completed in " << duration.count() << " seconds" << std::endl;
    
    StrategyGene best_strategy = ga.getBestStrategy();
    FitnessResult best_fitness = ga.evaluateFitness(best_strategy);
    
    std::cout << "\n=== BEST STRATEGY FOUND ===" << std::endl;
    std::cout << "Strategy: " << best_strategy.toString() << std::endl;
    std::cout << "Fitness: " << best_fitness.toString() << std::endl;
    
    std::string pine_script = ga.exportBestToPineScript();
    
    // Before writing Pine Script, always delete any existing file
    std::remove("best_strategy.pine");
    std::ofstream pine_file("best_strategy.pine");
    if (pine_file.is_open()) {
        pine_file << pine_script;
        pine_file.close();
        std::cout << "\n[SUCCESS] Best strategy exported to 'best_strategy.pine'" << std::endl;
    } else {
        std::cerr << "\n[ERROR] Could not write Pine Script file" << std::endl;
    }
    
    std::ofstream results_file("evolution_results.csv");
    if (results_file.is_open()) {
        results_file << "Generation,BestFitness,AvgFitness,BestReturn,BestSharpe,BestMaxDD,BestWinRate,BestTrades\n";
        
        results_file << "Final," << best_strategy.fitness << "," 
                    << best_fitness.total_return << "," 
                    << best_fitness.sharpe_ratio << "," 
                    << best_fitness.max_drawdown << "," 
                    << best_fitness.win_rate << "," 
                    << best_fitness.total_trades << "\n";
        
        results_file.close();
        std::cout << "[SUCCESS] Detailed results saved to 'evolution_results.csv'" << std::endl;
    }
    
    std::cout << "\n=== TOP 5 STRATEGIES ===" << std::endl;
    std::sort(final_population.begin(), final_population.end(), 
              [](const StrategyGene& a, const StrategyGene& b) { return a.fitness > b.fitness; });
    
    for (int i = 0; i < std::min(5, static_cast<int>(final_population.size())); ++i) {
        FitnessResult fitness = ga.evaluateFitness(final_population[i]);
        std::cout << (i + 1) << ". Fitness: " << final_population[i].fitness 
                  << " | " << fitness.toString() << std::endl;
    }
    
    std::cout << "\n=== EVOLUTION COMPLETE ===" << std::endl;
    std::cout << "Check 'best_strategy.pine' for the Pine Script code to use in TradingView!" << std::endl;
    
    return 0;
} 