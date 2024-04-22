/*
 * A5MS1
 * The set_value client
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
 * Communicates with the server by sending requests and handling responses
 *
 * Parameters:
 *   clientfd - The client socket file descriptor connected to the server
 *   requests - A list of formatted request strings to send to the server
 *
 * Returns:
 *   Returns 0 on success, 1 on error
 */
int handle_server_communication(int clientfd, const std::vector<std::string>& requests) {
    char buffer[1024];
    Message response_msg;

    for (const auto& req : requests) {
        memset(buffer, 0, 1024);  // Clear buffer
        strcpy(buffer, req.c_str());  // Copy the request into the buffer

        // Send request to the server
        if (write(clientfd, buffer, strlen(buffer)) < 0) {
            std::cerr << "Error: unable to write to server\n";
            return 1;
        }

        // Read response from the server
        ssize_t bytes_read = read(clientfd, buffer, 1024);
        if (bytes_read < 0) {
            std::cerr << "Error: unable to read from server\n";
            return 1;
        }
        buffer[bytes_read] = '\0';  // Ensure null termination

        // Decode the received message
        std::string received_msg(buffer);
        MessageSerialization::decode(received_msg, response_msg);

        // Check for error in the response
        if (response_msg.get_message_type() == MessageType::ERROR || response_msg.get_message_type() == MessageType::FAILED) {
            std::cerr << "Error: " + response_msg.get_quoted_text() << "\n";
            return 1;
        }
    }
    return 0;
}

/*
 * Main function to setup client, parse command line arguments, and manage server communications.
 *
 * Parameters:
 *   argc - Number of command-line arguments.
 *   argv - Command-line argument values.
 *
 * Returns:
 *   Returns 0 for successful execution, 1 for failure
 */
int main(int argc, char **argv) {
    if (argc != 7) {
        std::cerr << "Usage: ./set_value <hostname> <port> <username> <table> <key> <value>\n";
        return 1;
    }

    std::string hostname = argv[1];
    std::string port = argv[2];
    std::string username = argv[3];
    std::string table = argv[4];
    std::string key = argv[5];
    std::string value = argv[6];

    // List of requests to be sent to the server
    std::vector<std::string> requests = {
        "LOGIN " + username + "\n",
        "PUSH " + value + "\n",
        "SET " + table + " " + key + "\n",
        "BYE\n"
    };

    char* buffer = new char[1024];

    // Open client file descriptor to the server
    int clientfd = open_clientfd(hostname.c_str(), port.c_str());
    if (clientfd == -1) {
        std::cerr << "Error: Failed to connect to server\n";
        delete[] buffer;
        return 1;
    }

    int result = handle_server_communication(clientfd, requests);

    delete[] buffer;
    close(clientfd);
    return result;
}