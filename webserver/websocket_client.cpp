#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/io_context.hpp>
#include <iostream>
#include <thread>
#include <vector>
#include <cstdint>
#include <string>
#include <fstream>
#include <chrono>
#include "../src/game.h"
#include "../src/map/map.h"
#include "../src/map/enums.h"


namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace asio = boost::asio;
using tcp = asio::ip::tcp;

// Action codes matching the Python server
enum class ActionCode : uint8_t {
    WELCOME = 0x01,
    ECHO = 0x02,
    PING = 0x03,
    MOVE_UP = 0x04,
    MOVE_DOWN = 0x05,
    MOVE_LEFT = 0x06,
    MOVE_RIGHT = 0x07,
    ATTACK = 0x08,
    DEFEND = 0x09,
    STATUS_UPDATE = 0x0A,
    SEND_OBSERVATION = 0x0B,
    ERROR = 0xFF,
    START_GAME = 0x0C,
};

class MessageHandler {
public:
    static std::vector<uint8_t> encode_action(ActionCode action_code, const std::string& data = "") {
        std::vector<uint8_t> message;
        message.push_back(static_cast<uint8_t>(action_code));
        uint16_t data_length = static_cast<uint16_t>(data.length());
        message.push_back(static_cast<uint8_t>(data_length >> 8));
        message.push_back(static_cast<uint8_t>(data_length & 0xFF));
        if (data_length > 0) {
            message.insert(message.end(), data.begin(), data.end());
        }
        return message;
    }
    
    static std::pair<ActionCode, std::string> decode_action(const std::vector<uint8_t>& message) {
        if (message.size() < 3) {
            throw std::runtime_error("Message too short");
        }
        ActionCode action_code = static_cast<ActionCode>(message[0]);
        uint16_t data_length = (static_cast<uint16_t>(message[1]) << 8) | message[2];
        std::string data;
        if (data_length > 0) {
            if (message.size() < 3 + data_length) {
                throw std::runtime_error("Message shorter than declared data length");
            }
            data.assign(message.begin() + 3, message.begin() + 3 + data_length);
        }
        return {action_code, data};
    }
    
    static std::string action_name(ActionCode code) {
        switch (code) {
            case ActionCode::WELCOME: return "WELCOME";
            case ActionCode::ECHO: return "ECHO";
            case ActionCode::PING: return "PING";
            case ActionCode::MOVE_UP: return "MOVE_UP";
            case ActionCode::MOVE_DOWN: return "MOVE_DOWN";
            case ActionCode::MOVE_LEFT: return "MOVE_LEFT";
            case ActionCode::MOVE_RIGHT: return "MOVE_RIGHT";
            case ActionCode::ATTACK: return "ATTACK";
            case ActionCode::DEFEND: return "DEFEND";
            case ActionCode::STATUS_UPDATE: return "STATUS_UPDATE";
            case ActionCode::SEND_OBSERVATION: return "SEND_OBSERVATION";
            case ActionCode::START_GAME: return "START_GAME";
            case ActionCode::ERROR: return "ERROR";
            default: return "UNKNOWN";
        }
    }
};

// Forward declaration
class NetworkedGameClient;

class WebSocketClient {
public:
    WebSocketClient(asio::io_context& ioc, const std::string& host, const std::string& port, NetworkedGameClient* game_client)
        : resolver_(ioc), ws_(ioc), host_(host), game_client_(game_client)
    {
        
        
        // Resolve DNS and connect
        auto const results = resolver_.resolve(host, port);
        asio::connect(ws_.next_layer(), results);
        ws_.handshake(host, "/");
        ws_.binary(true);
        std::cout << "Connected to " << host << "\n";
    }

    void send_action(ActionCode action_code, const std::string& data = "") {
        try {
            std::vector<uint8_t> message = MessageHandler::encode_action(action_code, data);
            ws_.write(asio::buffer(message.data(), message.size()));
            std::cout << "SENT: " << MessageHandler::action_name(action_code) 
                    << " (Size: " << message.size() << " bytes)\n";
        } catch (std::exception& e) {
            std::cerr << "Send failed: " << e.what() << "\n";
        }
    }

void receive_actions_loop() {
    std::cout << "WebSocket thread started" << std::endl;
    
    try {
        while (true) {
            beast::flat_buffer buffer;
            
            try {
                ws_.read(buffer);
                
                // Your existing message processing code:
                std::vector<uint8_t> message_data;
                auto const buffer_bytes = static_cast<const uint8_t*>(buffer.data().data());
                auto const buffer_size = buffer.size();
                message_data.assign(buffer_bytes, buffer_bytes + buffer_size);
                
                try {
                    auto [action_code, data] = MessageHandler::decode_action(message_data);
                    std::cout << "Received action: " << MessageHandler::action_name(action_code);
                    if (!data.empty()) {
                        std::cout << " with data: " << data;
                    }
                    std::cout << std::endl;
                    handle_received_action(action_code);
                } catch (const std::exception& e) {
                    std::cout << "Failed to decode message: " << e.what() << std::endl;
                }
                
            } catch (const beast::system_error& e) {
                // Check if it's a closed connection error
                if (e.code() == websocket::error::closed) {
                    std::cout << "WebSocket closed normally, exiting thread" << std::endl;
                    break;
                } else {
                    // Other system errors
                    std::cout << "WebSocket system error: " << e.what() << " - thread will continue" << std::endl;
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
            } catch (const std::exception& e) {
                // Other exceptions
                std::cout << "WebSocket error: " << e.what() << " - thread will continue" << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
    } catch (std::exception& e) {
        std::cerr << "Receive loop fatal error: " << e.what() << "\n";
    }
    
    std::cout << "WebSocket thread finished" << std::endl;
}

    void handle_received_action(ActionCode action_code);
    void close() { ws_.close(websocket::close_code::normal); }

private:
    tcp::resolver resolver_;
    websocket::stream<tcp::socket> ws_;
    std::string host_;
    NetworkedGameClient* game_client_ = nullptr;

};


class NetworkedGameClient : public Game {
public:
    NetworkedGameClient(asio::io_context& ioc, const std::string& host, const std::string& port, 
                        const int _bx, const int _by, const int _framerate)
        : Game(_bx, _by, _framerate),
          webSocketClient_(ioc, host, port, this)
    {
        
        
        // Start the game loop (shows waiting screen)
    }     
    void startNetworkAsync() {
    receiveThread_ = std::thread(&WebSocketClient::receive_actions_loop, &webSocketClient_);
}


void stop() {
    gameEnded = true;
    webSocketClient_.close();
    if (receiveThread_.joinable()) receiveThread_.join();
}

void startNetwork() {
    startNetworkAsync();
}

    ~NetworkedGameClient() {
        stop();
    }

    void applyAgentAction(ActionCode action) {
        std::cout << "AGENT COMMAND RECEIVED: " << MessageHandler::action_name(action) << " -> Applying to game logic." << std::endl;

        auto player = Driver::realPlayer;
        if (!player || !player->canDrive()) return;

        // Reset previous inputs at the start of each action
        player->speedForward = 0.0f;
        player->speedTurn = 0.0f;

        switch (action) {
            case ActionCode::MOVE_UP: 
                player->speedForward = 1.0f;  // Accelerate
                break;
            case ActionCode::MOVE_DOWN:
                player->speedForward = -1.0f; // Brake/Reverse
                break;
            case ActionCode::MOVE_LEFT:
                player->speedTurn = -1.0f;    // Turn left
                break;
            case ActionCode::MOVE_RIGHT:
                player->speedTurn = 1.0f;     // Turn right
                break;
            case ActionCode::ATTACK:
                // Use item if available
                if (player->canUsePowerUp()) {
                    player->pickUpPowerUp(PowerUps::NONE); // Trigger item use
                }
                break;
            default:
                // No action or unknown action - maintain current speed
                break;
        }

        // Apply terrain effects using proper map dimensions
        sf::Vector2f normPos(
            player->position.x / MAP_ASSETS_WIDTH,
            player->position.y / MAP_ASSETS_HEIGHT
        );

        LandMaterial mat = Map::getMaterial(normPos);

        if (mat == LandMaterial::WATER || mat == LandMaterial::LAVA || mat == LandMaterial::VOID) {
            player->speedForward *= 0.5f;
            player->speedTurn *= 0.7f;
            if (mat == LandMaterial::WATER) {
                Map::addEffectDrown(player->position, true);
            }
        } else if (mat == LandMaterial::GRASS || mat == LandMaterial::DIRT) {
            player->speedForward *= 0.75f;
        }
    // STONE and RAINBOW are normal track materials - no changes needed
    }  

    void updateInputBlocking() {
        // This will be called regularly to update input blocking state

        

        bool inRaceState = isInRaceState();

        //THIS LINE CAUSES THE GAME TO CRASH
        if (window.isOpen()) {
    //Input::disableInputs(inRaceState);
}

    }

    void run() override {
    Input::setGameWindow(window);
    std::cout << "[DEBUG] setGameWindow(): window addr = " << &window << ", isOpen=" << window.isOpen() << "\n";
    is_running_ = true;

    sf::Clock timer;
    sf::Time lastTime = sf::Time::Zero;
    sf::Time fixedUpdateStep = sf::seconds(1.0f / framerate);
    sf::Time fixedUpdateTime = sf::Time::Zero;

    while (!gameEnded) {
        StatePtr currentState = getCurrentState();

        sf::Time time = timer.getElapsedTime();
        sf::Time deltaTime = time - lastTime;
        lastTime = time;
        if (deltaTime > sf::seconds(1.0f)) continue;

        handleEvents(currentState);
        bool updated = currentState->update(deltaTime);
        
        fixedUpdateTime += deltaTime;
        while (fixedUpdateTime >= fixedUpdateStep) {
            fixedUpdateTime -= fixedUpdateStep;
            updated = currentState->fixedUpdate(fixedUpdateStep) || updated;
            
            updateInputBlocking();
            // SEND GAME STATE TO SERVER at fixed intervals
            frameCounter++;
            if (isInRaceState() && frameCounter >= SEND_EVERY_N_FRAMES) {
                std::string observation = serializeGameState();
                webSocketClient_.send_action(ActionCode::SEND_OBSERVATION, observation);
                frameCounter = 0;
            }
        }
        
        // KEEP RENDERING - Show visuals
        if (updated) {
            currentState->draw(window);
            window.display();
        }

        handleTryPop();
    }
    is_running_ = false;
    Input::disableInputs(false);
}

void startGame() {
    std::cout << "SERVER COMMAND: Game is ready for AI control!" << std::endl;

}
private:
    WebSocketClient webSocketClient_;
    std::thread receiveThread_;
    std::atomic<bool> gameEnded{false};
    bool is_running_ = false;
    int frameCounter = 0;
    const int SEND_EVERY_N_FRAMES = 6;

         bool isInRaceState() {
        StatePtr current = getCurrentState();
        if (!current) return false;
        
        // Check if we're in the actual race state
        std::string stateName = current->string();
        return (stateName == "Race"); // Exact match for "Race"
    }

std::string serializeGameState() {
    auto player = Driver::realPlayer;
    if (!player) return "GameFrame:0,PlayerHealth:0,speed:0.0";
    
    // Send ACTUAL data instead of hardcoded values
    return "GameFrame:" + std::to_string(frameCounter) + 
           ",PlayerHealth:100" +
           ",speed:" + std::to_string(player->speedForward);
}

    bool isInGameplayMode() {
        // Since we removed WaitingState, just check if we have any state
        StatePtr current = getCurrentState();
        return current != nullptr;
    }
   


 };

void WebSocketClient::handle_received_action(ActionCode action_code) {
    if (!game_client_) return;

    std::cout << "Received action: " << MessageHandler::action_name(action_code) << std::endl;

    switch (action_code) {
        case ActionCode::START_GAME:
            game_client_->startGame();
            break;

        case ActionCode::MOVE_UP:
        case ActionCode::MOVE_DOWN:
        case ActionCode::MOVE_LEFT:
        case ActionCode::MOVE_RIGHT:
        case ActionCode::ATTACK:
        case ActionCode::DEFEND:
            game_client_->applyAgentAction(action_code);
            break;
        
        case ActionCode::WELCOME: 
        case ActionCode::PING:
        case ActionCode::STATUS_UPDATE:
            break;

        case ActionCode::ERROR:
            std::cerr << "Received ERROR from server." << std::endl;
            break;
            
        default:
            std::cout << "Received unhandled action.\n";
            break;
    }
}

//g++ -std=c++17 $(find bin -name "*.o" -not -name "main.o") webserver/networked_main.cpp -o networked_client -Isrc -lboost_system -lboost_thread -lpthread -lsfml-graphics -lsfml-window -lsfml-system -lsfml-audio

//./networked_client