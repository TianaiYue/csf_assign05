/*
 * A5MS1
 * The get_value client
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
 * Function to establish a connection to the server, send requests, and receive responses
 *
 * Parameters:
 *   hostname - The name of the host where the server is running
 *   port - The port number on which the server is listening
 *   requests - A vector of strings containing the requests to be sent to the server
 *   result - A string reference to store the result retrieved from the server
 *
 * Returns:
 *   Returns true if all requests are processed successfully, false if any error occurs
 */
bool send_and_receive(const std::string& hostname, const std::string& port, const std::vector<std::string>& requests, std::string& result) {
    char* buffer = new char[1024];
    Message responseMessage;
    int clientfd = open_clientfd(hostname.c_str(), port.c_str());

    if (clientfd == -1) {
        std::cerr << "Error: failed to connect to server\n";
        delete[] buffer;
        return false;
    }

    for (const auto& req : requests) {
        memset(buffer, 0, 1024);
        strcpy(buffer, req.c_str());
        if (write(clientfd, buffer, strlen(buffer)) < 0) {
            std::cerr << "Error: unable to write to server\n";
            delete[] buffer;
            close(clientfd);
            return false;
        }

        ssize_t bytes_read = read(clientfd, buffer, 1024);
        if (bytes_read < 0) {
            std::cerr << "Error: unable to read from server\n";
            delete[] buffer;
            close(clientfd);
            return false;
        }
        buffer[bytes_read] = '\0';
        std::string received_msg(buffer);
        MessageSerialization::decode(received_msg, responseMessage);

        if (responseMessage.get_message_type() == MessageType::ERROR || responseMessage.get_message_type() == MessageType::FAILED) {
            std::cerr << "Error: " + responseMessage.get_quoted_text() << "\n";
            delete[] buffer;
            close(clientfd);
            return false;
        }

        if (responseMessage.get_message_type() == MessageType::DATA) {
            result = responseMessage.get_value();
        }
    }

    close(clientfd);
    delete[] buffer;
    return true;
}

/*
 * Main function to set up client, parse command line arguments, and manage server communications
 *
 * Parameters:
 *   argc - Number of command-line arguments.
 *   argv - Command-line argument values.
 *
 * Returns:
 *   Returns 0 for successful execution, 1 for failure
 */
int main(int argc, char **argv) {
    if (argc != 6) {
        std::cerr << "Usage: ./get_value <hostname> <port> <username> <table> <key>\n";
        return 1;
    }

    std::string hostname = argv[1];
    std::string port = argv[2];
    std::string username = argv[3];
    std::string table = argv[4];
    std::string key = argv[5];

    std::vector<std::string> requests = {
        "LOGIN " + username + "\n",
        "GET " + table + " " + key + "\n",
        "TOP\n",
        "BYE\n"
    };

    std::string result;
    if (!send_and_receive(hostname, port, requests, result)) {
        return 1;
    }

    std::cout << result << std::endl;
    return 0;
}