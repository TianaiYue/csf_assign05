#include <iostream>
#include <vector>
#include "csapp.h"              // For network functionality
#include "exceptions.h"         // Custom exception classes
#include "message.h"            // Custom message handling class
#include "message_serialization.h"  // For encoding and decoding messages

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

    char* buffer = new char[1024];
    Message responseMessage;

    int clientfd = open_clientfd(hostname.c_str(), port.c_str());

    if (clientfd == -1) {
        std::cerr << "Error: Failed to connect to server\n";
        delete[] buffer;
        return 1;
    }

    std::string result;
    for (const auto& req : requests) {
        // Send request
        memset(buffer, 0, 1024);  // Clear buffer
        strcpy(buffer, req.c_str());
        write(clientfd, buffer, req.size());

        // Receive response
        ssize_t bytes_read = read(clientfd, buffer, 1024);
        buffer[bytes_read] = '\0';
        if (bytes_read < 0) {
            std::cerr << "Error: unable to reading from server\n";
            delete[] buffer;
            close(clientfd);
            return 1;
        }

        // Decode response
        std::string received_msg(buffer);
        MessageSerialization::decode(received_msg, responseMessage);
        if (responseMessage.get_message_type() == MessageType::ERROR || responseMessage.get_message_type() == MessageType::FAILED) {
            std::cerr << "Error: " + responseMessage.get_quoted_text() << "\n";
            delete[] buffer;
            close(clientfd);
            return 1;
        }

        if (responseMessage.get_message_type() == MessageType::DATA) {
            result = responseMessage.get_value();
        }
    }

    close(clientfd);
    delete[] buffer;
    std::cout << result << std::endl;
    return 0;
}
