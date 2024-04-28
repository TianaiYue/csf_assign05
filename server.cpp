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
    for (auto &pair : tables) {
        delete pair.second;
    }
    tables.clear();
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
/*
  ClientConnection *client = new ClientConnection( this, client_fd );
  pthread_t thr_id;
  if ( pthread_create( &thr_id, nullptr, client_worker, client ) != 0 )
    log_error( "Could not create client thread" );
*/
    while (true) {
        int client_fd = Accept(listenfd, nullptr, nullptr);
        if (client_fd < 0) {
            log_error("Failed to accept client connection");
            continue;
        }

        ClientConnection *client = new ClientConnection(this, client_fd);
        pthread_t thr_id;
        if (pthread_create(&thr_id, nullptr, client_worker, client) != 0) {
            log_error("Could not create client thread");
            delete client;
        } else {
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
/*
  std::unique_ptr<ClientConnection> client( static_cast<ClientConnection *>( arg ) );
  client->chat_with_client();
  return nullptr;
*/
    std::unique_ptr<ClientConnection> client(static_cast<ClientConnection *>(arg));
    try {
        client->chat_with_client();
    } catch (const std::exception &e) {
        std::cerr << "Error communicating with client: " << e.what() << std::endl;
        client->send_message(MessageType::ERROR);
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


Table* Server::find_table(const std::string &name) {
    Guard guard(mutex);
    auto it = tables.find(name);

    if (it != tables.end()) {
        return it->second;
    } else {
        throw OperationException("Table not found");
    }
}

void Server::begin_transaction(int client_id) {
    Guard guard(mutex);
    if (in_transaction.find(client_id) == in_transaction.end() || !in_transaction[client_id]) {
        in_transaction[client_id] = true;
        transaction_locks[client_id].clear();  // Ensure clean transaction state
    }
}

void Server::commit_transaction(int client_id) {
    Guard guard(mutex);
    auto trans_it = in_transaction.find(client_id);
    if (trans_it != in_transaction.end() && trans_it->second) {
        try {
            for (const auto &table_name : transaction_locks[client_id]) {
                auto it = tables.find(table_name);
                if (it != tables.end()) {
                    it->second->commit_changes();
                    it->second->unlock();  // Ensure unlocking after commit
                }
            }
            transaction_locks[client_id].clear();
            in_transaction[client_id] = false;
        } catch (const std::exception& e) {
            // Handle exception, possibly rolling back changes
            std::cerr << "Failed to commit transaction: " << e.what() << std::endl;
            rollback_transaction(client_id);
        }
    }
}

void Server::rollback_transaction(int client_id) {
    Guard guard(mutex);
    auto trans_it = in_transaction.find(client_id);
    if (trans_it != in_transaction.end() && trans_it->second) {
        for (const auto &table_name : transaction_locks[client_id]) {
            auto it = tables.find(table_name);
            if (it != tables.end()) {
                it->second->rollback_changes();
                it->second->unlock();
            }
        }
        transaction_locks[client_id].clear();
        in_transaction[client_id] = false;
    }
}

bool Server::lock_table(const std::string& table_name, int client_id) {
    Guard guard(mutex);
    auto it = tables.find(table_name);
    if (it == tables.end()) {
        throw OperationException("Table not found for locking");
    }
    if (it->second->trylock()) {
        transaction_locks[client_id].insert(table_name);
        return true;
    }
    return false; // Could consider throwing an exception or handling this situation differently.
}

void Server::unlock_table(const std::string& table_name, int client_id) {
    Guard guard(mutex);
    auto it = tables.find(table_name);
    if (it != tables.end() && transaction_locks[client_id].find(table_name) != transaction_locks[client_id].end()) {
        it->second->unlock();
        transaction_locks[client_id].erase(table_name);
    } else {
        // Log or handle the case where an attempt is made to unlock a table that isn't locked by this client.
        std::cerr << "Attempt to unlock a table not locked by client" << std::endl;
    }
}

bool Server::is_transaction_active(int client_id) {
    Guard guard(mutex);
    return in_transaction.find(client_id) != in_transaction.end() && in_transaction[client_id];
}