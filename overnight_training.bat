@echo off
echo ========================================
echo    OVERNIGHT ML TRAINING STARTED
echo ========================================
echo.
echo Starting time: %date% %time%
echo.

cd /d "C:\Users\koanr\Desktop\C++\Trading-Algorithm\build\Release"

echo [INFO] Starting overnight genetic algorithm training...
echo [INFO] Population: 200 strategies
echo [INFO] Generations: 200 cycles
echo [INFO] Estimated time: ~6-8 hours
echo [INFO] Results will be saved to overnight_results/
echo.

if not exist "overnight_results" mkdir overnight_results

echo [INFO] Redirecting output to overnight_results/training.log...
echo.

genetic_evolution.exe > overnight_results\training.log 2>&1

move /Y best_strategy.pine overnight_results\best_strategy.pine >nul 2>&1

echo.
echo ========================================
echo    OVERNIGHT TRAINING COMPLETED
echo ========================================
echo.
echo End time: %date% %time%
echo.
echo [SUCCESS] Training completed! Check overnight_results/ for results.
echo [INFO] Best strategy: overnight_results/best_strategy.pine
echo [INFO] Full log: overnight_results/training.log
echo.

pause 