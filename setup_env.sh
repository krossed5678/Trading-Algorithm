#!/bin/bash

echo "Setting up environment configuration..."

if [ ! -f .env ]; then
    if [ -f env.example ]; then
        cp env.example .env
        echo ".env created from env.example"
        echo ""
        echo "IMPORTANT: Please edit .env and add your actual API keys!"
        echo ""
    else
        echo "ERROR: env.example not found!"
        echo "Please create env.example first."
        exit 1
    fi
else
    echo ".env already exists"
fi

echo "Environment setup complete!" 