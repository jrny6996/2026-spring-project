#!/bin/bash

echo "Updating package list..."
sudo apt update

echo "📦 Installing required development tools and libraries..."
sudo apt install -y \
    build-essential \
    cmake \
    g++ \
    git \
    libboost-system-dev \
    libboost-thread-dev \
    libboost-regex-dev \
    libssl-dev \
    zlib1g-dev \
    inotify-tools

    # libpqxx-dev
    # libboost-all-dev


echo "Installing Tailwind CSS" && npm install tailwindcss @tailwindcss/cli
