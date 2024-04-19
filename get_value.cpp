#include <iostream>
#include <cstring>
#include <cstdlib>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

// Function to send a message to the server
bool send_message(int sockfd, const std::string& message) {
    // Send the message
    ssize_t bytes_sent = send(sockfd, message.c_str(), message.size(), 0);
    if (bytes_sent < 0) {
        std::cerr << "Error: Failed to send message\n";
        return false;
    }
    return true;
}

// Function to receive a message from the server
bool receive_message(int sockfd, std::string& message) {
    const int BUFFER_SIZE = 1024;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_received = recv(sockfd, buffer, BUFFER_SIZE, 0);
    if (bytes_received < 0) {
        std::cerr << "Error: Failed to receive message\n";
        return false;
    } else if (bytes_received == 0) {
        std::cerr << "Error: Connection closed by server\n";
        return false;
    }
    // Append received data to message
    message.append(buffer, bytes_received);
    return true;
}

int main(int argc, char **argv)
{
  if ( argc != 6 ) {
    std::cerr << "Usage: ./get_value <hostname> <port> <username> <table> <key>\n";
    return 1;
  }

  std::string hostname = argv[1];
  int port = std::stoi(argv[2]); 
  std::string username = argv[3];
  std::string table = argv[4];
  std::string key = argv[5];

  // TODO: implement
  // Create a socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        std::cerr << "Error: Failed to create socket\n";
        return 1;
    }

    // Initialize server address structure
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, hostname.c_str(), &server_addr.sin_addr);

    // Connect to the server
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Error: Failed to connect to server\n";
        close(sockfd);
        return 1;
    }

    // Send LOGIN message
    std::string login_msg = "LOGIN " + username + "\n";
    if (!send_message(sockfd, login_msg)) {
        close(sockfd);
        return 1;
    }

    // Receive response for LOGIN message
    std::string response;
    if (!receive_message(sockfd, response)) {
        close(sockfd);
        return 1;
    }

    if (response.find("ERROR") != std::string::npos || response.find("FAILED") != std::string::npos) {
        std::cerr << "Error: " << response;
        close(sockfd);
        return 1;
    }

    // Send GET message
    std::string get_msg = "GET " + table + " " + key + "\n";
    if (!send_message(sockfd, get_msg)) {
        close(sockfd);
        return 1;
    }

    // Receive response for GET message
    if (!receive_message(sockfd, response)) {
        close(sockfd);
        return 1;
    }

    if (response.find("ERROR") != std::string::npos || response.find("FAILED") != std::string::npos) {
        std::cerr << "Error: " << response;
        close(sockfd);
        return 1;
    }

    // Send GET message
    std::string top_msg = "TOP \n";
    if (!send_message(sockfd, top_msg)) {
        close(sockfd);
        return 1;
    }

    // Receive response for GET message
    if (!receive_message(sockfd, response)) {
        close(sockfd);
        return 1;
    }

    // Extract the value from the response (assuming the format "DATA value")
    size_t data_pos = response.find("DATA ");
    if (data_pos == std::string::npos) {
        std::cerr << "Error: Unexpected response format\n";
        close(sockfd);
        return 1;
    }

    std::string value = response.substr(data_pos + 5); // Length of "DATA " is 5

    // Print the value
    std::cout << value + "\n";

    // Send BYE message to terminate connection
    std::string bye_msg = "BYE\n";
    if (!send_message(sockfd, bye_msg)) {
        close(sockfd);
        return 1;
    }

    // Receive response for BYE message
    if (!receive_message(sockfd, response)) {
        close(sockfd);
        return 1;
    }

    close(sockfd);
    return 0;
}