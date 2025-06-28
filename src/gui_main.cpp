#include "../include/DataLoader.hpp"
#include "../include/Backtester.hpp"
#include "../include/Strategy.hpp"
#ifdef USE_CUDA
#include "../include/GPUStrategy.hpp"
#endif
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <sstream>
#include <string>

// Factory functions for strategies
Strategy* createGoldenFoundationStrategy(double risk_reward);
#ifdef USE_CUDA
Strategy* createGPUGoldenFoundationStrategy(double risk_reward);
#endif

// Helper to run the backtest and collect results
void run_backtest(float start_amount, float risk_reward, bool use_gpu, std::string& result_text) {
    auto data = DataLoader::loadCSV("data/SPY_1m.csv");
    if (data.empty()) {
        result_text = "No data loaded!";
        return;
    }
    
    Strategy* strategy;
#ifdef USE_CUDA
    if (use_gpu) {
        strategy = createGPUGoldenFoundationStrategy(risk_reward);
    } else {
        strategy = createGoldenFoundationStrategy(risk_reward);
    }
#else
    strategy = createGoldenFoundationStrategy(risk_reward);
    if (use_gpu) {
        result_text = "GPU acceleration not available (CUDA not installed)";
        delete strategy;
        return;
    }
#endif
    
    Backtester backtester(data, strategy, start_amount);
    backtester.run();
    std::ostringstream oss;
    oss << "Yearly P&L:\n";
    for (const auto& kv : backtester.getYearlyPnL()) {
        oss << kv.first << ": " << kv.second << "\n";
    }
    oss << "\nTotal gain: " << (backtester.getFinalEquity() - start_amount)
        << "\nFinal equity: " << backtester.getFinalEquity()
        << "\nRisk/Reward: " << risk_reward << ":1"
        << "\nStrategy: " << (use_gpu ? "GPU" : "CPU")
        << "\nStart Amount: $" << start_amount;
    result_text = oss.str();
    delete strategy;
}

int main() {
    // Setup window
    if (!glfwInit()) return 1;
    GLFWwindow* window = glfwCreateWindow(800, 600, "Trading Backtester", NULL, NULL);
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
    float risk_reward = 3.0f;
    std::string result_text = "";
    
    // Mode selection state
    bool mode_selected = false;
    bool use_gpu = false;
#ifdef USE_CUDA
    bool cuda_available = true;
#else
    bool cuda_available = false;
#endif

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        if (!mode_selected) {
            // Startup mode selection dialog
            ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
            ImGui::SetNextWindowSize(ImVec2(400, 200), ImGuiCond_Always);
            
            ImGui::Begin("Select Processing Mode", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
            
            ImGui::Text("Choose your processing mode:");
            ImGui::Spacing();
            
            // CPU option
            if (ImGui::Button("CPU Mode (Compatible)", ImVec2(180, 40))) {
                use_gpu = false;
                mode_selected = true;
                glfwSetWindowTitle(window, "Trading Backtester - CPU Mode");
            }
            ImGui::SameLine();
            ImGui::TextWrapped("Works on all systems\nSlower for large datasets");
            
            ImGui::Spacing();
            
            // GPU option (only if CUDA available)
            if (cuda_available) {
                if (ImGui::Button("GPU Mode (Fast)", ImVec2(180, 40))) {
                    use_gpu = true;
                    mode_selected = true;
                    glfwSetWindowTitle(window, "Trading Backtester - GPU Mode");
                }
                ImGui::SameLine();
                ImGui::TextWrapped("Requires NVIDIA GPU\nMuch faster computation");
            } else {
                ImGui::BeginDisabled();
                ImGui::Button("GPU Mode (Unavailable)", ImVec2(180, 40));
                ImGui::EndDisabled();
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "CUDA not installed\nInstall NVIDIA CUDA Toolkit");
            }
            
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::TextWrapped("You can change this later in the main interface.");
            
            ImGui::End();
        } else {
            // Main backtest interface
            ImGui::Begin("Backtest Controls");
            
            // Mode indicator
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), 
                "Mode: %s", use_gpu ? "GPU (Fast)" : "CPU (Compatible)");
            
            // Start amount input
            ImGui::InputFloat("Start Amount ($)", &start_amount, 100.0f, 500.0f, "%.0f");
            
            // Risk/Reward slider with labels
            ImGui::Text("Risk/Reward Ratio:");
            ImGui::SliderFloat("##RiskReward", &risk_reward, 0.2f, 5.0f, "%.1f:1");
            
            // Risk level indicator
            ImGui::SameLine();
            if (risk_reward >= 4.0f) {
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "VERY RISKY");
            } else if (risk_reward >= 2.5f) {
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "RISKY");
            } else if (risk_reward >= 1.5f) {
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "MODERATE");
            } else {
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "SAFE");
            }
            
            // Mode switching (only if CUDA available)
            if (cuda_available) {
                ImGui::Spacing();
                if (ImGui::Button(use_gpu ? "Switch to CPU Mode" : "Switch to GPU Mode")) {
                    use_gpu = !use_gpu;
                    glfwSetWindowTitle(window, use_gpu ? "Trading Backtester - GPU Mode" : "Trading Backtester - CPU Mode");
                }
            }
            
            // Explanation
            ImGui::TextWrapped("Higher ratios = more aggressive (larger stops, bigger targets)");
            ImGui::TextWrapped("Lower ratios = more conservative (tighter stops, smaller targets)");
            if (use_gpu) {
                ImGui::TextWrapped("GPU acceleration provides faster computation for large datasets");
            } else {
                ImGui::TextWrapped("CPU mode works on all systems but may be slower");
            }
            
            ImGui::Separator();
            
            // Run backtest on EVERY frame (continuous updates)
            run_backtest(start_amount, risk_reward, use_gpu, result_text);
            
            // Results
            ImGui::Text("Results:");
            ImGui::TextWrapped("%s", result_text.c_str());
            
            ImGui::End();
        }

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