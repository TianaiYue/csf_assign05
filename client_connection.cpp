/*
 * A5MS1
 * The client connection
 * Cassie Zhang xzhan304
 * Tianai Yue tyue4
 */

#include <iostream>
#include <cassert>
#include "csapp.h"
#include "message.h"
#include "message_serialization.h"
#include "server.h"
#include "exceptions.h"
#include "client_connection.h"

ClientConnection::ClientConnection( Server *server, int client_fd )
  : m_server( server )
  , m_client_fd( client_fd )
  , in_transaction(false)
{
    rio_readinitb( &m_fdbuf, m_client_fd );
    stack = ValueStack();
}

ClientConnection::~ClientConnection()
{
    // TODO: implement
    //close_connection(); 
    close(m_client_fd);
}

void ClientConnection::chat_with_client()
{
    Message request = read_message();
    if (request.get_message_type() != MessageType::LOGIN)
    {
        send_message(Message(MessageType::ERROR, {"First request must be LOGIN"}));
        return;
    }
    handle_login_request(request);

    while (true){
        request = read_message(); // Read next message
        if (request.get_message_type() == MessageType::BYE)
        {
            handle_bye_request();
            break; // Exit the loop if BYE message is received
        }
        handle_request(request);
    }
}

// TODO: additional member functions
Message ClientConnection::read_message() {
    char buf[Message::MAX_ENCODED_LEN];
    ssize_t result = rio_readlineb(&m_fdbuf, buf, sizeof(buf));
    if (result <= 0) {
        if (result == 0) {
            throw CommException("Client closed the connection");
        } else {
            throw CommException("Failed to read from client");
        }
    }
    Message msg;
    MessageSerialization::decode(std::string(buf), msg);
    if (!msg.is_valid()) {
        throw InvalidMessage("Received invalid message");
    }
    return msg;
}

void ClientConnection::send_message(const Message& msg) {
    std::string encoded_msg;
    MessageSerialization::encode(msg, encoded_msg);
    try {
        if (rio_writen(m_client_fd, encoded_msg.c_str(), encoded_msg.length()) < 0) {
            std::cerr << "Failed to send message to client. FD: " << m_client_fd << std::endl;
            throw CommException("Failed to send message to client");
        }
    } catch (const CommException& e) {
        std::cerr << "Communication error occurred: " << e.what() << std::endl;
    }
}

void ClientConnection::handle_request(const Message& request) {
    try {
        if (!request.is_valid()) {
            throw InvalidMessage("Invalid message format");
        }

        switch (request.get_message_type()) {
            case MessageType::LOGIN:
                handle_login_request(request);
                break;
            case MessageType::BEGIN:
                handle_begin_request();
                break;
            case MessageType::CREATE:
                handle_create_request(request);
                break;
            case MessageType::PUSH:
                handle_push_request(request);
                break;
            case MessageType::POP:
                handle_pop_request();
                break;
            case MessageType::TOP:
                handle_top_request();
                break;
            case MessageType::SET:
                handle_set_request(request);
                break;
            case MessageType::GET:
                handle_get_request(request);
                break;
            case MessageType::ADD:
            case MessageType::MUL:
            case MessageType::SUB:
            case MessageType::DIV:
                handle_arithmetic_request(request);
                break;
            case MessageType::COMMIT:
                handle_commit_request();
                break;
            case MessageType::BYE:
                handle_bye_request();
                break;
            default:
                throw InvalidMessage("Unsupported operation");
        }
    } catch (const InvalidMessage& e) {
        send_message(Message(MessageType::ERROR, {e.what()}));
    } catch (const OperationException& e) {
        send_message(Message(MessageType::FAILED, {e.what()})); // FAILED does not end the session
    } catch (const FailedTransaction& e) {
        rollback_transaction();
        send_message(Message(MessageType::FAILED, {e.what()})); // Likewise, FAILED does not end the session
    } catch (const CommException& e) {
        // Communication errors should generally close the connection
        send_message(Message(MessageType::ERROR, {"Communication failure"}));
    }
}

void ClientConnection::handle_login_request(const Message& request) {
    std::string username = request.get_username();
    if (username.empty()) {
        throw InvalidMessage("Invalid login format");
    }
    if (!is_valid_username(username)) {
        throw InvalidMessage("Invalid username");
    }
    send_message(Message(MessageType::OK));
}

bool ClientConnection::is_valid_username(const std::string& username) {
    if (username.empty() || !isalpha(username[0])) {
        return false;
    }

    for (char ch : username) {
        if (!(isalnum(ch) || ch == '_')) {
            return false;
        }
    }

    return true;
}

void ClientConnection::handle_create_request(const Message& request) {
    std::string table_name = request.get_table();
    try {
        m_server->create_table(table_name);
        send_message(Message(MessageType::OK));
    } catch (const OperationException& e) {
        throw OperationException(e.what());
    }
}

void ClientConnection::handle_begin_request() {
    in_transaction = true;
    begin_transaction();
    send_message(Message(MessageType::OK));
}

void ClientConnection::handle_commit_request() {
    commit_transaction();
    send_message(Message(MessageType::OK));
}

void ClientConnection::handle_rollback_request() {
    rollback_transaction();
    send_message(Message(MessageType::OK));
}

void ClientConnection::handle_set_request(const Message& request) {
    Table* table = m_server->find_table(request.get_table());

    std::string table_name = request.get_table();
    std::string key_name = request.get_key();
    if (!is_valid_username(table_name) || !is_valid_username(key_name)) {
        throw InvalidMessage("Invalid table or key name");
    }

    if (!table) {
        throw FailedTransaction("Unknown table");
    }

    if (stack.is_empty()) {
        throw OperationException("Stack is empty");
    }

    lock_table(table); // Lock the table
    std::string value = stack.get_top();
    stack.pop();

    
    table->set(key_name, value);

    if (!in_transaction) {
        table->commit_changes();
        table->unlock();
        locked_tables.erase(table);
    }
    send_message(Message(MessageType::OK)); // Send confirmation regardless of transaction state
}


void ClientConnection::handle_get_request(const Message& request) {
    // Assume 'table_name' is validated before being used to find the table.
    std::string table_name = request.get_table();
    std::string key_name = request.get_key();

    // Validate the table and key names.
    if (!is_valid_username(table_name) || !is_valid_username(key_name)) {
        throw InvalidMessage("Invalid table or key name");
    }

    // Try to find the table.
    Table* table = m_server->find_table(table_name);
    if (!table) {
        throw OperationException("Unknown table");
    }

    lock_table(table);
        // Get the value from the table.
        std::string value = table->get(key_name);
        stack.push(value); // Push the retrieved value onto the stack.

        // If we are not in a transaction, we can unlock the table right away.
        if (!in_transaction) {
            table->unlock();
            locked_tables.erase(table);
        }

        // Respond with OK to indicate a successful operation.
        send_message(Message(MessageType::OK));
    
}


void ClientConnection::handle_push_request(const Message& request) {
    try {
        // Make sure there is at least one argument to push
        if (request.get_arg(0).empty()) {  // Using get_arg(index) which is assumed to exist and return the argument at index 0
            send_message(Message(MessageType::ERROR, {"No value provided to push"}));
            return;
        }
        std::string value = request.get_arg(0); // Get the first argument for pushing onto the stack
        stack.push(value);
        send_message(Message(MessageType::OK));
    } catch (const std::exception& e) {
        send_message(Message(MessageType::ERROR, {e.what()}));
    }
}

void ClientConnection::handle_pop_request() {
    stack.pop();
    send_message(Message(MessageType::OK));
}

void ClientConnection::handle_top_request() {
    try {
        if (stack.is_empty()) {  // Check if the stack is empty before accessing
            send_message(Message(MessageType::FAILED, {"Error: empty stack"}));
            return;
        }
        std::string topValue = stack.get_top(); // Access the top value safely
        send_message(Message(MessageType::DATA, {topValue}));
    } catch (const std::exception& e) {
        send_message(Message(MessageType::ERROR, {e.what()}));
    }
}

void ClientConnection::handle_arithmetic_request(const Message& request) {
        // Attempt to pop twice. This assumes stack throws an exception if underflow occurs.
        int right, left;

        if (stack.is_empty()) {
            throw OperationException("Insufficient operands");
        }

        try {
            right = std::stoi(stack.get_top());
        } catch (const std::invalid_argument&) { // Catch std::invalid_argument
            throw OperationException("non-numeric operands");
        }
        stack.pop();

        if (stack.is_empty()) {
            throw OperationException("Insufficient operands");
        }
        try {
            left = std::stoi(stack.get_top());
        } catch (const std::invalid_argument&) { // Catch std::invalid_argument
            throw OperationException("non-numeric operands");
        }
        stack.pop();

        // Perform arithmetic operations
        int result = 0;
        switch (request.get_message_type()) {
            case MessageType::ADD:
                result = left + right;
                break;
            case MessageType::SUB:
                result = left - right;
                break;
            case MessageType::MUL:
                result = left * right;
                break;
            case MessageType::DIV:
                if (right == 0) {
                    throw OperationException("Division by zero");
                    return;
                }
                result = left / right;
                break;
            default:
                return;
        }
        stack.push(std::to_string(result));
        send_message(Message(MessageType::OK));
}

void ClientConnection::handle_bye_request() {
    send_message(Message(MessageType::OK));
}
            
void ClientConnection::handle_exception(const std::exception& e) {
    // Log the error
    std::cerr << "Error during client communication: " << e.what() << std::endl;

    if (dynamic_cast<const OperationException*>(&e) || dynamic_cast<const FailedTransaction*>(&e)) {
        send_message(Message(MessageType::FAILED, {e.what()}));
    } else {
        send_message(Message(MessageType::ERROR, {e.what()}));
    }
}

void ClientConnection::begin_transaction() {
    in_transaction = true;
}

void ClientConnection::commit_transaction() {
    if (!in_transaction) {
        throw OperationException("No active transaction to commit.");
    }

    // Commit changes for all locked tables
    for (Table* table: locked_tables) {
        table->commit_changes();
    }
    unlock_all_locked_tables();
    in_transaction = false;
}

void ClientConnection::rollback_transaction() {
    if (!in_transaction) {
        throw FailedTransaction("Error: No active transaction to rollback");
    } 
    for (Table* table: locked_tables) {
        table->rollback_changes();
    }
    unlock_all_locked_tables();
    in_transaction = false;
}

void ClientConnection::lock_table(Table* table) {
    if (in_transaction) {
        if (locked_tables.find(table) != locked_tables.end()) {
            return;
        } 
        if (!table->trylock()) {
            throw FailedTransaction("Error: Table couldn't be locked");
        }
        locked_tables.insert(table);
    } else {
        table->lock();
        locked_tables.insert(table);
    }

}

void ClientConnection::unlock_all_locked_tables() {
  for (Table* table : locked_tables) {
      table->unlock();
  }
  locked_tables.clear();
}

