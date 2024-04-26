/*
 * A5MS1
 * The ser
 * Cassie Zhang xzhan304
 * Tianai Yue tyue4
 */

#include <iostream>
#include <cassert>
#include <memory>
#include "csapp.h"
#include "exceptions.h"
#include "guard.h"
#include "server.h"

// Server::Server()
//   // TODO: initialize member variables
// {
//   // TODO: implement
// }

Server::Server() {
    pthread_mutex_init(&mutex, nullptr);
}

Server::~Server()
{
    // TODO: implement
    running = false;
    Close(listenfd);
    pthread_mutex_destroy(&mutex);
}

void Server::listen( const std::string &port )
{
  // TODO: implement
  listenfd = open_listenfd(port.c_str());
    if (listenfd < 0) {
        log_error("Failed to listen on port: " + port);
        exit(1);
    }
}

void Server::server_loop()
{
  // TODO: implement

  // Note that your code to start a worker thread for a newly-connected
  // client might look something like this:
/*
  ClientConnection *client = new ClientConnection( this, client_fd );
  pthread_t thr_id;
  if ( pthread_create( &thr_id, nullptr, client_worker, client ) != 0 )
    log_error( "Could not create client thread" );
*/
    std::cout << "Server is running and waiting for connections..." << std::endl;
    sockaddr_storage clientaddr;
    socklen_t clientlen = sizeof(sockaddr_storage);
    pthread_t tid;

    while (running) {
        int clientfd = accept(listenfd, (sockaddr *)&clientaddr, &clientlen);
        if (clientfd < 0) {
            log_error("Failed to accept client connection");
            continue;
        }

        auto clientConn = new ClientConnection(this, clientfd);
        pthread_t client_thread;
        if (pthread_create(&client_thread, nullptr, client_worker, clientConn) != 0) {
            log_error("Could not create client thread");
            delete clientConn;
        } else {
            pthread_detach(client_thread);
        }
    }
}


void *Server::client_worker( void *arg )
{
  // TODO: implement

  // Assuming that your ClientConnection class has a member function
  // called chat_with_client(), your implementation might look something
  // like this:
/*
  std::unique_ptr<ClientConnection> client( static_cast<ClientConnection *>( arg ) );
  client->chat_with_client();
  return nullptr;
*/
    auto client = static_cast<ClientConnection *>(arg);
    client->chat_with_client();
    delete client;
    return nullptr;
}

void Server::log_error( const std::string &what )
{
  std::cerr << "Error: " << what << "\n";
}

// TODO: implement member functions
bool Server::create_table(const std::string &name) {
    Guard guard(mutex);
    if (tables.find(name) == tables.end()) {
        tables.emplace(std::piecewise_construct,
                       std::forward_as_tuple(name),
                       std::forward_as_tuple(name));
        return true;
    }
    return false;
}

Table* Server::find_table(const std::string &name) {
    Guard guard(mutex);
    auto it = tables.find(name);
    if (it != tables.end()) {
        return &it->second;
    }
    return nullptr;
}

void Server::begin_transaction(int client_id) {
    pthread_mutex_lock(&mutex);
    in_transaction[client_id] = true;
    pthread_mutex_unlock(&mutex);
}

void Server::commit_transaction(int client_id) {
    pthread_mutex_lock(&mutex);
    if (in_transaction[client_id]) {
        for (const auto& table_name : transaction_locks[client_id]) {
            tables[table_name].commit_changes();
            unlock_table(table_name, client_id);
        }
        in_transaction[client_id] = false;
    }
    pthread_mutex_unlock(&mutex);
}

void Server::rollback_transaction(int client_id) {
    Guard guard(mutex);
    // Assume there is a map called transaction_locks that keeps track of all the tables a client has locked
    if (is_transaction_active(client_id)) {
        for (const auto& table_name : transaction_locks[client_id]) {
            Table* table = find_table(table_name);
            if (table) {
                table->rollback_changes();
                table->unlock();
            }
        }
        in_transaction[client_id] = false;
        transaction_locks.erase(client_id); // Clear all locks related to this transaction
    }
}

bool Server::lock_table(const std::string& table_name, int client_id) {
    auto it = tables.find(table_name);
    if (it == tables.end()) {
        return false; 
    }
    if (it->second.trylock()) {
        transaction_locks[client_id].insert(table_name);
        return true;
    }
    return false;
}

void Server::unlock_table(const std::string& table_name, int client_id) {
    auto it = tables.find(table_name);
    if (it != tables.end() && transaction_locks[client_id].find(table_name) != transaction_locks[client_id].end()) {
        it->second.unlock();
        transaction_locks[client_id].erase(table_name);
    }
}

bool Server::is_transaction_active(int client_id) {
    Guard guard(mutex); // Assuming you have a Guard class that handles locking
    // Check if there is an active transaction for the given client ID
    return in_transaction.find(client_id) != in_transaction.end() && in_transaction[client_id];
}


