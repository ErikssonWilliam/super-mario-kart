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
    /**
     * Encode a statevector of integers to be sent to the server
     * Format: [vector_size(4 bytes)][int1(4 bytes)][int2(4 bytes)]...
     */
    static std::vector<uint8_t> encode_vector_message(const std::vector<int>& data) {
        std::vector<uint8_t> message;
        
        // Add vector size (4 bytes, big-endian)
        // The size of the vector is determined by 4 bytes. 0xFF means filter one byte >> means shift right. 
        // Might not be needed since we know the size of the vector already.
        uint32_t size = static_cast<uint32_t>(data.size());
        message.push_back((size >> 24) & 0xFF);
        message.push_back((size >> 16) & 0xFF);
        message.push_back((size >> 8) & 0xFF);
        message.push_back(size & 0xFF);
        
        // Add each integer (4 bytes each, big-endian) 4 bytes is integer size. 
        for (int value : data) {
            uint32_t int_value = static_cast<uint32_t>(value);
            message.push_back((int_value >> 24) & 0xFF);
            message.push_back((int_value >> 16) & 0xFF);
            message.push_back((int_value >> 8) & 0xFF);
            message.push_back(int_value & 0xFF);
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


    /**
     * Send GameState data to the server
     * Format: 3 floats + 2 booleans (14 bytes total)
     */
    void send_gamestate(float speed, float dist_to_next, float angle_to_next, bool off_track, bool done) {
        try {
            // Create 14-byte message: 3 floats (4 bytes each) + 2 booleans (1 byte each)
            std::vector<uint8_t> message(14);
            
            // Pack floats (big-endian)
            uint32_t speed_bits = *reinterpret_cast<uint32_t*>(&speed);
            uint32_t dist_bits = *reinterpret_cast<uint32_t*>(&dist_to_next);
            uint32_t angle_bits = *reinterpret_cast<uint32_t*>(&angle_to_next);
            
            // Convert to big-endian
            message[0] = (speed_bits >> 24) & 0xFF;
            message[1] = (speed_bits >> 16) & 0xFF;
            message[2] = (speed_bits >> 8) & 0xFF;
            message[3] = speed_bits & 0xFF;
            
            message[4] = (dist_bits >> 24) & 0xFF;
            message[5] = (dist_bits >> 16) & 0xFF;
            message[6] = (dist_bits >> 8) & 0xFF;
            message[7] = dist_bits & 0xFF;
            
            message[8] = (angle_bits >> 24) & 0xFF;
            message[9] = (angle_bits >> 16) & 0xFF;
            message[10] = (angle_bits >> 8) & 0xFF;
            message[11] = angle_bits & 0xFF;
            
            // Pack booleans
            message[12] = off_track ? 1 : 0;
            message[13] = done ? 1 : 0;
            
            // Send the message
            ws_.write(asio::buffer(message));
            std::cout << "Sent GameState: speed=" << speed << ", dist=" << dist_to_next 
                      << ", angle=" << angle_to_next << ", off_track=" << off_track 
                      << ", done=" << done << std::endl;
        } catch (std::exception& e) {
            std::cerr << "Error sending GameState: " << e.what() << std::endl;
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
        
        // Send GameState data to Python server
        client.send_gamestate(15.5f, 25.0f, 0.1f, false, false);
        
        // Send another GameState with different data
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        client.send_gamestate(20.0f, 10.0f, -0.3f, true, false);
        
        // Send final GameState
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        client.send_gamestate(0.0f, 0.0f, 0.0f, false, true);
        
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
// g++ -std=c++17 -I/path/to/boost websocket.cpp -lboost_system -pthread -o websocket