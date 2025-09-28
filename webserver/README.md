# WebSocket Action-Based Communication System

This project implements a WebSocket communication system with a Python server and C++ client that exchange binary-encoded action messages.

## Overview

- **Server**: Python WebSocket server that accepts connections and sends welcome messages
- **Client**: C++ WebSocket client that connects to the server and receives action messages
- **Protocol**: Custom binary protocol for action-based communication

## Prerequisites

### Python Server Requirements
- Python 3.7 or higher
- `websockets` library
- `asyncio` (built-in)
- `struct` (built-in)

### C++ Client Requirements
- C++17 compatible compiler (GCC 7+, Clang 5+, or MSVC 2017+)
- Boost.Beast library (part of Boost 1.70+)
- Boost.Asio library
- pthread library (Linux/macOS)

## Installation

### Python Dependencies

```bash
# Install websockets library
pip install websockets

### C++ Dependencies

#### Ubuntu/Debian
```bash
sudo apt update
sudo apt install build-essential libboost-all-dev
```

#### macOS (using Homebrew)
```bash
brew install boost
```

#### Windows (using vcpkg)
```bash
vcpkg install boost-beast boost-asio
```

## Running the System

### Step 1: Start the Python Server

```bash
# Navigate to the directory containing websocket_server.py
cd /path/to/your/project

# Run the server
python3 websocket_server.py
```

### Step 2: Compile the C++ Client

```bash
# Compile the client (adjust paths as needed)
g++ -std=c++17 websocket.cpp -lboost_system -pthread -o websocket_client

# If Boost is installed in a custom location, specify include path:
g++ -std=c++17 -I/usr/local/include websocket.cpp -L/usr/local/lib -lboost_system -pthread -o websocket_client
```

### Step 3: Run the C++ Client

```bash
# Run the compiled client
./websocket_client
```
