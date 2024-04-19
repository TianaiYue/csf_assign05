#include <iostream>
#include <cstring>
#include <cstdlib>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>


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

  // Create socket
  int sock = socket(AF_INET, SOCK_STREAM, 0);  // 'sock' is declared here as an integer.
  if (sock == -1) {
      std::cerr << "Failed to create socket.\n";
      return 1;
  }
  
  // Server address structure
    sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(port);

    // Convert hostname to IP address
    if (port <= 1024) {
        std::cerr << "Invalid port number. Port must be above 1024.\n";
        close(sock);
        return 1;
    }

    // Connect to the server
    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        std::cerr << "Connection Failed.\n";
        close(sock);
        return 1;
    }

    // Prepare the set message
    std::string message = "SET " + table + " " + key + " " + value + " BY " + username + "\n";

    // Send the message
    if (send(sock, message.c_str(), message.size(), 0) < 0) {
        std::cerr << "Send failed.\n";
        close(sock);
        return 1;
    }

    // Receive the response
    char buffer[4096] = {0};
    ssize_t valread = read(sock, buffer, 4095);
    if (valread < 0) {
        std::cerr << "Read failed.\n";
        close(sock);
        return 1;
    }

    // Output received data
    std::cout << "Response: " << buffer << std::endl;

    // Close socket
    close(sock);
    return 0;
}
