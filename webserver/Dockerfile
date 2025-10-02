#Base Image
FROM ubuntu:22.04

# Set environment variables for non-interactive install
ENV DEBIAN_FRONTEND=noninteractive

#Install Build Tools and Boost Libraries
RUN apt-get update && \ 
    apt-get install -y \
    build-essential \
    libboost-all-dev \
    git \
    cmake \
    && rm -rf /var/lib/apt/lists/*

# -y = yes which does not require user confirmation
#Build essential provides GNU Compiler Collection (g++) and other build tools
# && equals execute the next command only if the previous one was successful

#Setup Working Directory and Copy Source File
WORKDIR /app
COPY websocket_client.cpp /app/

#Compile the C++ Client
# Links required Boost libraries and the thread library
RUN g++ -std=c++17 websocket_client.cpp -o websocket_client \
    -lboost_system \
    -lboost_thread \
    -pthread

#Define the Default Execution Command
# This will run the client when the container starts
CMD ["./websocket_client"]