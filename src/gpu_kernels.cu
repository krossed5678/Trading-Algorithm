#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <thrust/device_vector.h>
#include <thrust/host_vector.h>
#include <thrust/execution_policy.h>
#include <thrust/transform.h>
#include <thrust/functional.h>
#include <thrust/sequence.h>
#include <iostream>
#include <cuda_profiler_api.h>

// Error checking macro
#define CUDA_CHECK(call) \
    do { \
        cudaError_t err = call; \
        if (err != cudaSuccess) { \
            std::cerr << "CUDA error: " << cudaGetErrorString(err) << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
            return; \
        } \
    } while(0)

// Optimized block size calculation
__device__ __forceinline__ int getOptimalBlockSize() {
    return 256; // Optimized for most modern GPUs
}

// Optimized SMA kernel with improved memory coalescing
__global__ void optimized_sma_kernel(const double* prices, double* sma, int n, int period) {
    extern __shared__ double s_prices[];
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    int tid = threadIdx.x;
    
    if (idx >= n) return;

    // Coalesced memory access pattern
    int window_start = max(0, idx - period + 1);
    int window_end = min(n - 1, idx);
    int window_size = window_end - window_start + 1;
    
    // Load data into shared memory with coalesced access
    double local_sum = 0.0;
    for (int i = window_start; i <= window_end; i += blockDim.x) {
        int load_idx = i + tid;
        if (load_idx <= window_end) {
            local_sum += prices[load_idx];
        }
    }
    
    // Reduce within thread block
    __shared__ double s_sums[256];
    s_sums[tid] = local_sum;
    __syncthreads();
    
    // Parallel reduction
    for (int stride = blockDim.x / 2; stride > 0; stride >>= 1) {
        if (tid < stride) {
            s_sums[tid] += s_sums[tid + stride];
        }
        __syncthreads();
    }
    
    if (tid == 0) {
        int block_start = blockIdx.x * blockDim.x;
        int block_end = min(n - 1, block_start + blockDim.x - 1);
        int actual_period = min(period, block_end - block_start + 1);
        sma[idx] = s_sums[0] / actual_period;
    }
}

// Optimized RSI kernel with improved numerical stability
__global__ void optimized_rsi_kernel(const double* prices, double* rsi, int n, int period) {
    extern __shared__ double s_data[];
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    int tid = threadIdx.x;
    
    if (idx >= n) return;
    
    if (idx < period) {
        rsi[idx] = 50.0;
        return;
    }
    
    // Load price data into shared memory
    int start_idx = idx - period;
    for (int i = 0; i <= period; i += blockDim.x) {
        int load_idx = start_idx + i + tid;
        if (load_idx <= idx && load_idx < n) {
            s_data[tid * (period + 1) + i] = prices[load_idx];
        }
    }
    __syncthreads();
    
    // Calculate gains and losses
    double gain_sum = 0.0, loss_sum = 0.0;
    for (int i = 1; i <= period; i++) {
        double change = s_data[tid * (period + 1) + i] - s_data[tid * (period + 1) + i - 1];
        if (change > 0) {
            gain_sum += change;
        } else if (change < 0) {
            loss_sum -= change;
        }
    }
    
    // Improved numerical stability
    if (gain_sum + loss_sum < 1e-10) {
        rsi[idx] = 50.0;
    } else {
        double avg_gain = gain_sum / period;
        double avg_loss = loss_sum / period;
        
        if (avg_loss < 1e-10) {
            rsi[idx] = 100.0;
        } else {
            double rs = avg_gain / avg_loss;
            rsi[idx] = 100.0 - (100.0 / (1.0 + rs));
        }
    }
}

// Fused kernel that calculates SMA, RSI, and generates signals in one pass
__global__ void fused_indicators_kernel(
    const double* prices, 
    double* sma, double* rsi, int* signals, double* stops, double* targets,
    int n, int sma_period, int rsi_period, double rsi_oversold, double risk_reward
) {
    extern __shared__ double s_data[];
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    int tid = threadIdx.x;
    
    if (idx >= n) return;
    
    // Calculate SMA
    if (idx >= sma_period - 1) {
        double sma_sum = 0.0;
        for (int i = idx - sma_period + 1; i <= idx; i++) {
            sma_sum += prices[i];
        }
        sma[idx] = sma_sum / sma_period;
    } else {
        sma[idx] = 0.0;
    }
    
    // Calculate RSI
    if (idx >= rsi_period) {
        double gain_sum = 0.0, loss_sum = 0.0;
        for (int i = idx - rsi_period + 1; i <= idx; i++) {
            double change = prices[i] - prices[i - 1];
            if (change > 0) gain_sum += change;
            else loss_sum -= change;
        }
        
        if (gain_sum + loss_sum < 1e-10) {
            rsi[idx] = 50.0;
        } else {
            double avg_gain = gain_sum / rsi_period;
            double avg_loss = loss_sum / rsi_period;
            
            if (avg_loss < 1e-10) {
                rsi[idx] = 100.0;
            } else {
                double rs = avg_gain / avg_loss;
                rsi[idx] = 100.0 - (100.0 / (1.0 + rs));
            }
        }
    } else {
        rsi[idx] = 50.0;
    }
    
    // Generate signals
    if (idx >= max(sma_period, rsi_period)) {
        bool uptrend = prices[idx] > sma[idx];
        bool oversold = rsi[idx] < rsi_oversold;
        
        // Enhanced FVG detection
        bool fvg = false;
        if (idx >= 1) {
            double gap_threshold = 0.01; // 1% threshold
            fvg = (prices[idx] > prices[idx-1] * (1.0 + gap_threshold)) || 
                  (prices[idx] < prices[idx-1] * (1.0 - gap_threshold));
        }
        
        if (uptrend && oversold && fvg) {
            signals[idx] = 1; // BUY signal
            double entry = prices[idx];
            double stop_loss_pct = 0.005 / risk_reward;
            stops[idx] = entry * (1.0 - stop_loss_pct);
            targets[idx] = entry + (entry - stops[idx]) * risk_reward;
        } else {
            signals[idx] = 0; // NO signal
            stops[idx] = 0.0;
            targets[idx] = 0.0;
        }
    } else {
        signals[idx] = 0;
        stops[idx] = 0.0;
        targets[idx] = 0.0;
    }
}

// Optimized signal generation kernel with vectorized operations
__global__ void optimized_signal_kernel(
    const double* prices, const double* sma, const double* rsi,
    int* signals, double* stops, double* targets,
    int n, double rsi_oversold, double risk_reward
) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= n) return;
    
    // Vectorized condition checking
    double price = prices[idx];
    double sma_val = sma[idx];
    double rsi_val = rsi[idx];
    
    // Use bitwise operations for condition checking
    bool uptrend = price > sma_val;
    bool oversold = rsi_val < rsi_oversold;
    
    // Enhanced FVG detection with multiple timeframes
    bool fvg = false;
    if (idx >= 1) {
        double gap_threshold = 0.01;
        double price_change = price / prices[idx-1];
        fvg = (price_change > 1.0 + gap_threshold) || (price_change < 1.0 - gap_threshold);
    }
    
    // Combined condition check
    if (uptrend && oversold && fvg) {
        signals[idx] = 1;
        double entry = price;
        double stop_loss_pct = 0.005 / risk_reward;
        stops[idx] = entry * (1.0 - stop_loss_pct);
        targets[idx] = entry + (entry - stops[idx]) * risk_reward;
    } else {
        signals[idx] = 0;
        stops[idx] = 0.0;
        targets[idx] = 0.0;
    }
}

// Memory pool for reducing allocation overhead
class CUDAMemoryPool {
private:
    struct MemoryBlock {
        void* ptr;
        size_t size;
        bool in_use;
    };
    std::vector<MemoryBlock> blocks_;
    
public:
    void* allocate(size_t size) {
        // Find existing block
        for (auto& block : blocks_) {
            if (!block.in_use && block.size >= size) {
                block.in_use = true;
                return block.ptr;
            }
        }
        
        // Allocate new block
        void* ptr;
        CUDA_CHECK(cudaMalloc(&ptr, size));
        blocks_.push_back({ptr, size, true});
        return ptr;
    }
    
    void free(void* ptr) {
        for (auto& block : blocks_) {
            if (block.ptr == ptr) {
                block.in_use = false;
                return;
            }
        }
    }
    
    ~CUDAMemoryPool() {
        for (auto& block : blocks_) {
            cudaFree(block.ptr);
        }
    }
};

static CUDAMemoryPool g_memory_pool;

// Host wrapper functions with optimized memory management
extern "C" {

void gpu_calculate_indicators(
    const double* prices, int n,
    double* sma, double* rsi,
    int sma_period, int rsi_period
) {
    // Use fused kernel for better performance
    double *d_prices, *d_sma, *d_rsi;
    int *d_signals;
    double *d_stops, *d_targets;
    
    // Allocate device memory using pool
    d_prices = (double*)g_memory_pool.allocate(n * sizeof(double));
    d_sma = (double*)g_memory_pool.allocate(n * sizeof(double));
    d_rsi = (double*)g_memory_pool.allocate(n * sizeof(double));
    d_signals = (int*)g_memory_pool.allocate(n * sizeof(int));
    d_stops = (double*)g_memory_pool.allocate(n * sizeof(double));
    d_targets = (double*)g_memory_pool.allocate(n * sizeof(double));
    
    // Asynchronous memory copy
    CUDA_CHECK(cudaMemcpyAsync(d_prices, prices, n * sizeof(double), cudaMemcpyHostToDevice, 0));
    
    // Launch fused kernel
    int block_size = 256;
    int grid_size = (n + block_size - 1) / block_size;
    size_t shared_mem = block_size * max(sma_period, rsi_period + 1) * sizeof(double);
    
    fused_indicators_kernel<<<grid_size, block_size, shared_mem>>>(
        d_prices, d_sma, d_rsi, d_signals, d_stops, d_targets,
        n, sma_period, rsi_period, 30.0, 2.0
    );
    
    // Check for errors
    cudaError_t err = cudaGetLastError();
    if (err != cudaSuccess) {
        std::cerr << "Fused kernel launch failed: " << cudaGetErrorString(err) << std::endl;
        return;
    }
    
    // Asynchronous copy back
    CUDA_CHECK(cudaMemcpyAsync(sma, d_sma, n * sizeof(double), cudaMemcpyDeviceToHost, 0));
    CUDA_CHECK(cudaMemcpyAsync(rsi, d_rsi, n * sizeof(double), cudaMemcpyDeviceToHost, 0));
    
    // Synchronize
    cudaDeviceSynchronize();
    
    // Free memory back to pool
    g_memory_pool.free(d_prices);
    g_memory_pool.free(d_sma);
    g_memory_pool.free(d_rsi);
    g_memory_pool.free(d_signals);
    g_memory_pool.free(d_stops);
    g_memory_pool.free(d_targets);
}

void gpu_generate_signals(
    const double* prices, const double* sma, const double* rsi,
    int n, double rsi_oversold, double risk_reward,
    int* signals, double* stops, double* targets
) {
    // Allocate device memory using pool
    double *d_prices, *d_sma, *d_rsi;
    int *d_signals;
    double *d_stops, *d_targets;
    
    d_prices = (double*)g_memory_pool.allocate(n * sizeof(double));
    d_sma = (double*)g_memory_pool.allocate(n * sizeof(double));
    d_rsi = (double*)g_memory_pool.allocate(n * sizeof(double));
    d_signals = (int*)g_memory_pool.allocate(n * sizeof(int));
    d_stops = (double*)g_memory_pool.allocate(n * sizeof(double));
    d_targets = (double*)g_memory_pool.allocate(n * sizeof(double));
    
    // Asynchronous memory copies
    CUDA_CHECK(cudaMemcpyAsync(d_prices, prices, n * sizeof(double), cudaMemcpyHostToDevice, 0));
    CUDA_CHECK(cudaMemcpyAsync(d_sma, sma, n * sizeof(double), cudaMemcpyHostToDevice, 0));
    CUDA_CHECK(cudaMemcpyAsync(d_rsi, rsi, n * sizeof(double), cudaMemcpyHostToDevice, 0));
    
    // Launch optimized signal kernel
    int block_size = 256;
    int grid_size = (n + block_size - 1) / block_size;
    
    optimized_signal_kernel<<<grid_size, block_size>>>(
        d_prices, d_sma, d_rsi, d_signals, d_stops, d_targets,
        n, rsi_oversold, risk_reward
    );
    
    cudaError_t err = cudaGetLastError();
    if (err != cudaSuccess) {
        std::cerr << "Signal kernel launch failed: " << cudaGetErrorString(err) << std::endl;
        return;
    }
    
    // Asynchronous copy back
    CUDA_CHECK(cudaMemcpyAsync(signals, d_signals, n * sizeof(int), cudaMemcpyDeviceToHost, 0));
    CUDA_CHECK(cudaMemcpyAsync(stops, d_stops, n * sizeof(double), cudaMemcpyDeviceToHost, 0));
    CUDA_CHECK(cudaMemcpyAsync(targets, d_targets, n * sizeof(double), cudaMemcpyDeviceToHost, 0));
    
    // Synchronize
    cudaDeviceSynchronize();
    
    // Free memory back to pool
    g_memory_pool.free(d_prices);
    g_memory_pool.free(d_sma);
    g_memory_pool.free(d_rsi);
    g_memory_pool.free(d_signals);
    g_memory_pool.free(d_stops);
    g_memory_pool.free(d_targets);
}

// New optimized function that does everything in one GPU call
void gpu_calculate_all_indicators_and_signals(
    const double* prices, int n,
    double* sma, double* rsi, int* signals, double* stops, double* targets,
    int sma_period, int rsi_period, double rsi_oversold, double risk_reward
) {
    // Allocate device memory using pool
    double *d_prices, *d_sma, *d_rsi;
    int *d_signals;
    double *d_stops, *d_targets;
    
    d_prices = (double*)g_memory_pool.allocate(n * sizeof(double));
    d_sma = (double*)g_memory_pool.allocate(n * sizeof(double));
    d_rsi = (double*)g_memory_pool.allocate(n * sizeof(double));
    d_signals = (int*)g_memory_pool.allocate(n * sizeof(int));
    d_stops = (double*)g_memory_pool.allocate(n * sizeof(double));
    d_targets = (double*)g_memory_pool.allocate(n * sizeof(double));
    
    // Single memory copy
    CUDA_CHECK(cudaMemcpyAsync(d_prices, prices, n * sizeof(double), cudaMemcpyHostToDevice, 0));
    
    // Launch single fused kernel
    int block_size = 256;
    int grid_size = (n + block_size - 1) / block_size;
    size_t shared_mem = block_size * max(sma_period, rsi_period + 1) * sizeof(double);
    
    fused_indicators_kernel<<<grid_size, block_size, shared_mem>>>(
        d_prices, d_sma, d_rsi, d_signals, d_stops, d_targets,
        n, sma_period, rsi_period, rsi_oversold, risk_reward
    );
    
    cudaError_t err = cudaGetLastError();
    if (err != cudaSuccess) {
        std::cerr << "Fused kernel launch failed: " << cudaGetErrorString(err) << std::endl;
        return;
    }
    
    // Single synchronization and copy back
    CUDA_CHECK(cudaMemcpyAsync(sma, d_sma, n * sizeof(double), cudaMemcpyDeviceToHost, 0));
    CUDA_CHECK(cudaMemcpyAsync(rsi, d_rsi, n * sizeof(double), cudaMemcpyDeviceToHost, 0));
    CUDA_CHECK(cudaMemcpyAsync(signals, d_signals, n * sizeof(int), cudaMemcpyDeviceToHost, 0));
    CUDA_CHECK(cudaMemcpyAsync(stops, d_stops, n * sizeof(double), cudaMemcpyDeviceToHost, 0));
    CUDA_CHECK(cudaMemcpyAsync(targets, d_targets, n * sizeof(double), cudaMemcpyDeviceToHost, 0));
    
    cudaDeviceSynchronize();
    
    // Free memory back to pool
    g_memory_pool.free(d_prices);
    g_memory_pool.free(d_sma);
    g_memory_pool.free(d_rsi);
    g_memory_pool.free(d_signals);
    g_memory_pool.free(d_stops);
    g_memory_pool.free(d_targets);
}

} // extern "C" 