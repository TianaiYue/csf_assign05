/*
 * A5MS1
 * The incr_value client
 * Cassie Zhang xzhan304
 * Tianai Yue tyue4
 */

#include <iostream>
#include <vector>
#include "csapp.h"
#include "exceptions.h"
#include "message.h"
#include "message_serialization.h"

/*
 * Sends requests to the server and handles the responses
 *
 * Parameters:
 *   clientfd - Client socket file descriptor
 *   requests - List of requests to send to the server
 *
 * Returns:
 *   Returns 0 if all requests are processed successfully, 1 if an error occurred
 */
int handle_server_communication(int clientfd, const std::vector<std::string>& requests) {
    char buffer[1024];
    Message response_msg;

    for (const auto& req : requests) {
        memset(buffer, 0, 1024); // Clear buffer
        strcpy(buffer, req.c_str()); // Copy request to buffer

        // Send request to the server
        if (write(clientfd, buffer, strlen(buffer)) < 0) {
            std::cerr << "Error: unable to write to server\n";
            return 1;
        }

        // Receive response
        ssize_t bytes_read = read(clientfd, buffer, 1024);
        if (bytes_read < 0) {
            std::cerr << "Error: unable to read from server\n";
            return 1;
        }
        buffer[bytes_read] = '\0'; // Null-terminate the received data

        // Decode response
        std::string received_msg(buffer);
        MessageSerialization::decode(received_msg, response_msg);

        // Handle possible errors
        if (response_msg.get_message_type() == MessageType::ERROR || response_msg.get_message_type() == MessageType::FAILED) {
            std::cerr << "Error: " + response_msg.get_quoted_text() << "\n";
            return 1;
        }
    }

    return 0;
}

/*
 * Main function to setup and execute server communication based on command-line arguments
 *
 * Parameters:
 *   argc - Number of command-line arguments
 *   argv - Array of command-line argument values
 *
 * Returns:
 *   Returns 0 for success, 1 for failure
 */
int main(int argc, char **argv) {
    if ( argc != 6 && (argc != 7 || std::string(argv[1]) != "-t") ) {
        std::cerr << "Usage: ./incr_value [-t] <hostname> <port> <username> <table> <key>\n";
        std::cerr << "Options:\n";
        std::cerr << "  -t      execute the increment as a transaction\n";
        return 1;
    }

    int count = 1;
    bool use_transaction = false;
    if ( argc == 7 ) {
        use_transaction = true;
        count = 2;
    }

    std::string hostname = argv[count++];
    std::string port = argv[count++];
    std::string username = argv[count++];
    std::string table = argv[count++];
    std::string key = argv[count++];

    // List of requests to be sent to the server
    std::vector<std::string> requests;
    requests.push_back("LOGIN " + username + "\n");
    if (use_transaction) {
        requests.push_back("BEGIN\n");
    }
    requests.push_back("GET " + table + " " + key + "\n");
    requests.push_back("PUSH 1\n");
    requests.push_back("ADD\n");
    requests.push_back("SET " + table + " " + key + "\n");
    if (use_transaction) {
        requests.push_back("COMMIT\n");
    }
    requests.push_back("BYE\n");

    // Open client file descriptor to the server
    int clientfd = open_clientfd(hostname.c_str(), port.c_str());
    if (clientfd == -1) {
        std::cerr << "Error: Failed to connect to server\n";
        return 1;
    }

    int result = handle_server_communication(clientfd, requests);
    close(clientfd);

    return result;
}