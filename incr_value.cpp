#include <iostream>
#include <cstdlib>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

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
  int port = std::atoi(argv[count++]);
  std::string username = argv[count++];
  std::string table = argv[count++];
  std::string key = argv[count++];

  // TODO: implement
  // Create socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        std::cerr << "Failed to create socket.\n";
        return 1;
    }

    // Server address structure
    sockaddr_in server;
    std::memset(&server, 0, sizeof(server));  // Clear structure
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

    // Prepare the increment message
    std::string message = "INCR " + table + " " + key + " BY " + username + (use_transaction ? " TRANSACTION" : "") + "\n";

    // Send the message
    if (send(sock, message.c_str(), message.length(), 0) < 0) {
        std::cerr << "Send failed.\n";
        close(sock);
        return 1;
    }

    // Receive the response
    char buffer[4096];
    std::memset(buffer, 0, sizeof(buffer));  // Clear buffer
    ssize_t valread = read(sock, buffer, sizeof(buffer) - 1);
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
