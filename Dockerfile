FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && \
    apt-get install -y \
    build-essential \
    libboost-all-dev \
    libsfml-dev \
    xvfb \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy ONLY source files, NOT pre-compiled object files
COPY src /app/src
COPY webserver /app/webserver
COPY assets /app/assets

# Compile everything from source in one command
RUN g++ -std=c++17 \
    $(find src -name "*.cpp" ! -name "main.cpp") \
    webserver/networked_main.cpp \
    -o networked_client \
    -Isrc \
    -lboost_system -lboost_thread -lpthread \
    -lsfml-graphics -lsfml-window -lsfml-system -lsfml-audio

CMD ["xvfb-run", "-a", "./networked_client"]