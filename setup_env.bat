@echo off
echo Setting up environment configuration...

if not exist .env (
    if exist env.example (
        copy env.example .env
        echo .env created from env.example
        echo.
        echo IMPORTANT: Please edit .env and add your actual API keys!
        echo.
    ) else (
        echo ERROR: env.example not found!
        echo Please create env.example first.
        pause
        exit /b 1
    )
) else (
    echo .env already exists
)

echo Environment setup complete!
pause 