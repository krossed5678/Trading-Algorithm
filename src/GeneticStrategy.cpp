#include "../include/GeneticStrategy.hpp"
#include "../include/MovingAverage.hpp"
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cmath>
#include <numeric>

StrategyGene StrategyGene::random(std::mt19937& rng) {
    StrategyGene gene;
    
    std::uniform_int_distribution<int> indicator_dist(0, 7);
    std::uniform_int_distribution<int> entry_dist(0, 5);
    std::uniform_int_distribution<int> exit_dist(0, 3);
    std::uniform_int_distribution<int> period_dist(5, 200);
    std::uniform_real_distribution<double> threshold_dist(-50.0, 50.0);
    std::uniform_real_distribution<double> rr_dist(1.0, 10.0);
    std::uniform_real_distribution<double> pct_dist(0.005, 0.1);
    std::uniform_int_distribution<int> time_dist(1, 168);
    std::uniform_real_distribution<double> size_dist(0.01, 0.5);
    
    gene.primary_indicator = static_cast<IndicatorType>(indicator_dist(rng));
    gene.secondary_indicator = static_cast<IndicatorType>(indicator_dist(rng));
    gene.primary_period = period_dist(rng);
    gene.secondary_period = period_dist(rng);
    gene.primary_threshold = threshold_dist(rng);
    gene.secondary_threshold = threshold_dist(rng);
    gene.entry_condition = static_cast<EntryCondition>(entry_dist(rng));
    gene.exit_condition = static_cast<ExitCondition>(exit_dist(rng));
    gene.risk_reward_ratio = rr_dist(rng);
    gene.stop_loss_pct = pct_dist(rng);
    gene.take_profit_pct = pct_dist(rng);
    gene.max_hold_time = time_dist(rng);
    gene.position_size_pct = size_dist(rng);
    
    return gene;
}

void StrategyGene::mutate(std::mt19937& rng, double mutation_rate) {
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    std::uniform_int_distribution<int> indicator_dist(0, 7);
    std::uniform_int_distribution<int> entry_dist(0, 5);
    std::uniform_int_distribution<int> exit_dist(0, 3);
    std::uniform_int_distribution<int> period_dist(5, 200);
    std::uniform_real_distribution<double> threshold_dist(-50.0, 50.0);
    std::uniform_real_distribution<double> rr_dist(1.0, 10.0);
    std::uniform_real_distribution<double> pct_dist(0.005, 0.1);
    std::uniform_int_distribution<int> time_dist(1, 168);
    std::uniform_real_distribution<double> size_dist(0.01, 0.5);
    
    if (dist(rng) < mutation_rate) primary_indicator = static_cast<IndicatorType>(indicator_dist(rng));
    if (dist(rng) < mutation_rate) secondary_indicator = static_cast<IndicatorType>(indicator_dist(rng));
    if (dist(rng) < mutation_rate) primary_period = period_dist(rng);
    if (dist(rng) < mutation_rate) secondary_period = period_dist(rng);
    if (dist(rng) < mutation_rate) primary_threshold = threshold_dist(rng);
    if (dist(rng) < mutation_rate) secondary_threshold = threshold_dist(rng);
    if (dist(rng) < mutation_rate) entry_condition = static_cast<EntryCondition>(entry_dist(rng));
    if (dist(rng) < mutation_rate) exit_condition = static_cast<ExitCondition>(exit_dist(rng));
    if (dist(rng) < mutation_rate) risk_reward_ratio = rr_dist(rng);
    if (dist(rng) < mutation_rate) stop_loss_pct = pct_dist(rng);
    if (dist(rng) < mutation_rate) take_profit_pct = pct_dist(rng);
    if (dist(rng) < mutation_rate) max_hold_time = time_dist(rng);
    if (dist(rng) < mutation_rate) position_size_pct = size_dist(rng);
}

StrategyGene StrategyGene::crossover(const StrategyGene& other, std::mt19937& rng) const {
    StrategyGene child;
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    
    child.primary_indicator = (dist(rng) < 0.5) ? primary_indicator : other.primary_indicator;
    child.secondary_indicator = (dist(rng) < 0.5) ? secondary_indicator : other.secondary_indicator;
    child.primary_period = (dist(rng) < 0.5) ? primary_period : other.primary_period;
    child.secondary_period = (dist(rng) < 0.5) ? secondary_period : other.secondary_period;
    child.primary_threshold = (dist(rng) < 0.5) ? primary_threshold : other.primary_threshold;
    child.secondary_threshold = (dist(rng) < 0.5) ? secondary_threshold : other.secondary_threshold;
    child.entry_condition = (dist(rng) < 0.5) ? entry_condition : other.entry_condition;
    child.exit_condition = (dist(rng) < 0.5) ? exit_condition : other.exit_condition;
    child.risk_reward_ratio = (dist(rng) < 0.5) ? risk_reward_ratio : other.risk_reward_ratio;
    child.stop_loss_pct = (dist(rng) < 0.5) ? stop_loss_pct : other.stop_loss_pct;
    child.take_profit_pct = (dist(rng) < 0.5) ? take_profit_pct : other.take_profit_pct;
    child.max_hold_time = (dist(rng) < 0.5) ? max_hold_time : other.max_hold_time;
    child.position_size_pct = (dist(rng) < 0.5) ? position_size_pct : other.position_size_pct;
    
    return child;
}

std::string StrategyGene::toString() const {
    std::ostringstream oss;
    oss << "Primary: " << static_cast<int>(primary_indicator) << "(" << primary_period << ") @ " << primary_threshold
        << " | Secondary: " << static_cast<int>(secondary_indicator) << "(" << secondary_period << ") @ " << secondary_threshold
        << " | Entry: " << static_cast<int>(entry_condition) << " | Exit: " << static_cast<int>(exit_condition)
        << " | RR: " << risk_reward_ratio << " | SL: " << stop_loss_pct << " | TP: " << take_profit_pct
        << " | Hold: " << max_hold_time << "h | Size: " << position_size_pct;
    return oss.str();
}

std::string StrategyGene::toPineScript() const {
    std::ostringstream oss;
    
    oss << "//@version=5\n";
    oss << "strategy(\"Evolved Strategy\", overlay=true, default_qty_type=strategy.percent_of_equity, default_qty_value=" << (position_size_pct * 100) << ")\n\n";
    
    oss << "// Primary indicator\n";
    switch (primary_indicator) {
        case IndicatorType::SMA:
            oss << "primary = ta.sma(close, " << primary_period << ")\n";
            break;
        case IndicatorType::EMA:
            oss << "primary = ta.ema(close, " << primary_period << ")\n";
            break;
        case IndicatorType::RSI:
            oss << "primary = ta.rsi(close, " << primary_period << ")\n";
            break;
        case IndicatorType::MACD:
            oss << "primary = ta.macd(close, 12, 26, 9)\n";
            break;
        case IndicatorType::BB:
            oss << "primary = ta.bb(close, " << primary_period << ", 2)\n";
            break;
        case IndicatorType::ATR:
            oss << "primary = ta.atr(" << primary_period << ")\n";
            break;
        case IndicatorType::STOCH:
            oss << "primary = ta.stoch(close, high, low, " << primary_period << ")\n";
            break;
        case IndicatorType::ADX:
            oss << "primary = ta.adx(high, low, close, " << primary_period << ")\n";
            break;
    }
    
    oss << "\n// Secondary indicator\n";
    switch (secondary_indicator) {
        case IndicatorType::SMA:
            oss << "secondary = ta.sma(close, " << secondary_period << ")\n";
            break;
        case IndicatorType::EMA:
            oss << "secondary = ta.ema(close, " << secondary_period << ")\n";
            break;
        case IndicatorType::RSI:
            oss << "secondary = ta.rsi(close, " << secondary_period << ")\n";
            break;
        case IndicatorType::MACD:
            oss << "secondary = ta.macd(close, 12, 26, 9)\n";
            break;
        case IndicatorType::BB:
            oss << "secondary = ta.bb(close, " << secondary_period << ", 2)\n";
            break;
        case IndicatorType::ATR:
            oss << "secondary = ta.atr(" << secondary_period << ")\n";
            break;
        case IndicatorType::STOCH:
            oss << "secondary = ta.stoch(close, high, low, " << secondary_period << ")\n";
            break;
        case IndicatorType::ADX:
            oss << "secondary = ta.adx(high, low, close, " << secondary_period << ")\n";
            break;
    }
    
    oss << "\n// Entry conditions\n";
    switch (entry_condition) {
        case EntryCondition::CROSS_ABOVE:
            oss << "longCondition = ta.crossover(primary, " << primary_threshold << ")\n";
            break;
        case EntryCondition::CROSS_BELOW:
            oss << "longCondition = ta.crossunder(primary, " << primary_threshold << ")\n";
            break;
        case EntryCondition::ABOVE:
            oss << "longCondition = primary > " << primary_threshold << "\n";
            break;
        case EntryCondition::BELOW:
            oss << "longCondition = primary < " << primary_threshold << "\n";
            break;
        case EntryCondition::INSIDE_BB:
            oss << "longCondition = close > primary[1] and close < primary[2]\n";
            break;
        case EntryCondition::OUTSIDE_BB:
            oss << "longCondition = close < primary[1] or close > primary[2]\n";
            break;
    }
    
    oss << "\n// Exit conditions\n";
    switch (exit_condition) {
        case ExitCondition::FIXED_RR:
            oss << "strategy.entry(\"Long\", strategy.long, when=longCondition)\n";
            oss << "strategy.exit(\"Exit\", \"Long\", stop=strategy.position_avg_price * (1 - " << stop_loss_pct << "), limit=strategy.position_avg_price * (1 + " << take_profit_pct << "))\n";
            break;
        case ExitCondition::TRAILING_STOP:
            oss << "strategy.entry(\"Long\", strategy.long, when=longCondition)\n";
            oss << "strategy.exit(\"Exit\", \"Long\", trail_points=close * " << stop_loss_pct << " * 10000)\n";
            break;
        case ExitCondition::TIME_BASED:
            oss << "strategy.entry(\"Long\", strategy.long, when=longCondition)\n";
            oss << "strategy.close(\"Long\", when=time - strategy.opentrades.entry_time(0) > " << max_hold_time << " * 60 * 60 * 1000)\n";
            break;
        case ExitCondition::INDICATOR_SIGNAL:
            oss << "strategy.entry(\"Long\", strategy.long, when=longCondition)\n";
            oss << "strategy.close(\"Long\", when=secondary < " << secondary_threshold << ")\n";
            break;
    }
    
    return oss.str();
}

std::string FitnessResult::toString() const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(4);
    oss << "Return: " << (total_return * 100) << "% | Sharpe: " << sharpe_ratio 
        << " | MaxDD: " << (max_drawdown * 100) << "% | WinRate: " << (win_rate * 100) << "%"
        << " | Trades: " << total_trades << " | PF: " << profit_factor 
        << " | Calmar: " << calmar_ratio << " | Fitness: " << fitness_score;
    return oss.str();
}

GeneticAlgorithm::GeneticAlgorithm(const std::vector<OHLCV>& data, int population_size, int generations, double mutation_rate, double crossover_rate)
    : data_(data), population_size_(population_size), generations_(generations), 
      mutation_rate_(mutation_rate), crossover_rate_(crossover_rate) {
    
    std::random_device rd;
    rng_.seed(rd());
    
    std::cout << "[INFO] Genetic Algorithm initialized with " << data_.size() << " bars, population: " << population_size_ << ", generations: " << generations_ << std::endl;
}

std::vector<StrategyGene> GeneticAlgorithm::evolve() {
    std::cout << "[INFO] Starting genetic algorithm evolution..." << std::endl;
    
    initializePopulation();
    
    for (int generation = 0; generation < generations_; ++generation) {
        std::cout << "[INFO] Generation " << (generation + 1) << "/" << generations_ << std::endl;
        
        evaluatePopulation();
        
        auto best_in_gen = std::max_element(population_.begin(), population_.end(), 
            [](const StrategyGene& a, const StrategyGene& b) { return a.fitness < b.fitness; });
        
        if (best_in_gen->fitness > best_strategy_.fitness) {
            best_strategy_ = *best_in_gen;
            best_fitness_ = evaluateFitness(best_strategy_);
            std::cout << "[INFO] New best strategy found! Fitness: " << best_strategy_.fitness << std::endl;
            std::cout << "[INFO] " << best_fitness_.toString() << std::endl;
        }
        
        selectParents();
        crossover();
        mutate();
        elitism();
    }
    
    std::cout << "[INFO] Evolution complete! Best fitness: " << best_strategy_.fitness << std::endl;
    return population_;
}

void GeneticAlgorithm::initializePopulation() {
    population_.clear();
    for (int i = 0; i < population_size_; ++i) {
        population_.push_back(StrategyGene::random(rng_));
    }
}

void GeneticAlgorithm::evaluatePopulation() {
    for (auto& gene : population_) {
        FitnessResult result = evaluateFitness(gene);
        gene.fitness = result.fitness_score;
    }
}

FitnessResult GeneticAlgorithm::evaluateFitness(const StrategyGene& gene) {
    EvolvedStrategy strategy(gene);
    
    std::vector<double> equity_curve;
    std::vector<double> returns;
    std::vector<double> profits, losses;
    
    double current_equity = 10000.0;
    int winning_trades = 0;
    int total_trades = 0;
    
    for (size_t i = 0; i < data_.size(); ++i) {
        TradeSignal signal = strategy.generateSignal(data_, i);
        
        if (signal.type == SignalType::BUY) {
            double entry_price = data_[i].close;
            double stop_loss = signal.stop_loss;
            double take_profit = signal.take_profit;
            
            for (size_t j = i + 1; j < data_.size(); ++j) {
                if (data_[j].low <= stop_loss || data_[j].high >= take_profit) {
                    double exit_price = (data_[j].low <= stop_loss) ? stop_loss : take_profit;
                    double trade_return = (exit_price - entry_price) / entry_price;
                    
                    if (trade_return > 0) {
                        winning_trades++;
                        profits.push_back(trade_return);
                    } else {
                        losses.push_back(-trade_return);
                    }
                    
                    total_trades++;
                    current_equity *= (1 + trade_return * gene.position_size_pct);
                    break;
                }
            }
        }
        
        equity_curve.push_back(current_equity);
        if (i > 0) {
            returns.push_back((current_equity - equity_curve[i-1]) / equity_curve[i-1]);
        }
    }
    
    FitnessResult result;
    result.total_return = (current_equity - 10000.0) / 10000.0;
    result.sharpe_ratio = calculateSharpeRatio(returns);
    result.max_drawdown = calculateMaxDrawdown(equity_curve);
    result.win_rate = (total_trades > 0) ? static_cast<double>(winning_trades) / total_trades : 0.0;
    result.total_trades = total_trades;
    result.profit_factor = calculateProfitFactor(profits, losses);
    result.calmar_ratio = (result.max_drawdown > 0) ? result.total_return / result.max_drawdown : 0.0;
    
    result.fitness_score = result.sharpe_ratio * 0.4 + result.total_return * 0.3 + 
                          result.win_rate * 0.2 + result.profit_factor * 0.1 - 
                          result.max_drawdown * 0.5;
    
    return result;
}

void GeneticAlgorithm::selectParents() {
    std::vector<StrategyGene> new_population;
    std::uniform_int_distribution<int> dist(0, population_size_ - 1);
    
    for (int i = 0; i < population_size_; ++i) {
        int best_idx = dist(rng_);
        for (int j = 0; j < 2; ++j) {
            int candidate = dist(rng_);
            if (population_[candidate].fitness > population_[best_idx].fitness) {
                best_idx = candidate;
            }
        }
        new_population.push_back(population_[best_idx]);
    }
    
    population_ = new_population;
}

void GeneticAlgorithm::crossover() {
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    
    for (int i = 0; i < population_size_ - 1; i += 2) {
        if (dist(rng_) < crossover_rate_) {
            StrategyGene child1 = population_[i].crossover(population_[i + 1], rng_);
            StrategyGene child2 = population_[i + 1].crossover(population_[i], rng_);
            population_[i] = child1;
            population_[i + 1] = child2;
        }
    }
}

void GeneticAlgorithm::mutate() {
    for (auto& gene : population_) {
        gene.mutate(rng_, mutation_rate_);
    }
}

void GeneticAlgorithm::elitism() {
    auto best = std::max_element(population_.begin(), population_.end(), 
        [](const StrategyGene& a, const StrategyGene& b) { return a.fitness < b.fitness; });
    
    if (best_strategy_.fitness > best->fitness) {
        *best = best_strategy_;
    }
}

StrategyGene GeneticAlgorithm::getBestStrategy() const {
    return best_strategy_;
}

std::string GeneticAlgorithm::exportBestToPineScript() const {
    return best_strategy_.toPineScript();
}

double GeneticAlgorithm::calculateSharpeRatio(const std::vector<double>& returns) {
    if (returns.empty()) return 0.0;
    
    double mean = std::accumulate(returns.begin(), returns.end(), 0.0) / returns.size();
    double variance = 0.0;
    for (double ret : returns) {
        variance += (ret - mean) * (ret - mean);
    }
    variance /= returns.size();
    
    double std_dev = std::sqrt(variance);
    return (std_dev > 0) ? mean / std_dev : 0.0;
}

double GeneticAlgorithm::calculateMaxDrawdown(const std::vector<double>& equity_curve) {
    if (equity_curve.empty()) return 0.0;
    
    double max_dd = 0.0;
    double peak = equity_curve[0];
    
    for (double equity : equity_curve) {
        if (equity > peak) {
            peak = equity;
        }
        double dd = (peak - equity) / peak;
        if (dd > max_dd) {
            max_dd = dd;
        }
    }
    
    return max_dd;
}

double GeneticAlgorithm::calculateProfitFactor(const std::vector<double>& profits, const std::vector<double>& losses) {
    double total_profit = std::accumulate(profits.begin(), profits.end(), 0.0);
    double total_loss = std::accumulate(losses.begin(), losses.end(), 0.0);
    
    return (total_loss > 0) ? total_profit / total_loss : (total_profit > 0) ? 1000.0 : 0.0;
}

EvolvedStrategy::EvolvedStrategy(const StrategyGene& gene) : gene_(gene) {}

TradeSignal EvolvedStrategy::generateSignal(const std::vector<OHLCV>& data, size_t current_index) {
    if (!precomputed_) {
        precomputeIndicators(data);
    }
    
    if (current_index < std::max(gene_.primary_period, gene_.secondary_period)) {
        return {SignalType::NONE, current_index, 0.0, 0.0, "Not enough data"};
    }
    
    if (checkEntryCondition(data, current_index)) {
        double stop_loss = calculateStopLoss(data, current_index);
        double take_profit = calculateTakeProfit(data, current_index);
        
        return {
            SignalType::BUY,
            current_index,
            stop_loss,
            take_profit,
            "Evolved Strategy Signal"
        };
    }
    
    return {SignalType::NONE, current_index, 0.0, 0.0, "No signal"};
}

void EvolvedStrategy::precomputeIndicators(const std::vector<OHLCV>& data) {
    primary_values_.resize(data.size());
    secondary_values_.resize(data.size());
    
    for (size_t i = 0; i < data.size(); ++i) {
        primary_values_[i] = calculateIndicator(data, i, gene_.primary_indicator, gene_.primary_period);
        secondary_values_[i] = calculateIndicator(data, i, gene_.secondary_indicator, gene_.secondary_period);
    }
    
    precomputed_ = true;
}

double EvolvedStrategy::calculateIndicator(const std::vector<OHLCV>& data, size_t index, StrategyGene::IndicatorType type, int period) {
    switch (type) {
        case StrategyGene::IndicatorType::SMA:
            return Indicators::SMA(data, index, period);
        case StrategyGene::IndicatorType::RSI:
            return Indicators::RSI(data, index, period);
        default:
            return 0.0;
    }
}

bool EvolvedStrategy::checkEntryCondition(const std::vector<OHLCV>& data, size_t index) {
    double primary_val = primary_values_[index];
    double secondary_val = secondary_values_[index];
    double close = data[index].close;
    
    switch (gene_.entry_condition) {
        case StrategyGene::EntryCondition::CROSS_ABOVE:
            return primary_val > gene_.primary_threshold && 
                   primary_values_[index-1] <= gene_.primary_threshold;
        case StrategyGene::EntryCondition::CROSS_BELOW:
            return primary_val < gene_.primary_threshold && 
                   primary_values_[index-1] >= gene_.primary_threshold;
        case StrategyGene::EntryCondition::ABOVE:
            return primary_val > gene_.primary_threshold && 
                   secondary_val > gene_.secondary_threshold;
        case StrategyGene::EntryCondition::BELOW:
            return primary_val < gene_.primary_threshold && 
                   secondary_val < gene_.secondary_threshold;
        default:
            return false;
    }
}

double EvolvedStrategy::calculateStopLoss(const std::vector<OHLCV>& data, size_t index) {
    return data[index].close * (1.0 - gene_.stop_loss_pct);
}

double EvolvedStrategy::calculateTakeProfit(const std::vector<OHLCV>& data, size_t index) {
    return data[index].close * (1.0 + gene_.take_profit_pct);
} 