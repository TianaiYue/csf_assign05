#include <iostream>
#include <cstdlib>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

bool send_message(int sockfd, const std::string& message) {
    ssize_t bytes_sent = send(sockfd, message.c_str(), message.size(), 0);
    if (bytes_sent < 0) {
        std::cerr << "Error: Failed to send message\n";
        return false;
    }
    return true;
}

bool receive_message(int sockfd, std::string& message) {
    const int BUFFER_SIZE = 1024;
    char buffer[BUFFER_SIZE] = {0}; // Initialize buffer to zero
    ssize_t bytes_received = recv(sockfd, buffer, BUFFER_SIZE - 1, 0); // Leave space for null terminator
    if (bytes_received < 0) {
        std::cerr << "Error: Failed to receive message\n";
        return false;
    } else if (bytes_received == 0) {
        std::cerr << "Error: Connection closed by server\n";
        return false;
    }
    buffer[bytes_received] = '\0'; // Ensure null termination
    message = buffer;
    return true;
}


int main(int argc, char **argv)
{
  if (argc != 7) {
    std::cerr << "Usage: ./set_value <hostname> <port> <username> <table> <key> <value>\n";
    return 1;
  }

  std::string hostname = argv[1];
  int port = std::stoi(argv[2]);
  std::string username = argv[3];
  std::string table = argv[4];
  std::string key = argv[5];
  std::string value = argv[6];

  // TODO: implement
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        std::cerr << "Error: Failed to create socket\n";
        return 1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, hostname.c_str(), &server_addr.sin_addr);

    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Error: Failed to connect to server\n";
        close(sockfd);
        return 1;
    }

    // Send LOGIN message
    if (!send_message(sockfd, "LOGIN " + username + "\n")) {
        close(sockfd);
        return 1;
    }

    // Check login response
    std::string response;
    if (!receive_message(sockfd, response) || response.find("OK") == std::string::npos) {
        std::cerr << "Login failed: " << response << '\n';
        close(sockfd);
        return 1;
    }

    // Send PUSH message
    if (!send_message(sockfd, "PUSH " + table + " " + key + "\n")) {
        close(sockfd);
        return 1;
    }

    // Check PUSH response
    if (!receive_message(sockfd, response) || response.find("OK") == std::string::npos) {
        std::cerr << "PUSH failed: " << response << '\n';
        close(sockfd);
        return 1;
    }

    // Send SET message
    if (!send_message(sockfd, "SET " + value + "\n")) {
        close(sockfd);
        return 1;
    }

    // Check SET response
    if (!receive_message(sockfd, response) || response.find("OK") == std::string::npos) {
        std::cerr << "SET failed: " << response << '\n';
        close(sockfd);
        return 1;
    }

    // Send BYE message
    if (!send_message(sockfd, "BYE\n")) {
        close(sockfd);
        return 1;
    }

    // Optionally check the BYE response
    if (!receive_message(sockfd, response) || response.find("OK") == std::string::npos) {
        std::cerr << "Error: Failed to properly logout\n";
        close(sockfd);
        return 1;
    }

    close(sockfd);
    return 0;
}