#pragma once
#include <vector>
#include <random>
#include <memory>
#include <string>
#include "Strategy.hpp"
#include "DataLoader.hpp"

// Represents a single trading strategy's parameters
struct StrategyGene {
    // Indicator types
    enum class IndicatorType {
        SMA, EMA, RSI, MACD, BB, ATR, STOCH, ADX
    };
    
    // Entry conditions
    enum class EntryCondition {
        CROSS_ABOVE, CROSS_BELOW, ABOVE, BELOW, INSIDE_BB, OUTSIDE_BB
    };
    
    // Exit conditions
    enum class ExitCondition {
        FIXED_RR, TRAILING_STOP, TIME_BASED, INDICATOR_SIGNAL
    };
    
    // Strategy parameters
    IndicatorType primary_indicator = IndicatorType::SMA;
    IndicatorType secondary_indicator = IndicatorType::RSI;
    int primary_period = 20;
    int secondary_period = 14;
    double primary_threshold = 0.0;
    double secondary_threshold = 30.0;
    EntryCondition entry_condition = EntryCondition::CROSS_ABOVE;
    ExitCondition exit_condition = ExitCondition::FIXED_RR;
    double risk_reward_ratio = 2.0;
    double stop_loss_pct = 0.02;
    double take_profit_pct = 0.04;
    int max_hold_time = 48; // hours
    double position_size_pct = 0.1;
    
    // Fitness score
    double fitness = 0.0;
    
    // Methods
    StrategyGene() = default;
    StrategyGene(const StrategyGene& other) = default;
    StrategyGene& operator=(const StrategyGene& other) = default;
    
    // Generate random strategy
    static StrategyGene random(std::mt19937& rng);
    
    // Mutate this strategy
    void mutate(std::mt19937& rng, double mutation_rate = 0.1);
    
    // Crossover with another strategy
    StrategyGene crossover(const StrategyGene& other, std::mt19937& rng) const;
    
    // Convert to string for debugging
    std::string toString() const;
    
    // Convert to Pine Script
    std::string toPineScript() const;
};

// Fitness evaluation results
struct FitnessResult {
    double total_return = 0.0;
    double sharpe_ratio = 0.0;
    double max_drawdown = 0.0;
    double win_rate = 0.0;
    int total_trades = 0;
    double profit_factor = 0.0;
    double calmar_ratio = 0.0;
    
    // Combined fitness score
    double fitness_score = 0.0;
    
    std::string toString() const;
};

// Genetic Algorithm for strategy evolution
class GeneticAlgorithm {
public:
    GeneticAlgorithm(const std::vector<OHLCV>& data, 
                     int population_size = 50,
                     int generations = 100,
                     double mutation_rate = 0.1,
                     double crossover_rate = 0.8);
    
    // Run the genetic algorithm
    std::vector<StrategyGene> evolve();
    
    // Evaluate fitness of a single strategy
    FitnessResult evaluateFitness(const StrategyGene& gene);
    
    // Get the best strategy found
    StrategyGene getBestStrategy() const;
    
    // Export best strategy to Pine Script
    std::string exportBestToPineScript() const;
    
private:
    std::vector<OHLCV> data_;
    std::vector<StrategyGene> population_;
    StrategyGene best_strategy_;
    FitnessResult best_fitness_;
    
    int population_size_;
    int generations_;
    double mutation_rate_;
    double crossover_rate_;
    
    std::mt19937 rng_;
    
    // Genetic algorithm steps
    void initializePopulation();
    void evaluatePopulation();
    void selectParents();
    void crossover();
    void mutate();
    void elitism();
    
    // Helper functions
    double calculateSharpeRatio(const std::vector<double>& returns);
    double calculateMaxDrawdown(const std::vector<double>& equity_curve);
    double calculateProfitFactor(const std::vector<double>& profits, const std::vector<double>& losses);
};

// Strategy implementation that uses StrategyGene
class EvolvedStrategy : public Strategy {
public:
    EvolvedStrategy(const StrategyGene& gene);
    TradeSignal generateSignal(const std::vector<OHLCV>& data, size_t current_index) override;
    
private:
    StrategyGene gene_;
    std::vector<double> primary_values_;
    std::vector<double> secondary_values_;
    bool precomputed_ = false;
    
    void precomputeIndicators(const std::vector<OHLCV>& data);
    double calculateIndicator(const std::vector<OHLCV>& data, size_t index, StrategyGene::IndicatorType type, int period);
    bool checkEntryCondition(const std::vector<OHLCV>& data, size_t index);
    double calculateStopLoss(const std::vector<OHLCV>& data, size_t index);
    double calculateTakeProfit(const std::vector<OHLCV>& data, size_t index);
}; 