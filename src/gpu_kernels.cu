#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <thrust/device_vector.h>
#include <thrust/host_vector.h>
#include <iostream>

// Error checking macro
#define CUDA_CHECK(call) \
    do { \
        cudaError_t err = call; \
        if (err != cudaSuccess) { \
            std::cerr << "CUDA error: " << cudaGetErrorString(err) << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
            return; \
        } \
    } while(0)

// Optimized SMA kernel using shared memory for small/medium periods
__global__ void sma_kernel(const double* prices, double* sma, int n, int period) {
    extern __shared__ double s_prices[];
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    int tid = threadIdx.x;
    if (idx >= n) return;

    // Load data into shared memory (window for this block)
    int window_start = idx - period + 1;
    int window_end = idx;
    for (int i = 0; i < period; ++i) {
        int data_idx = window_start + i;
        if (data_idx >= 0 && data_idx < n) {
            s_prices[tid * period + i] = prices[data_idx];
        } else {
            s_prices[tid * period + i] = 0.0;
        }
    }
    __syncthreads();

    if (idx < period - 1) {
        sma[idx] = 0.0;
        return;
    }

    double sum = 0.0;
    for (int i = 0; i < period; ++i) {
        sum += s_prices[tid * period + i];
    }
    sma[idx] = sum / period;
}

// Optimized RSI kernel using shared memory for small/medium periods
__global__ void rsi_kernel(const double* prices, double* rsi, int n, int period) {
    extern __shared__ double s_prices[];
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    int tid = threadIdx.x;
    if (idx >= n) return;

    // Load data into shared memory (window for this block)
    int window_start = idx - period + 1;
    for (int i = 0; i <= period; ++i) {
        int data_idx = window_start + i - 1;
        if (data_idx >= 0 && data_idx < n) {
            s_prices[tid * (period + 1) + i] = prices[data_idx];
        } else {
            s_prices[tid * (period + 1) + i] = 0.0;
        }
    }
    __syncthreads();

    if (idx < period) {
        rsi[idx] = 50.0;
        return;
    }

    double gain = 0.0, loss = 0.0;
    for (int i = 1; i <= period; ++i) {
        double change = s_prices[tid * (period + 1) + i] - s_prices[tid * (period + 1) + i - 1];
        if (change > 0) gain += change;
        else loss -= change;
    }

    if (gain + loss == 0) {
        rsi[idx] = 50.0;
    } else {
        double rs = gain / (loss == 0 ? 1e-10 : loss);
        rsi[idx] = 100.0 - (100.0 / (1.0 + rs));
    }
}

// GPU kernel for signal generation
__global__ void signal_kernel(
    const double* prices, const double* sma, const double* rsi,
    int* signals, double* stops, double* targets,
    int n, double rsi_oversold, double risk_reward
) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= n) return;
    
    // Check conditions for buy signal
    bool uptrend = prices[idx] > sma[idx];
    bool oversold = rsi[idx] < rsi_oversold;
    
    // Simple FVG detection (gap up/down)
    bool fvg = false;
    if (idx >= 1) {
        fvg = (prices[idx] > prices[idx-1] * 1.01) || (prices[idx] < prices[idx-1] * 0.99);
    }
    
    if (uptrend && oversold && fvg) {
        signals[idx] = 1; // BUY signal
        double entry = prices[idx];
        double stop_loss_pct = 0.005 / risk_reward;
        stops[idx] = entry - (entry * stop_loss_pct);
        targets[idx] = entry + (entry - stops[idx]) * risk_reward;
    } else {
        signals[idx] = 0; // NO signal
        stops[idx] = 0.0;
        targets[idx] = 0.0;
    }
}

// Host wrapper functions
extern "C" {

void gpu_calculate_indicators(
    const double* prices, int n,
    double* sma, double* rsi,
    int sma_period, int rsi_period
) {
    // Allocate device memory
    double *d_prices, *d_sma, *d_rsi;
    CUDA_CHECK(cudaMalloc(&d_prices, n * sizeof(double)));
    CUDA_CHECK(cudaMalloc(&d_sma, n * sizeof(double)));
    CUDA_CHECK(cudaMalloc(&d_rsi, n * sizeof(double)));
    
    // Copy data to GPU
    CUDA_CHECK(cudaMemcpy(d_prices, prices, n * sizeof(double), cudaMemcpyHostToDevice));
    
    // Launch kernels with shared memory
    int block_size = 256;
    int grid_size = (n + block_size - 1) / block_size;
    size_t sma_shmem = block_size * sma_period * sizeof(double);
    size_t rsi_shmem = block_size * (rsi_period + 1) * sizeof(double);
    
    sma_kernel<<<grid_size, block_size, sma_shmem>>>(d_prices, d_sma, n, sma_period);
    cudaDeviceSynchronize();
    cudaError_t err = cudaGetLastError();
    if (err != cudaSuccess) {
        std::cerr << "SMA kernel launch failed: " << cudaGetErrorString(err) << std::endl;
        return;
    }
    
    rsi_kernel<<<grid_size, block_size, rsi_shmem>>>(d_prices, d_rsi, n, rsi_period);
    cudaDeviceSynchronize();
    err = cudaGetLastError();
    if (err != cudaSuccess) {
        std::cerr << "RSI kernel launch failed: " << cudaGetErrorString(err) << std::endl;
        return;
    }
    
    // Copy results back
    CUDA_CHECK(cudaMemcpy(sma, d_sma, n * sizeof(double), cudaMemcpyDeviceToHost));
    CUDA_CHECK(cudaMemcpy(rsi, d_rsi, n * sizeof(double), cudaMemcpyDeviceToHost));
    
    // Cleanup
    cudaFree(d_prices);
    cudaFree(d_sma);
    cudaFree(d_rsi);
}

void gpu_generate_signals(
    const double* prices, const double* sma, const double* rsi,
    int n, double rsi_oversold, double risk_reward,
    int* signals, double* stops, double* targets
) {
    // Allocate device memory
    double *d_prices, *d_sma, *d_rsi;
    int *d_signals;
    double *d_stops, *d_targets;
    
    CUDA_CHECK(cudaMalloc(&d_prices, n * sizeof(double)));
    CUDA_CHECK(cudaMalloc(&d_sma, n * sizeof(double)));
    CUDA_CHECK(cudaMalloc(&d_rsi, n * sizeof(double)));
    CUDA_CHECK(cudaMalloc(&d_signals, n * sizeof(int)));
    CUDA_CHECK(cudaMalloc(&d_stops, n * sizeof(double)));
    CUDA_CHECK(cudaMalloc(&d_targets, n * sizeof(double)));
    
    // Copy data to GPU
    CUDA_CHECK(cudaMemcpy(d_prices, prices, n * sizeof(double), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(d_sma, sma, n * sizeof(double), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(d_rsi, rsi, n * sizeof(double), cudaMemcpyHostToDevice));
    
    // Launch kernel
    int block_size = 256;
    int grid_size = (n + block_size - 1) / block_size;
    
    signal_kernel<<<grid_size, block_size>>>(
        d_prices, d_sma, d_rsi, d_signals, d_stops, d_targets,
        n, rsi_oversold, risk_reward
    );
    
    cudaDeviceSynchronize();
    cudaError_t err = cudaGetLastError();
    if (err != cudaSuccess) {
        std::cerr << "Signal kernel launch failed: " << cudaGetErrorString(err) << std::endl;
        return;
    }
    
    // Copy results back
    CUDA_CHECK(cudaMemcpy(signals, d_signals, n * sizeof(int), cudaMemcpyDeviceToHost));
    CUDA_CHECK(cudaMemcpy(stops, d_stops, n * sizeof(double), cudaMemcpyDeviceToHost));
    CUDA_CHECK(cudaMemcpy(targets, d_targets, n * sizeof(double), cudaMemcpyDeviceToHost));
    
    // Cleanup
    cudaFree(d_prices);
    cudaFree(d_sma);
    cudaFree(d_rsi);
    cudaFree(d_signals);
    cudaFree(d_stops);
    cudaFree(d_targets);
}

} // extern "C" 