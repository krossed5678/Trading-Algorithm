# 🚀 High-Performance C++ Trading Algorithm Backtester

A sophisticated, GPU-accelerated trading algorithm backtester built in C++ with real-time GUI, advanced technical indicators, and comprehensive performance analytics.

## ✨ Features

### 🎯 **Core Trading Engine**
- Modular, extensible strategy framework
- Genetic algorithm for machine learning-based strategy optimization
- Real-time and batch backtesting
- Risk management and performance analytics

### ⚡ **High-Performance Computing**
- **CUDA GPU Acceleration** for population-wide fitness evaluation (requires NVIDIA GPU and CUDA Toolkit)
- SIMD-optimized CPU math for fallback
- Real-time GUI with live parameter adjustment

### 🖥️ **Interactive GUI**
- Dear ImGui interface for live strategy tuning
- 60 FPS updates, real-time equity curve, and performance metrics

### 📈 **Data Management**
- Polygon.io integration for 1-minute SPY data
- CSV data loading

## 🏗️ Architecture

```
Trading-Algorithm/
├── include/                 # Header files
├── src/                    # Source files
├── data/                   # Market data storage
├── overnight_training.bat  # Batch script for overnight genetic training
├── requirements.txt        # Python dependencies for data scripts
├── fetch_spy_data.py       # Python script to fetch SPY data
├── CMakeLists.txt          # Build configuration
```

## 🚀 Quick Start

### Prerequisites
- **C++17** compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- **CMake** 3.16 or higher
- **CUDA Toolkit** 11.0+ (for GPU acceleration, optional but recommended)
- **Python 3.7+** (for data fetching)
- **Polygon.io API key** (for data fetching)

### Installation
1. **Clone the repository**
   ```bash
   git clone https://github.com/krossed5678/Trading-Algorithm.git
   cd Trading-Algorithm
   ```
2. **Set up environment variables**
   - Windows: `setup_env.bat`
   - Linux/Mac: `setup_env.sh`
   - Or manually create a `.env` file with your API keys
3. **Build the project**
   ```bash
   mkdir build && cd build
   cmake ..
   cmake --build . --config Release
   ```
   - The build system will auto-detect CUDA. If found, GPU acceleration is enabled for the genetic algorithm.
4. **Fetch market data**
   ```bash
   python fetch_spy_data.py
   # Data will be saved in data/SPY_1m.csv
   ```
5. **Run the GUI**
   ```bash
   cd build/Release
   trading_bot_gui.exe
   ```
6. **Run overnight genetic training**
   ```bash
   cd build/Release
   ../../overnight_training.bat
   # Results in overnight_results/
   ```

## ⚡ GPU Acceleration
- The genetic algorithm now uses CUDA to accelerate population-wide fitness evaluation if an NVIDIA GPU and CUDA Toolkit are available.
- The build system will automatically enable GPU mode if CUDA is detected.
- If CUDA is not available, the system will fall back to CPU evaluation.
- No manual configuration is needed—just install CUDA and rebuild!

## 🧪 Automated Genetic Training
- Use `overnight_training.bat` to run long genetic optimization sessions.
- All logs and the latest Pine Script (`best_strategy.pine`) are saved in `overnight_results/`.
- The Pine Script is always deleted and rewritten after every session, so it is always up to date.

## 🧹 Redundant/Obsolete Files
- The following files are no longer needed and can be deleted if present:
  - `env.example` (use `.env` instead)
  - Any old or unused test scripts not referenced above
  - Old build artifacts in `build/` except for `Release/` and `overnight_results/`

## 📝 Notes
- For custom strategies, see `include/Strategy.hpp` and the provided examples.
- For advanced users: you can extend GPU acceleration by porting additional strategy logic to CUDA in `src/gpu_kernels.cu`.

## 📦 Python Requirements
- See `requirements.txt` for Python dependencies (used only for data fetching scripts).

---

For more details, see the in-code documentation and comments.
