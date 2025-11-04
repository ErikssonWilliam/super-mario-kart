#include <boost/asio/io_context.hpp>
#include <iostream>
#include "../src/game.h"
#include "../src/entities/enums.h"  
#include "websocket_client.cpp"

int main() {
    try {
        asio::io_context ioc;

        const std::string HOST = "127.0.0.1";
        const std::string PORT = "8080";
        const int BASE_WIDTH = static_cast<int>(BASIC_WIDTH / 2.0f);
        const int BASE_HEIGHT = static_cast<int>(BASIC_HEIGHT / 2.0f);
        const int FRAMERATE = 60;

        NetworkedGameClient gameClient(ioc, HOST, PORT, BASE_WIDTH, BASE_HEIGHT, FRAMERATE);

        std::cout << "Client connected and waiting for START_GAME command from server...\n";

        gameClient.startNetwork();

        gameClient.run();
        
        std::cout << "Game finished normally" << std::endl;
        
    } catch (std::exception& e) {
        std::cerr << "Fatal Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}