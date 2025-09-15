
#include "../include/GeneticStrategy.hpp"
#include "../include/DataLoader.hpp"
#include <fstream>
#include <chrono>
#include <iostream>
#include <filesystem>
#include <limits>
#include <numeric>


#define LOG(msg)     std::cout << "[LOG] " << msg << std::endl;
#define INFO(msg)    std::cout << "[INFO] " << msg << std::endl;
#define SUCCESS(msg) std::cout << "[SUCCESS] " << msg << std::endl;
#define ERROR(msg)   std::cerr << "[ERROR] " << msg << std::endl;
#define DEBUG(msg)   std::cout << "[DEBUG] " << msg << std::endl;

namespace {

    // Scoped timer for runtime measurement
    struct ScopedTimer {
        std::string label;
        std::chrono::high_resolution_clock::time_point start;
        explicit ScopedTimer(std::string name)
            : label(std::move(name)), start(std::chrono::high_resolution_clock::now()) {}
        ~ScopedTimer() {
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - start).count();
            INFO(label << " completed in " << duration << " seconds");
        }
    };

    std::vector<OHLCV> load_data(const std::vector<std::string>& paths, std::string& out_path) {
        for (const auto& path : paths) {
            LOG("Trying data path: " << path);
            auto data = DataLoader::loadCSV(path);
            if (!data.empty()) {
                SUCCESS("Data loaded from: " << path);
                out_path = path;
                return data;
            }
        }
        ERROR("Could not load data from any path!");
        return {};
    }

    void print_params(int pop, int gen, double mut, double cross) {
        INFO("Genetic Algorithm Parameters:");
        std::cout << "  Population Size : " << pop   << "\n"
                  << "  Generations     : " << gen   << "\n"
                  << "  Mutation Rate   : " << mut   << "\n"
                  << "  Crossover Rate  : " << cross << "\n"
                  << "  Mode            : Overnight Training\n";
    }

    void export_pine(const StrategyGene& best_strategy) {
        auto now = std::chrono::system_clock::now();
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);

        // Timestamp for filename
        char time_buf[20];
        std::strftime(time_buf, sizeof(time_buf), "%Y%m%d_%H%M%S", std::localtime(&now_c));
        std::string filename = "best_strategy_" + std::string(time_buf) + ".pine";

        std::ofstream pine_file(filename);
        if (pine_file.is_open()) {
            pine_file << "// Exported on: "
                      << std::put_time(std::localtime(&now_c), "%Y-%m-%d %H:%M:%S") << "\n";
            pine_file << best_strategy.toPineScript();
            pine_file.close();
            SUCCESS("Best strategy exported to '" << filename << "'");
        } else {
            ERROR("Could not write Pine Script file");
        }
    }

    void export_results(const std::vector<StrategyGene>& pop, const FitnessResult& best_fitness) {
    std::ofstream results_file("evolution_results.csv", std::ios::out | std::ios::trunc);
    if (!results_file) {
        ERROR("Could not open evolution_results.csv for writing");
        return;
    }

    // Write header
    results_file << "Generation,BestFitness,AvgFitness,BestReturn,BestSharpe,BestMaxDD,BestWinRate,BestTrades\n";

    if (!pop.empty()) {
        // Safely compute best and average fitness
        double maxFitness = std::numeric_limits<double>::lowest();
        double sumFitness = 0.0;

        for (const auto& g : pop) {
            // Defensive check: ensure fitness is finite
            if (std::isfinite(g.fitness)) {
                if (g.fitness > maxFitness) {
                    maxFitness = g.fitness;
                }
                sumFitness += g.fitness;
            }
        }

        double avgFitness = (pop.size() > 0) 
                            ? sumFitness / static_cast<double>(pop.size()) 
                            : 0.0;

        results_file << "Final,"
                     << maxFitness << ","
                     << avgFitness << ","
                     << best_fitness.total_return << ","
                     << best_fitness.sharpe_ratio << ","
                     << best_fitness.max_drawdown << ","
                     << best_fitness.win_rate << ","
                     << best_fitness.total_trades << "\n";
    } else {
        ERROR("Population is empty — no results written.");
    }

    if (!results_file.good()) {
        ERROR("Write to evolution_results.csv may have failed.");
    } else {
        SUCCESS("Detailed results saved to 'evolution_results.csv'");
    }
}

int main() {
    std::string data_path;
    const std::vector<std::string> search_paths = {
        "data/SPY_1m.csv",
        "../data/SPY_1m.csv",
        "../../data/SPY_1m.csv",
        "../../../data/SPY_1m.csv"
    };
    {
        std::cout << "[INFO] Running in overnight mode" << std::endl;
    }

    auto data = load_data(search_paths, data_path);
    if (data.empty()) return 1;

    INFO("Loaded " << data.size() << " bars from " << data_path);
    INFO("Data range: " << data.front().timestamp << " to " << data.back().timestamp);

    int population_size = 200, generations = 200;
    double mutation_rate = 0.1, crossover_rate = 0.8;
    print_params(population_size, generations, mutation_rate, crossover_rate);

    ScopedTimer timer("Evolution");

    GeneticAlgorithm ga(data, population_size, generations, mutation_rate, crossover_rate);
    auto final_population = ga.evolve();

    StrategyGene best_strategy = ga.getBestStrategy();
    FitnessResult best_fitness = ga.evaluateFitness(best_strategy);

    std::cout << "\n=== BEST STRATEGY FOUND ===\n"
              << "Strategy: " << best_strategy.toString() << "\n"
              << "Fitness : " << best_fitness.toString() << "\n";

    export_pine(best_strategy);
    export_results(final_population, best_fitness);

    std::cout << "\n=== TOP 5 STRATEGIES ===\n";
    std::sort(final_population.begin(), final_population.end(),
              [](const StrategyGene& a, const StrategyGene& b) { return a.fitness > b.fitness; });

    for (int i = 0; i < std::min(5, static_cast<int>(final_population.size())); ++i) {
        FitnessResult fitness = ga.evaluateFitness(final_population[i]);
        std::cout << (i + 1) << ". Fitness: " << final_population[i].fitness
                  << " | " << fitness.toString() << "\n";
    }

    std::cout << "\n=== EVOLUTION COMPLETE ===\n";
    std::cout << "Check output Pine Script file in the current directory for TradingView import.\n";
    return 0;
}