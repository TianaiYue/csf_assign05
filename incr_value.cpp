#include <iostream>
#include <vector>
#include "csapp.h"
#include "exceptions.h"
#include "message.h"
#include "message_serialization.h"
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

    // Implement

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

    char* buffer = new char[1024];
    Message responseMessage;

    int clientfd = open_clientfd(hostname.c_str(), port.c_str());
    if (clientfd == -1) {
        std::cerr << "Error: Failed to connect to server\n";
        delete[] buffer;
        return 1;
    }

    for (const auto& req : requests) {
        memset(buffer, 0, 1024);  // Clear buffer
        strcpy(buffer, req.c_str());

        if (write(clientfd, buffer, strlen(buffer)) < 0) {
            std::cerr << "Error: unable to write to server\n";
            delete[] buffer;
            close(clientfd);
            return 1;
        }

        // Receive response
        ssize_t bytes_read = read(clientfd, buffer, 1024);
        buffer[bytes_read] = '\0';
        if (bytes_read < 0) {
            std::cerr << "Error: unable to read from server\n";
            delete[] buffer;
            close(clientfd);
            return 1;
        }
        
        std::string received_msg(buffer);
        MessageSerialization::decode(received_msg, responseMessage);
        if (responseMessage.get_message_type() == MessageType::ERROR || responseMessage.get_message_type() == MessageType::FAILED) {
            std::cerr << "Error: " + responseMessage.get_quoted_text() << "\n";
            delete[] buffer;
            close(clientfd);
            return 1;
        }
    }

    close(clientfd);
    delete[] buffer;
    return 0;  // Successful exit without printing any output
}