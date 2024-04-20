#include <iostream>
#include <vector>
#include "csapp.h"
#include "exceptions.h"
#include "message.h"
#include "message_serialization.h"

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

  // TODO: implement
  std::vector<std::string> requests = {
        "LOGIN " + username + "\n",
        "PUSH " + value + "\n",
        "SET " + table + " " + key + "\n",
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

    for (const auto& req : requests) {
        // Clear buffer
        memset(buffer, 0, 1024);
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

        // Decode response
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
    return 0;
}
