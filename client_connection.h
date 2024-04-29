#ifndef CLIENT_CONNECTION_H
#define CLIENT_CONNECTION_H

#include <set>
#include "message.h"
#include "csapp.h"
#include "value_stack.h"

class Server; // forward declaration
class Table; // forward declaration

class ClientConnection {
private:
  Server *m_server;
  int m_client_fd;
  rio_t m_fdbuf;
  ValueStack stack;
  bool in_transaction = false;
  std::set<Table*> locked_tables;

  // copy constructor and assignment operator are prohibited
  ClientConnection( const ClientConnection & );
  ClientConnection &operator=( const ClientConnection & );

public:
  ClientConnection( Server *server, int client_fd );
  ~ClientConnection();

  void chat_with_client();

  // TODO: additional member functions
  Message read_message();
  void send_message(const Message& msg);
  void handle_request(const Message& request);
  void close_connection();
  
  void handle_login_request(const Message &request);
  void handle_begin_request();
  void handle_commit_request();
  void handle_create_request(const Message &request);
  void handle_push_request(const Message &request);
  void handle_pop_request();
  void handle_top_request();
  void handle_set_request(const Message &request);
  void handle_get_request(const Message &request);
  void handle_exception(const std::exception& e);
  void handle_arithmetic_request(const Message &request);
  void handle_bye_request();
  void handle_rollback_request();

  bool is_valid_username(const std::string& username);
  void rollback_transaction();
  void begin_transaction();
  void commit_transaction();
  void lock_table(Table* table);
  void unlock_all_locked_tables();

};

#endif // CLIENT_CONNECTION_H