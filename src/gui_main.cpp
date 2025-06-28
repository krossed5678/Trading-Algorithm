#include "../include/DataLoader.hpp"
#include "../include/Backtester.hpp"
#include "../include/Strategy.hpp"
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <sstream>
#include <string>

// Factory function for strategy
Strategy* createGoldenFoundationStrategy(double risk);

// Helper to run the backtest and collect results
void run_backtest(double start_amount, double risk, std::string& result_text) {
    auto data = DataLoader::loadCSV("data/SPY_1m.csv");
    if (data.empty()) {
        result_text = "No data loaded!";
        return;
    }
    Strategy* strategy = createGoldenFoundationStrategy(risk);
    Backtester backtester(data, strategy, start_amount);
    backtester.run();
    std::ostringstream oss;
    oss << "Yearly P&L:\n";
    for (const auto& kv : backtester.getYearlyPnL()) {
        oss << kv.first << ": " << kv.second << "\n";
    }
    oss << "\nTotal gain: " << (backtester.getFinalEquity() - start_amount)
        << "\nFinal equity: " << backtester.getFinalEquity();
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
    double start_amount = 1000.0;
    double risk = 1.0;
    std::string result_text = "";

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Backtest Controls");
        ImGui::InputDouble("Start Amount", &start_amount);
        ImGui::SliderDouble("Risk/Volatility", &risk, 0.1, 5.0, "%.2f");
        if (ImGui::Button("Run Backtest")) {
            run_backtest(start_amount, risk, result_text);
        }
        ImGui::Separator();
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