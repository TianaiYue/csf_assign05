
#ifndef SERVER_H
#define SERVER_H

#include <map>
#include <string>
#include <pthread.h>
#include "table.h"
#include "client_connection.h"

class Server {
private:
  // TODO: add member variables
  int listenfd;
  std::map<std::string, Table> tables;
  pthread_mutex_t mutex;
  bool running;

  // copy constructor and assignment operator are prohibited
  Server(const Server&);
  Server& operator=(const Server&);

public:
  Server();
  ~Server();
  // TODO: add member variables
  std::map<int, std::set<std::string>> transaction_locks;
  std::map<int, bool> in_transaction;

  void listen( const std::string &port );
  void server_loop();

  static void *client_worker( void *arg );

  void log_error( const std::string &what );

  // TODO: add member functions
  bool create_table(const std::string &name);
  Table* find_table(const std::string &name);
  void begin_transaction(int client_id);
  void commit_transaction(int client_id);
  void rollback_transaction(int client_id);
  bool lock_table(const std::string& table_name, int client_id);
  void unlock_table(const std::string& table_name, int client_id);
  bool is_transaction_active(int client_id);

  // Some suggested member functions:
/*
  void create_table( const std::string &name );
  Table *find_table( const std::string &name );
  void log_error( const std::string &what );
*/
};


#endif // SERVER_H
