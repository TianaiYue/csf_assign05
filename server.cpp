#include <iostream>
#include <cassert>
#include <memory>
#include "csapp.h"
#include "exceptions.h"
#include "guard.h"
#include "server.h"
#include "table.h"

Server::Server()
  // TODO: initialize member variables
{
  // TODO: implement
  listenfd = -1;
}

Server::~Server()
{
  // TODO: implement
  close(listenfd);

}

void Server::listen( const std::string &port )
{
  // TODO: implement
  listenfd = Open_listenfd(port.c_str());
  if (listenfd < 0) {
    log_error("Failed to open listen socket");
    throw std::runtime_error("Failed to open listen socket");
  }
}

void Server::server_loop()
{
// TODO: implement

// Note that your code to start a worker thread for a newly-connected
// client might look something like this:

//   ClientConnection *client = new ClientConnection( this, client_fd );
//   pthread_t thr_id;
//   if ( pthread_create( &thr_id, nullptr, client_worker, client ) != 0 )
//       log_error( "Could not create client thread" );

  while (true) {
    int client_fd = Accept(listenfd, nullptr, nullptr);
    if (client_fd < 0) {
        log_error("Failed to accept client connection");
        continue;  // Continue listening for other connections
    }

    // Dynamically allocate a new ClientConnection object
    ClientConnection* client = new ClientConnection(this, client_fd);

    pthread_t thr_id;
    if (pthread_create(&thr_id, nullptr, client_worker, client) != 0) {
      log_error("Could not create client thread");
      delete client;  // Clean up on thread creation failure
    } else {
      // Detach the thread to allow it to run independently
      pthread_detach(thr_id);
    }
  }
}

void *Server::client_worker( void *arg )
{
// TODO: implement

// Assuming that your ClientConnection class has a member function
// called chat_with_client(), your implementation might look something
// like this:

  // std::unique_ptr<ClientConnection> client( static_cast<ClientConnection *>( arg ) );
  // client->chat_with_client();
  // return nullptr;

  std::unique_ptr<ClientConnection> client( static_cast<ClientConnection *>( arg ) );
  try {
    client->chat_with_client();
  } catch (const std::exception& e) {
    std::cerr << "Error communicating with client: " << e.what() << std::endl;
    client->send_error("ERROR");  // Send an error response if something goes wrong
  }
  return nullptr;
}

void Server::log_error( const std::string &what )
{
  std::cerr << "Error: " << what << "\n";
}

// TODO: implement member functions

void Server::create_table(const std::string &name) {
  Guard guard(mutex);

  if (tables.find(name) != tables.end()) {
      throw OperationException("Table already exists");
  }
  
  // Create a new table and add it to the map
  Table *new_table = new Table(name);
  tables[name] = new_table;
}

Table *Server::find_table(const std::string &name) {
  Guard guard(mutex);
  auto it = tables.find(name);
  if (it != tables.end()) {
    return it->second;
  } else {
    throw OperationException("Table not found\n");
  }
}