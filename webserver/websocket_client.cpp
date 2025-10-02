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
    ERROR = 0xFF
};

class MessageHandler {
public:
    static std::vector<uint8_t> encode_action(ActionCode action_code, const std::string& data = "") {
        std::vector<uint8_t> message;
        
        // Add action code (1 byte)
        message.push_back(static_cast<uint8_t>(action_code));
        
        // Add data length (2 bytes, big-endian)
        uint16_t data_length = static_cast<uint16_t>(data.length());
        message.push_back(static_cast<uint8_t>(data_length >> 8));   // High byte
        message.push_back(static_cast<uint8_t>(data_length & 0xFF)); // Low byte
        
        // Add data if present
        if (data_length > 0) {
            message.insert(message.end(), data.begin(), data.end());
        }
        
        return message;
    }
    
    /**
     * Decode an action message
     * Returns: pair of (action_code, data_string)
     */
    static ActionCode decode_action(const std::vector<uint8_t>& message) {
     
        // Extract action code (1 byte)
        ActionCode action_code = static_cast<ActionCode>(message[0]);
        
        return action_code;
    }
    
    /**
     * Get human-readable name for action code
     */
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
            case ActionCode::ERROR: return "ERROR";
            default: return "UNKNOWN";
        }
    }
};

class WebSocketClient {
public:
    WebSocketClient(asio::io_context& ioc, const std::string& host, const std::string& port)
        : resolver_(ioc), ws_(ioc), host_(host) 
    {
        // Resolve DNS aka look up the domain name of the host
        auto const results = resolver_.resolve(host, port);

        // Connect TCP. This establishes the connection to the server.
        asio::connect(ws_.next_layer(), results);

        // Perform WebSocket handshake. This happens once in the lifetime of the connection.
        ws_.handshake(host, "/");

        // Set binary mode
        ws_.binary(true);

        // print that connection has been established
        std::cout << "Connected to " << host << "\n";
    }


    void receive_actions_loop() {
        try {
            // Loop indefinetly
            for (;;) {
                // Create buffer to hold incoming message
                beast::flat_buffer buffer;
                // Get incomming message
                ws_.read(buffer);
                
                // Convert buffer to vector for easier handling
                std::vector<uint8_t> message_data;
                auto const buffer_bytes = static_cast<const uint8_t*>(buffer.data().data());
                auto const buffer_size = buffer.size();
                message_data.assign(buffer_bytes, buffer_bytes + buffer_size);
                
                try {
                    // Try to decode as action message
                    ActionCode action_code = MessageHandler::decode_action(message_data);
                    std::cout << "Received action: " << MessageHandler::action_name(action_code);
                    
                    // Handle specific actions if needed
                    handle_received_action(action_code);
                    
                } catch (const std::exception& e) {
                    // If decoding fails, log the error but continue
                    std::cout << "Received non-action message (size: " << message_data.size() << " bytes)" << std::endl;
                }
            }
        } catch (std::exception& e) {
            std::cerr << "Receive loop stopped: " << e.what() << "\n";
        }
    }

    /**
     * Handle received actions from the server
     * Override this method to implement specific action handling
     */
    void handle_received_action(ActionCode action_code) {
        switch (action_code) {
            case ActionCode::WELCOME:
                std::cout << "  -> Server welcomed us!" << std::endl;
                break;
            case ActionCode::PING:
                std::cout << "  -> Server sent ping, connection alive" << std::endl;
                break;
            default:
                std::cout << "  -> Other Action. No handler for this" << std::endl;
                break;
        }
    }

    void close() {
        ws_.close(websocket::close_code::normal);
    }

private:
    tcp::resolver resolver_;
    websocket::stream<tcp::socket> ws_;
    std::string host_;
};

int main() {
    try {
        // Create ioc object
        asio::io_context ioc;
        // localhost and port 8080
        //WebSocketClient client(ioc, "127.0.0.1", "8080");
        WebSocketClient client(ioc, "host.docker.internal", "8080");

        // Start background thread for receiving actions
        std::thread recv_thread([&client]() {
            client.receive_actions_loop();
        });

        // Give some time for welcome message
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Wait for responses and keep connection alive to receive actions
        std::cout << "\nListening for actions from server...\n";
        std::cout << "Press Ctrl+C to exit\n";
        
        // Keep main thread alive to receive actions
        std::this_thread::sleep_for(std::chrono::seconds(10));

        client.close();
        recv_thread.join();
    } catch (std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
    }
}

// Compilation command:
// g++ -std=c++17 -I/path/to/boost client.cpp -lboost_system -pthread -o websocket_client