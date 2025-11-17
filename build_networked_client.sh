#!/bin/bash

# Build script for networked client
# This compiles all game source files (except main.cpp) and links with networked_main.cpp

set -e  # Exit on error

# Directories
SRC_DIR="src"
BIN_DIR="bin"
WEBSERVER_DIR="webserver"

# Compiler flags
CXX="g++"
CXXFLAGS="-std=c++17 -Wall -W -O2 -g"
CPPFLAGS="-Isrc"
LDFLAGS="-pthread"
LDLIBS="-lboost_system -lboost_thread -lpthread -lsfml-graphics -lsfml-window -lsfml-system -lsfml-audio"

# Output executable
OUTPUT="networked_client"

# Create bin directory structure
echo "Creating bin directory structure..."
mkdir -p "$BIN_DIR"/{states,input,map,entities,gui,ai,audio}

# Find all source files except main.cpp
echo "Finding source files..."
SOURCES=$(find "$SRC_DIR" -name "*.cpp" -not -name "main.cpp")

# Compile all source files to object files
echo "Compiling source files..."
for src in $SOURCES; do
    # Get relative path from src directory
    rel_path="${src#$SRC_DIR/}"
    obj_path="$BIN_DIR/$rel_path"
    obj_path="${obj_path%.cpp}.o"
    
    # Create directory for object file if needed
    obj_dir=$(dirname "$obj_path")
    mkdir -p "$obj_dir"
    
    echo "  Compiling $src -> $obj_path"
    $CXX -c $CXXFLAGS $CPPFLAGS "$src" -o "$obj_path"
done

# Collect all object files
echo "Collecting object files..."
OBJECT_FILES=$(find "$BIN_DIR" -name "*.o")

# Compile networked_main.cpp
echo "Compiling networked_main.cpp..."
$CXX -c $CXXFLAGS $CPPFLAGS "$WEBSERVER_DIR/networked_main.cpp" -o "$BIN_DIR/networked_main.o"

# Link everything together
echo "Linking $OUTPUT..."
$CXX $LDFLAGS $OBJECT_FILES "$BIN_DIR/networked_main.o" $LDLIBS -o "$OUTPUT"

echo "Build complete! Executable: $OUTPUT"

