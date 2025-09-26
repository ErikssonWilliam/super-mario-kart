#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/io_context.hpp>
#include <iostream>
#include <thread>

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace asio = boost::asio;
using tcp = asio::ip::tcp;

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

        // print that connection has been established
        std::cout << "Connected to " << host << "\n";
    }

    /**
     * Send a message to the WebSocket server.
     * @param message The message to send.
     */
    void send(const std::string& message) {
        ws_.write(asio::buffer(message));
    }


    /**
     * Receive messages from the WebSocket server in a loop. This function runs indefinitely until an error occurs.
     * It prints received messages to the standard output.
     */
    void receive_loop() {
        try {
            // Loop indefinetly
            for (;;) {
                // Create buffer to hold incoming message
                beast::flat_buffer buffer;
                // Get incomming message
                ws_.read(buffer);
                // Print the message to standard output
                std::cout << "Received: " 
                          << beast::make_printable(buffer.data()) 
                          << std::endl;
            }
        } catch (std::exception& e) {
            std::cerr << "Receive loop stopped: " << e.what() << "\n";
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
        WebSocketClient client(ioc, "127.0.0.1", "8080");

        // Start background thread for receiving
        std::thread recv_thread([&client]() {
            client.receive_loop();
        });

        // Simple input loop
        std::string msg;
        while (std::getline(std::cin, msg)) {
            if (msg == "/quit") break;
            client.send(msg);
        }

        client.close();
        recv_thread.join();
    } catch (std::exception& e) { // any exception right now
        std::cerr << "Error: " << e.what() << "\n";
    }
}