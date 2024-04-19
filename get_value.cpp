#include <iostream>
#include <cstring>
#include <cstdlib>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

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
  // Create socket
    if (port < 1024) {
        std::cerr << "Invalid port number. Port must be 1024 or above.\n";
        return 1;
    }

    // Create socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        std::cerr << "Failed to create socket.\n";
        return 1;
    }

    // Server address structure
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(port);

    // Connect to the server
    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        std::cerr << "Connection Failed.\n";
        close(sock);
        return 1;
    }

    // Send the login request
    std::string loginMessage = "LOGIN " + username + "\n";
    if (send(sock, loginMessage.c_str(), loginMessage.size(), 0) < 0) {
        std::cerr << "Send failed.\n";
        close(sock);
        return 1;
    }

    // Wait for "OK" response
    char response[4096] = {0};
    ssize_t valread = read(sock, response, sizeof(response) - 1);
    if (valread < 0)
    {
        std::cerr << "Read failed.\n";
        close(sock);
        return 1;
    }
    response[valread] = '\0'; // Null-terminate the response
    std::cout << "Received: " << response << std::endl;
    
    // Check if the response is "OK"
    if (std::string(response).find("OK") == std::string::npos)
    {
        std::cerr << "Did not receive 'OK' response from server.\n";
        close(sock);
        return 1;
    }
    

    // Send the request
    std::string message = "GET " + table + " " + key + " FROM " + username + "\n";
    if (send(sock, message.c_str(), message.size(), 0) < 0) {
        std::cerr << "Send failed.\n";
        close(sock);
        return 1;
    }

    // Receive the response
    char buffer[4096] = {0};
    //ssize_t valread = read(sock, buffer, 4095);
    if (valread < 0) {
        std::cerr << "Read failed.\n";
        close(sock);
        return 1;
    }

    // Output received data
    std::cout << "Received: " << buffer << std::endl;

    // Close socket
    close(sock);
    return 0;
}