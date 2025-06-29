#include "../include/MovingAverage.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <immintrin.h> // For AVX/SSE instructions
#include <numeric>

namespace Indicators {
    
    // Optimized SMA using SIMD and better memory access patterns
    double SMA(const std::vector<OHLCV>& data, size_t end_index, size_t period) {
        if (end_index + 1 < period) {
            // std::cerr << "[DEBUG] SMA: Not enough data for period " << period << " at index " << end_index << std::endl;
            return 0.0;
        }
        
        // Use SIMD for large periods
        if (period >= 8) {
            size_t start_idx = end_index + 1 - period;
            const double* prices = &data[start_idx].close;
            
            // Use AVX2 if available
            #ifdef __AVX2__
            if (period >= 8) {
                __m256d sum = _mm256_setzero_pd();
                size_t simd_loops = period / 4;
                size_t remainder = period % 4;
                
                // Process 4 doubles at a time
                for (size_t i = 0; i < simd_loops; ++i) {
                    __m256d chunk = _mm256_loadu_pd(&prices[i * 4]);
                    sum = _mm256_add_pd(sum, chunk);
                }
                
                // Horizontal sum
                double simd_sum[4];
                _mm256_storeu_pd(simd_sum, sum);
                double total = simd_sum[0] + simd_sum[1] + simd_sum[2] + simd_sum[3];
                
                // Handle remainder
                for (size_t i = simd_loops * 4; i < period; ++i) {
                    total += prices[i];
                }
                
                return total / period;
            }
            #endif
            
            // Fallback to SSE for smaller periods or when AVX2 not available
            #ifdef __SSE2__
            if (period >= 4) {
                __m128d sum = _mm_setzero_pd();
                size_t simd_loops = period / 2;
                size_t remainder = period % 2;
                
                // Process 2 doubles at a time
                for (size_t i = 0; i < simd_loops; ++i) {
                    __m128d chunk = _mm_loadu_pd(&prices[i * 2]);
                    sum = _mm_add_pd(sum, chunk);
                }
                
                // Horizontal sum
                double simd_sum[2];
                _mm_storeu_pd(simd_sum, sum);
                double total = simd_sum[0] + simd_sum[1];
                
                // Handle remainder
                for (size_t i = simd_loops * 2; i < period; ++i) {
                    total += prices[i];
                }
                
                return total / period;
            }
            #endif
        }
        
        // Standard implementation for small periods
        double sum = 0.0;
        for (size_t i = end_index + 1 - period; i <= end_index; ++i) {
            sum += data[i].close;
        }
        return sum / period;
    }

    // Optimized RSI with SIMD and better numerical stability
    double RSI(const std::vector<OHLCV>& data, size_t end_index, size_t period) {
        if (end_index < period) {
            // std::cerr << "[DEBUG] RSI: Not enough data for period " << period << " at index " << end_index << std::endl;
            return 50.0;
        }
        
        // Pre-allocate arrays for better cache locality
        std::vector<double> gains(period);
        std::vector<double> losses(period);
        
        size_t gains_count = 0, losses_count = 0;
        
        // Calculate price changes and separate gains/losses
        for (size_t i = end_index - period + 1; i <= end_index; ++i) {
            if (i == 0) continue;
            double change = data[i].close - data[i - 1].close;
            if (change > 0) {
                gains[gains_count++] = change;
            } else if (change < 0) {
                losses[losses_count++] = -change;
            }
        }
        
        // Use SIMD for summing gains and losses
        double total_gain = 0.0, total_loss = 0.0;
        
        #ifdef __AVX2__
        if (gains_count >= 4) {
            __m256d gain_sum = _mm256_setzero_pd();
            size_t simd_loops = gains_count / 4;
            
            for (size_t i = 0; i < simd_loops; ++i) {
                __m256d chunk = _mm256_loadu_pd(&gains[i * 4]);
                gain_sum = _mm256_add_pd(gain_sum, chunk);
            }
            
            double simd_gains[4];
            _mm256_storeu_pd(simd_gains, gain_sum);
            total_gain = simd_gains[0] + simd_gains[1] + simd_gains[2] + simd_gains[3];
            
            // Handle remainder
            for (size_t i = simd_loops * 4; i < gains_count; ++i) {
                total_gain += gains[i];
            }
        } else {
            total_gain = std::accumulate(gains.begin(), gains.begin() + gains_count, 0.0);
        }
        #else
        total_gain = std::accumulate(gains.begin(), gains.begin() + gains_count, 0.0);
        #endif
        
        #ifdef __AVX2__
        if (losses_count >= 4) {
            __m256d loss_sum = _mm256_setzero_pd();
            size_t simd_loops = losses_count / 4;
            
            for (size_t i = 0; i < simd_loops; ++i) {
                __m256d chunk = _mm256_loadu_pd(&losses[i * 4]);
                loss_sum = _mm256_add_pd(loss_sum, chunk);
            }
            
            double simd_losses[4];
            _mm256_storeu_pd(simd_losses, loss_sum);
            total_loss = simd_losses[0] + simd_losses[1] + simd_losses[2] + simd_losses[3];
            
            // Handle remainder
            for (size_t i = simd_loops * 4; i < losses_count; ++i) {
                total_loss += losses[i];
            }
        } else {
            total_loss = std::accumulate(losses.begin(), losses.begin() + losses_count, 0.0);
        }
        #else
        total_loss = std::accumulate(losses.begin(), losses.begin() + losses_count, 0.0);
        #endif
        
        // Improved numerical stability
        if (total_gain + total_loss < 1e-10) return 50.0;
        
        double avg_gain = total_gain / period;
        double avg_loss = total_loss / period;
        
        if (avg_loss < 1e-10) return 100.0;
        
        double rs = avg_gain / avg_loss;
        return 100.0 - (100.0 / (1.0 + rs));
    }

    // Optimized FVG detection with better logic
    bool detectFVG(const std::vector<OHLCV>& data, size_t end_index) {
        if (end_index < 2) {
            // std::cerr << "[DEBUG] FVG: Not enough data at index " << end_index << std::endl;
            return false;
        }
        
        const auto& prev = data[end_index - 1];
        const auto& curr = data[end_index];
        
        // More precise FVG detection
        double gap_threshold = 0.001; // 0.1% threshold
        
        // Bullish FVG: current low > previous high
        if (curr.low > prev.high * (1.0 + gap_threshold)) {
            return true;
        }
        
        // Bearish FVG: current high < previous low
        if (curr.high < prev.low * (1.0 - gap_threshold)) {
            return true;
        }
        
        return false;
    }
    
    // New optimized function for batch calculations
    void calculateBatchIndicators(const std::vector<OHLCV>& data, 
                                 std::vector<double>& sma_values,
                                 std::vector<double>& rsi_values,
                                 size_t sma_period, size_t rsi_period) {
        size_t n = data.size();
        sma_values.resize(n);
        rsi_values.resize(n);
        
        // Pre-extract close prices for better cache locality
        std::vector<double> prices(n);
        for (size_t i = 0; i < n; ++i) {
            prices[i] = data[i].close;
        }
        
        // Batch calculate SMA with sliding window optimization
        if (n >= sma_period) {
            // Calculate first SMA
            double sum = 0.0;
            for (size_t i = 0; i < sma_period; ++i) {
                sum += prices[i];
            }
            sma_values[sma_period - 1] = sum / sma_period;
            
            // Use sliding window for subsequent values
            for (size_t i = sma_period; i < n; ++i) {
                sum = sum - prices[i - sma_period] + prices[i];
                sma_values[i] = sum / sma_period;
            }
        }
        
        // Batch calculate RSI
        for (size_t i = rsi_period; i < n; ++i) {
            rsi_values[i] = RSI(data, i, rsi_period);
        }
        
        // Fill initial values
        for (size_t i = 0; i < std::min(sma_period - 1, n); ++i) {
            sma_values[i] = 0.0;
        }
        for (size_t i = 0; i < std::min(rsi_period, n); ++i) {
            rsi_values[i] = 50.0;
        }
    }
}