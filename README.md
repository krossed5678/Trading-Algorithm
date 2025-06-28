# 🚀 High-Performance C++ Trading Algorithm Backtester

A sophisticated, GPU-accelerated trading algorithm backtester built in C++ with real-time GUI, advanced technical indicators, and comprehensive performance analytics.

## ✨ Features

### 🎯 **Core Trading Engine**
- **Modular Strategy Framework** - Easy to implement and test new trading strategies
- **Golden Foundation Strategy** - Advanced trend-following strategy with RSI filtering and Fair Value Gap detection
- **Risk Management** - Configurable stop-loss and take-profit levels with risk-reward optimization
- **Performance Analytics** - Detailed P&L tracking, annualized returns, and yearly performance breakdown

### ⚡ **High-Performance Computing**
- **CUDA GPU Acceleration** - Parallel processing of technical indicators and signal generation
- **Hybrid CPU/GPU Architecture** - Automatic fallback to CPU when GPU unavailable
- **Optimized Data Structures** - Memory-efficient OHLCV data handling
- **Real-time Processing** - Sub-second backtest execution for large datasets

### 🖥️ **Interactive GUI**
- **Dear ImGui Interface** - Native, responsive GUI with real-time parameter adjustment
- **Live Backtesting** - Continuous strategy evaluation as parameters change
- **Performance Visualization** - Real-time equity curve and P&L display
- **Mode Selection** - Toggle between CPU and GPU execution modes

### 📊 **Data Management**
- **Polygon.io Integration** - Fetch high-quality 1-minute SPY data
- **CSV Data Loading** - Support for custom OHLCV datasets
- **Historical Data Processing** - Efficient handling of large time series
- **Data Validation** - Automatic format checking and error handling

### 📈 **Technical Indicators**
- **Simple Moving Averages (SMA)** - Fast, GPU-accelerated calculations
- **Relative Strength Index (RSI)** - Momentum oscillator with configurable periods
- **Fair Value Gap Detection** - Advanced price action analysis
- **Trend Detection** - Multi-timeframe trend identification

## 🏗️ Architecture

```
tradingAlgo/
├── include/                 # Header files
│   ├── Backtester.hpp      # Core backtesting engine
│   ├── DataLoader.hpp      # Data loading and processing
│   ├── GPUStrategy.hpp     # GPU-accelerated strategy
│   ├── MovingAverage.hpp   # Technical indicators
│   ├── Strategy.hpp        # Strategy interface
│   └── TradeLog.hpp        # Trade logging system
├── src/                    # Source files
│   ├── gui_main.cpp        # GUI application entry point
│   ├── main.cpp           # Console application entry point
│   ├── gpu_kernels.cu     # CUDA kernels for GPU acceleration
│   └── ...                # Implementation files
├── data/                   # Market data storage
├── tests/                  # Unit tests
└── CMakeLists.txt         # Build configuration
```

## 🚀 Quick Start

### Prerequisites

- **C++17** compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- **CMake** 3.16 or higher
- **CUDA Toolkit** 11.0+ (optional, for GPU acceleration)
- **Python 3.7+** (for data fetching scripts)

### Installation

1. **Clone the repository**
   ```bash
   git clone (https://github.com/krossed5678/Trading-Algorithm.git)
   cd tradingAlgo
   ```

2. **Set Up Environment Variables**
   **Windows:**
   ```bash
   setup_env.bat
   ```

   **Linux/Mac:**
   ```bash
   chmod +x setup_env.sh
   ./setup_env.sh
   ```

   **Manual Setup:**
   ```bash
   cp env.example .env
   # Edit .env and add your API keys
   ```

3. **Build the project**
   ```bash
   mkdir build && cd build
   cmake ..
   make -j$(nproc)
   ```

4. **Fetch market data** (optional)
   ```bash
   # Set your Polygon.io API key
   export POLYGON_API_KEY="your_api_key_here"
   
   # Fetch SPY data
   python3 fetch_spy_data.py
   ```

5. **Run the application**
   ```bash
   # GUI version (recommended)
   ./tradingAlgo_gui
   
   # Console version
   ./tradingAlgo
   ```

## 🎮 Usage

### GUI Application

The GUI provides an intuitive interface for strategy testing:

1. **Set Initial Parameters**
   - Initial capital amount
   - Risk/reward ratio (Safe to Very Risky)
   - Strategy parameters

2. **Choose Execution Mode**
   - **CPU Mode**: Reliable, compatible execution
   - **GPU Mode**: High-performance CUDA acceleration

3. **Real-time Results**
   - Live P&L updates
   - Performance metrics
   - Trade statistics

### Console Application

For automated testing and scripting:

```bash
./tradingAlgo --data data/spy_data.csv --strategy golden --initial-capital 10000
```

## 📊 Strategy Details

### Golden Foundation Strategy

The core strategy implements a sophisticated trend-following approach:

1. **Trend Detection**
   - Uses multiple SMA periods for trend confirmation
   - Identifies strong directional moves

2. **Entry Conditions**
   - RSI oversold/overbought filtering
   - Fair Value Gap detection for optimal entry points
   - Volume confirmation

3. **Risk Management**
   - Dynamic stop-loss placement
   - Configurable risk-reward ratios
   - Position sizing based on volatility

4. **Exit Strategy**
   - Take-profit targets
   - Trailing stops
   - Trend reversal signals

## 🛠️ Creating Custom Strategies

The framework provides a flexible interface for implementing custom trading strategies. Here's how to create your own:

### Strategy Interface

All strategies must inherit from the `Strategy` base class:

```cpp
// include/Strategy.hpp
class Strategy {
public:
    virtual ~Strategy() = default;
    virtual TradeSignal generateSignal(const std::vector<OHLCV>& data, size_t index) = 0;
    virtual void setParameters(const std::map<std::string, double>& params) = 0;
};
```

### Example: Simple Moving Average Crossover Strategy

```cpp
// include/SMACrossoverStrategy.hpp
#pragma once
#include "Strategy.hpp"
#include "MovingAverage.hpp"

class SMACrossoverStrategy : public Strategy {
private:
    int short_period_;
    int long_period_;
    double risk_reward_ratio_;
    
public:
    SMACrossoverStrategy(int short_period = 10, int long_period = 20, double rr_ratio = 2.0)
        : short_period_(short_period), long_period_(long_period), risk_reward_ratio_(rr_ratio) {}
    
    TradeSignal generateSignal(const std::vector<OHLCV>& data, size_t index) override {
        if (index < long_period_) return {SignalType::HOLD, 0, 0};
        
        // Calculate SMAs
        double short_sma = calculateSMA(data, index, short_period_);
        double long_sma = calculateSMA(data, index, long_period_);
        double prev_short_sma = calculateSMA(data, index - 1, short_period_);
        double prev_long_sma = calculateSMA(data, index - 1, long_period_);
        
        // Check for crossover
        bool bullish_crossover = (prev_short_sma <= prev_long_sma) && (short_sma > long_sma);
        bool bearish_crossover = (prev_short_sma >= prev_long_sma) && (short_sma < long_sma);
        
        if (bullish_crossover) {
            double entry_price = data[index].close;
            double stop_loss = entry_price * 0.98; // 2% stop loss
            double take_profit = entry_price + (entry_price - stop_loss) * risk_reward_ratio_;
            
            return {SignalType::BUY, stop_loss, take_profit};
        }
        
        return {SignalType::HOLD, 0, 0};
    }
    
    void setParameters(const std::map<std::string, double>& params) override {
        if (params.count("short_period")) short_period_ = params.at("short_period");
        if (params.count("long_period")) long_period_ = params.at("long_period");
        if (params.count("risk_reward")) risk_reward_ratio_ = params.at("risk_reward");
    }
    
private:
    double calculateSMA(const std::vector<OHLCV>& data, size_t end_index, int period) {
        double sum = 0.0;
        for (int i = 0; i < period; ++i) {
            sum += data[end_index - i].close;
        }
        return sum / period;
    }
};
```

### Example: RSI Mean Reversion Strategy

```cpp
// include/RSIStrategy.hpp
#pragma once
#include "Strategy.hpp"

class RSIStrategy : public Strategy {
private:
    int rsi_period_;
    double oversold_threshold_;
    double overbought_threshold_;
    double risk_reward_ratio_;
    
public:
    RSIStrategy(int period = 14, double oversold = 30, double overbought = 70, double rr_ratio = 1.5)
        : rsi_period_(period), oversold_threshold_(oversold), 
          overbought_threshold_(overbought), risk_reward_ratio_(rr_ratio) {}
    
    TradeSignal generateSignal(const std::vector<OHLCV>& data, size_t index) override {
        if (index < rsi_period_) return {SignalType::HOLD, 0, 0};
        
        double rsi = calculateRSI(data, index, rsi_period_);
        double current_price = data[index].close;
        
        // Oversold condition - potential buy
        if (rsi < oversold_threshold_) {
            double stop_loss = current_price * 0.95; // 5% stop loss
            double take_profit = current_price + (current_price - stop_loss) * risk_reward_ratio_;
            
            return {SignalType::BUY, stop_loss, take_profit};
        }
        
        // Overbought condition - potential sell
        if (rsi > overbought_threshold_) {
            double stop_loss = current_price * 1.05; // 5% stop loss
            double take_profit = current_price - (stop_loss - current_price) * risk_reward_ratio_;
            
            return {SignalType::SELL, stop_loss, take_profit};
        }
        
        return {SignalType::HOLD, 0, 0};
    }
    
    void setParameters(const std::map<std::string, double>& params) override {
        if (params.count("rsi_period")) rsi_period_ = params.at("rsi_period");
        if (params.count("oversold")) oversold_threshold_ = params.at("oversold");
        if (params.count("overbought")) overbought_threshold_ = params.at("overbought");
        if (params.count("risk_reward")) risk_reward_ratio_ = params.at("risk_reward");
    }
    
private:
    double calculateRSI(const std::vector<OHLCV>& data, size_t end_index, int period) {
        double gains = 0.0, losses = 0.0;
        
        for (int i = 1; i <= period; ++i) {
            double change = data[end_index - i + 1].close - data[end_index - i].close;
            if (change > 0) gains += change;
            else losses -= change;
        }
        
        double avg_gain = gains / period;
        double avg_loss = losses / period;
        
        if (avg_loss == 0) return 100.0;
        
        double rs = avg_gain / avg_loss;
        return 100.0 - (100.0 / (1.0 + rs));
    }
};
```

### Example: Bollinger Bands Strategy

```cpp
// include/BollingerBandsStrategy.hpp
#pragma once
#include "Strategy.hpp"
#include <cmath>

class BollingerBandsStrategy : public Strategy {
private:
    int period_;
    double std_dev_multiplier_;
    double risk_reward_ratio_;
    
public:
    BollingerBandsStrategy(int period = 20, double std_dev = 2.0, double rr_ratio = 2.0)
        : period_(period), std_dev_multiplier_(std_dev), risk_reward_ratio_(rr_ratio) {}
    
    TradeSignal generateSignal(const std::vector<OHLCV>& data, size_t index) override {
        if (index < period_) return {SignalType::HOLD, 0, 0};
        
        auto bands = calculateBollingerBands(data, index, period_, std_dev_multiplier_);
        double current_price = data[index].close;
        
        // Price touches lower band - potential buy
        if (current_price <= bands.lower_band) {
            double stop_loss = bands.lower_band * 0.98;
            double take_profit = current_price + (current_price - stop_loss) * risk_reward_ratio_;
            
            return {SignalType::BUY, stop_loss, take_profit};
        }
        
        // Price touches upper band - potential sell
        if (current_price >= bands.upper_band) {
            double stop_loss = bands.upper_band * 1.02;
            double take_profit = current_price - (stop_loss - current_price) * risk_reward_ratio_;
            
            return {SignalType::SELL, stop_loss, take_profit};
        }
        
        return {SignalType::HOLD, 0, 0};
    }
    
    void setParameters(const std::map<std::string, double>& params) override {
        if (params.count("period")) period_ = params.at("period");
        if (params.count("std_dev")) std_dev_multiplier_ = params.at("std_dev");
        if (params.count("risk_reward")) risk_reward_ratio_ = params.at("risk_reward");
    }
    
private:
    struct BollingerBands {
        double upper_band, middle_band, lower_band;
    };
    
    BollingerBands calculateBollingerBands(const std::vector<OHLCV>& data, size_t end_index, 
                                          int period, double std_dev_mult) {
        // Calculate SMA
        double sum = 0.0;
        for (int i = 0; i < period; ++i) {
            sum += data[end_index - i].close;
        }
        double sma = sum / period;
        
        // Calculate standard deviation
        double variance = 0.0;
        for (int i = 0; i < period; ++i) {
            double diff = data[end_index - i].close - sma;
            variance += diff * diff;
        }
        double std_dev = std::sqrt(variance / period);
        
        return {
            sma + (std_dev * std_dev_mult),  // Upper band
            sma,                             // Middle band
            sma - (std_dev * std_dev_mult)   // Lower band
        };
    }
};
```

### Using Your Custom Strategy

1. **Create the header file** (`include/MyStrategy.hpp`)
2. **Implement the strategy** (`src/MyStrategy.cpp`)
3. **Register with the backtester**:

```cpp
// In your main application
#include "MyStrategy.hpp"

int main() {
    // Load data
    DataLoader loader;
    auto data = loader.loadCSV("data/spy_data.csv");
    
    // Create and configure your strategy
    MyStrategy strategy;
    std::map<std::string, double> params = {
        {"param1", 10.0},
        {"param2", 20.0},
        {"risk_reward", 2.0}
    };
    strategy.setParameters(params);
    
    // Run backtest
    Backtester backtester(data, &strategy, 10000.0);
    backtester.run();
    backtester.printTotalGain();
    
    return 0;
}
```

### Strategy Best Practices

1. **Parameter Validation**: Always validate input parameters
2. **Edge Case Handling**: Check for sufficient data before calculations
3. **Memory Efficiency**: Avoid unnecessary data copying
4. **Risk Management**: Always include stop-loss and take-profit levels
5. **Performance**: Pre-calculate indicators when possible
6. **Documentation**: Document your strategy logic and parameters

### GPU-Accelerated Strategies

For high-performance strategies, you can also implement GPU versions:

```cpp
// include/GPUMyStrategy.hpp
class GPUMyStrategy : public Strategy {
private:
    // GPU memory buffers
    float* d_prices_;
    float* d_indicators_;
    // ... other GPU resources
    
public:
    TradeSignal generateSignal(const std::vector<OHLCV>& data, size_t index) override {
        // Use GPU kernels for indicator calculation
        calculateIndicatorsGPU(data, index);
        
        // Generate signals on CPU (or GPU if complex)
        return generateSignalsCPU(data, index);
    }
};
```

This modular approach allows you to easily test and compare different strategies while maintaining high performance through GPU acceleration.

## 🐍 Python Scripts

The project includes Python scripts for data management and auto-updates:

### Data Fetching
```bash
python fetch_spy_data.py
```
- Downloads SPY 1-minute data from Polygon.io
- Requires `POLYGON_API_KEY` in `.env` file
- Creates `data/SPY_1m.csv` automatically
- Handles network errors and retries gracefully

### Auto-Updater
```bash
python updater.py
```
- Checks GitHub releases for updates
- Downloads and installs new versions safely
- Creates backups of existing executables
- Handles process management (waits for running apps to close)
- Can restart the application after update

### Installing Python Dependencies
**Windows:**
```bash
install_python_deps.bat
```

**Linux/Mac:**
```bash
pip install -r requirements.txt
```

### Python Script Features
- **Environment Integration**: Uses `.env` file for API keys
- **Error Handling**: Robust error handling and user feedback
- **Backup System**: Automatic backup creation before updates
- **Process Management**: Safely handles running applications
- **Cross-platform**: Works on Windows, Linux, and macOS

## 🔧 Configuration

### Build Options

```