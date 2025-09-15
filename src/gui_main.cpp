#include "../include/DataLoader.hpp"
#include "../include/Backtester.hpp"
#include "../include/Strategy.hpp"
#include "../include/FileUtils.hpp"
#ifdef USE_CUDA
#include "../include/GPUStrategy.hpp"
#endif
#include "../include/imgui.h"
#include "../include/imgui_impl_glfw.h"
#include "../include/imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <sstream>
#include <string>
#include <memory>
#include <chrono>

// Factory functions for strategies
Strategy* createGoldenFoundationStrategy(double risk_reward);
#ifdef USE_CUDA
Strategy* createGPUGoldenFoundationStrategy(double risk_reward);
#endif

struct BacktestParams {
    float start_amount;
    float risk_reward;
    bool use_gpu;
    std::string data_path;
    bool operator!=(const BacktestParams& other) const {
        return start_amount != other.start_amount ||
               risk_reward != other.risk_reward ||
               use_gpu != other.use_gpu ||
               data_path != other.data_path;
    }
};

std::string getRiskLabel(float risk_reward) {
    if (risk_reward <= 1.5f) return "SAFE";
    if (risk_reward <= 2.5f) return "MODERATE";
    if (risk_reward <= 3.5f) return "RISKY";
    return "EXTREMELY RISKY";
}

ImVec4 getRiskColor(float risk_reward) {
    if (risk_reward <= 1.5f) return ImVec4(0.0f, 1.0f, 0.0f, 1.0f);      // Green
    if (risk_reward <= 2.5f) return ImVec4(1.0f, 1.0f, 0.0f, 1.0f);      // Yellow
    if (risk_reward <= 3.5f) return ImVec4(1.0f, 0.5f, 0.0f, 1.0f);      // Orange
    return ImVec4(1.0f, 0.0f, 0.0f, 1.0f);                              // Red
}

void run_backtest(const BacktestParams& params, std::string& result_text, bool& running_flag) {
    running_flag = true;
    std::string data_path = params.data_path;
    auto data = DataLoader::loadCSV(data_path);
    if (data.empty()) {
        result_text = "[ERROR] No data loaded! Please ensure SPY_1m.csv exists.\nRun 'python fetch_spy_data.py' to download data.";
        std::cerr << "[ERROR] No data loaded from: " << data_path << std::endl;
        running_flag = false;
        return;
    }
    std::unique_ptr<Strategy> strategy;
#ifdef USE_CUDA
    if (params.use_gpu) {
        strategy.reset(createGPUGoldenFoundationStrategy(params.risk_reward));
    } else {
        strategy.reset(createGoldenFoundationStrategy(params.risk_reward));
    }
#else
    strategy.reset(createGoldenFoundationStrategy(params.risk_reward));
    if (params.use_gpu) {
        result_text = "[ERROR] GPU acceleration not available (CUDA not installed)";
        std::cerr << "[ERROR] GPU acceleration not available (CUDA not installed)" << std::endl;
        running_flag = false;
        return;
    }
#endif
    Backtester backtester(data, strategy.get(), params.start_amount);
    backtester.run();
    double gain = backtester.getFinalEquity() - params.start_amount;
    double pct_gain = (gain / params.start_amount) * 100.0;
    std::ostringstream oss;
    oss << "Yearly P&L:\n";
    for (const auto& kv : backtester.getYearlyPnL()) {
        oss << kv.first << ": $" << std::fixed << std::setprecision(2) << kv.second << "\n";
    }
    oss << "\nTotal gain: $" << std::fixed << std::setprecision(2) << gain
        << " (" << std::fixed << std::setprecision(2) << pct_gain << "%)"
        << "\nFinal equity: $" << std::fixed << std::setprecision(2) << backtester.getFinalEquity()
        << "\nRisk/Reward: " << params.risk_reward << ":1"
        << "\nStrategy: " << (params.use_gpu ? "GPU" : "CPU")
        << "\nStart Amount: $" << std::fixed << std::setprecision(2) << params.start_amount;
    if (backtester.getYearlyPnL().empty() || gain == 0.0) {
        oss << "\n\n[WARNING] No trades were generated. Try adjusting your risk/reward or ensure your data covers enough time for signals.";
    }
    running_flag = false;
    result_text = oss.str();
}

int main() {
    // Setup window
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return 1;
    }
    
    GLFWwindow* window = glfwCreateWindow(1000, 800, "Trading Algorithm Backtester", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return 1;
    }
    
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    // Setup Dear ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    // App state
    float start_amount = 1000.0f;
    float risk_reward = 2.0f;
    bool use_gpu = false;
    bool live_update = false;
    std::string result_text = "Click 'Run Backtest' to start...";
    bool running = false;
    std::string data_path = FileUtils::findDataFile("SPY_1m.csv");
    char data_path_buf[256];
    strncpy_s(data_path_buf, sizeof(data_path_buf), data_path.c_str(), _TRUNCATE);

    BacktestParams last_params = {start_amount, risk_reward, use_gpu, data_path};
    double last_update_time = 0.0;

    std::cout << "GUI window created successfully!" << std::endl;
    std::cout << "Window should be visible now." << std::endl;

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Trading Algorithm Backtester", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "=== TRADING ALGORITHM BACKTESTER ===");
        ImGui::Separator();
        
        // Mode indicator
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "Mode: %s", use_gpu ? "GPU (Fast)" : "CPU (Compatible)");
        
        ImGui::Spacing();
        
        // Starting Amount Control
        ImGui::Text("Starting Capital:");
        ImGui::InputFloat("Amount ($)", &start_amount, 100.0f, 1000.0f, "%.0f");
        ImGui::SameLine();
        ImGui::Text("Current: $%.0f", start_amount);
        
        ImGui::Spacing();
        
        // Risk/Reward Control with Labels
        ImGui::Text("Risk/Reward Ratio:");
        ImGui::SliderFloat("##RiskReward", &risk_reward, 0.067f, 5.0f, "%.2f:1"); // 1:15 = 0.067, 5:1 = 5.0
        
        // Risk level indicator
        std::string risk_label = getRiskLabel(risk_reward);
        ImVec4 risk_color = getRiskColor(risk_reward);
        ImGui::SameLine();
        ImGui::TextColored(risk_color, "%s", risk_label.c_str());
        
        // Risk explanation
        if (risk_reward <= 1.5f) {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Conservative: Tight stops, smaller targets");
        } else if (risk_reward <= 2.5f) {
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Moderate: Balanced risk and reward");
        } else if (risk_reward <= 3.5f) {
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Aggressive: Larger stops, bigger targets");
        } else {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Very Aggressive: High risk, high reward");
        }
        
        ImGui::Spacing();
        
        // GPU/CPU Toggle
        ImGui::Checkbox("Use GPU Acceleration", &use_gpu);
        if (use_gpu) {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "(Faster)");
        } else {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "(Compatible)");
        }
        
        ImGui::Spacing();
        
        // Live Update Toggle
        ImGui::Checkbox("Live Update (60 FPS)", &live_update);
        if (live_update) {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 1.0f, 1.0f), "(Real-time)");
        }
        
        ImGui::Spacing();
        
        // Data File Path
        ImGui::Text("Data File:");
        ImGui::InputText("##DataPath", data_path_buf, sizeof(data_path_buf));
        ImGui::SameLine();
        if (ImGui::Button("Reload")) {
            data_path = FileUtils::findDataFile("SPY_1m.csv");
            strncpy_s(data_path_buf, sizeof(data_path_buf), data_path.c_str(), _TRUNCATE);
        }
        
        ImGui::Spacing();
        ImGui::Separator();
        
        // Control Buttons
        if (ImGui::Button("Run Backtest", ImVec2(120, 30))) {
            last_params = {start_amount, risk_reward, use_gpu, std::string(data_path_buf)};
            running = true;
            result_text = "Running backtest...";
            run_backtest(last_params, result_text, running);
            last_update_time = ImGui::GetTime();
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Reset", ImVec2(80, 30))) {
            start_amount = 1000.0f;
            risk_reward = 2.0f;
            use_gpu = false;
            live_update = false;
            result_text = "Settings reset. Click 'Run Backtest' to start...";
        }
        
        ImGui::Spacing();
        ImGui::Separator();
        
        // Live Update Logic
        if (live_update) {
            double now = ImGui::GetTime();
            BacktestParams current_params = {start_amount, risk_reward, use_gpu, std::string(data_path_buf)};
            if ((now - last_update_time > 1.0/60.0) && !running) {
                if (current_params != last_params) {
                    running = true;
                    result_text = "Running...";
                    run_backtest(current_params, result_text, running);
                    last_params = current_params;
                    last_update_time = now;
                } else if (live_update) {
                    // Even if params didn't change, rerun at 60 FPS if live_update is on
                    running = true;
                    result_text = "Running...";
                    run_backtest(current_params, result_text, running);
                    last_update_time = now;
                }
            }
        }
        
        // Results Section
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "=== RESULTS ===");
        if (running) {
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Running backtest...");
        }
        ImGui::TextWrapped("%s", result_text.c_str());
        
        ImGui::End();

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
} 