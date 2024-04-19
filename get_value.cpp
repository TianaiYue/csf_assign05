#include <iostream>
#include <cstdlib>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

bool send_and_receive(int sockfd, const std::string& send_msg, std::string& recv_msg) {
    // Send the message
    ssize_t bytes_sent = send(sockfd, send_msg.c_str(), send_msg.size(), 0);
    if (bytes_sent < 0) {
        std::cerr << "Error: Failed to send message\n";
        return false;
    }

    // Receive the message
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
    recv_msg = buffer;

    // Check for error in the server response
    if (recv_msg.find("ERROR") != std::string::npos || recv_msg.find("FAILED") != std::string::npos) {
        std::cerr << "Error: " << recv_msg;
        return false;
    }

    return true;
}

int main(int argc, char **argv) {
    if (argc != 6) {
        std::cerr << "Usage: ./get_value <hostname> <port> <username> <table> <key>\n";
        return 1;
    }

    std::string hostname = argv[1];
    int port = std::stoi(argv[2]); 
    std::string username = argv[3];
    std::string table = argv[4];
    std::string key = argv[5];

    // Create and connect the socket
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

    // Send LOGIN message and handle response
    std::string response;
    if (!send_and_receive(sockfd, "LOGIN " + username + "\n", response)) {
        close(sockfd);
        return 1;
    }

    // Send GET message and handle response
    if (!send_and_receive(sockfd, "GET " + table + " " + key + "\n", response)) {
        close(sockfd);
        return 1;
    }

    // Send TOP message and handle response
    if (!send_and_receive(sockfd, "TOP \n", response)) {
        close(sockfd);
        return 1;
    }

    // Extract the value from the response assuming format "DATA value"
    size_t data_pos = response.find("DATA ");
    if (data_pos == std::string::npos) {
        std::cerr << "Error: Unexpected response format\n";
        close(sockfd);
        return 1;
    }
    std::string value = response.substr(data_pos + 5); // Skip "DATA "
    std::cout << value << "\n"; // Print the value

    // Send BYE message and handle response
    if (!send_and_receive(sockfd, "BYE\n", response)) {
        close(sockfd);
        return 1;
    }

    close(sockfd);
    return 0;
}
