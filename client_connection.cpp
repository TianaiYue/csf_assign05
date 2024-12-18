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
{
    rio_readinitb( &m_fdbuf, m_client_fd );
    stack = ValueStack();
}

ClientConnection::~ClientConnection()
{
    // TODO: implement
    close_connection(); 
}

void ClientConnection::chat_with_client() {
    try {
        Message request = read_message();
        if (request.get_message_type() != MessageType::LOGIN) {
            send_message(Message(MessageType::ERROR, {"First request must be LOGIN"}));
            close_connection();
            return;
        }
        handle_login_request(request);
        while ((request = read_message()), request.get_message_type() != MessageType::BYE) {
            handle_request(request);
        }
        handle_bye_request();
    } catch (const CommException& e) {
        std::cerr << "Communication exception: " << e.what() << std::endl;
        handle_exception(e);
    } catch (const InvalidMessage& e) {
        std::cerr << "InvalidMessage exception: " << e.what() << std::endl;
        handle_exception(e);
    } catch (const OperationException& e) {
        std::cerr << "InvalidMessage exception: " << e.what() << std::endl;
        handle_exception(e);
    } catch (const std::exception& e) {
        std::cerr << "General exception: " << e.what() << std::endl;
        
        close_connection();
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
        // Optionally log the exception or handle it in other ways
        std::cerr << "Communication error occurred: " << e.what() << std::endl;
        // Decide whether to close the connection or attempt recovery
        close_connection();
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
        close_connection(); // ERROR should close the session
    } catch (const OperationException& e) {
        send_message(Message(MessageType::FAILED, {e.what()})); // FAILED does not end the session
    } catch (const FailedTransaction& e) {
        send_message(Message(MessageType::FAILED, {e.what()})); // Likewise, FAILED does not end the session
    } catch (const CommException& e) {
        // Communication errors should generally close the connection
        send_message(Message(MessageType::ERROR, {"Communication failure"}));
        close_connection();
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
        return false; // First character must be a letter
    }

    for (char ch : username) {
        if (!(isalnum(ch) || ch == '_')) {
            return false; // Only alphanumeric characters and underscores are allowed
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
    m_server->begin_transaction(m_client_fd);
    send_message(Message(MessageType::OK));
}

void ClientConnection::handle_commit_request() {
    m_server->commit_transaction(m_client_fd);
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
    
    // Attempt to lock the table.
    if (!m_server->lock_table(table_name, m_client_fd)) {
        table->lock();
    }
    
    try {
        auto table = m_server->find_table(table_name);
        std::string value = stack.get_top();
        stack.pop();
        table->set(key_name, value);
        if (!in_transaction) {
            table->commit_changes();
            table->unlock();
        }
        send_message(Message(MessageType::OK));
    } catch (...) {
        if (!m_server->is_transaction_active(m_client_fd || !in_transaction)) {
            // Only unlock if we're not in a transaction
            table->unlock();
        }
        throw;
    }
    if (!m_server->is_transaction_active(m_client_fd)) {
        // Unlock the table after a successful set in auto-commit mode
        table->unlock();
    }
}

void ClientConnection::handle_get_request(const Message& request) {
     std::string table_name = request.get_table();
    Table* table = m_server->find_table(request.get_table());
    if (!m_server->is_transaction_active(m_client_fd)) {
        // In auto-commit mode, lock the table for the duration of this operation
        table->lock();
    }


    try {
        auto table = m_server->find_table(table_name);
        std::string value = table->get(request.get_key());
        stack.push(value);
        send_message(Message(MessageType::OK));
    } catch (...) {
        if (!m_server->is_transaction_active(m_client_fd)) {
            // Only unlock if we're not in a transaction
            table->unlock();
        }
        if (!in_transaction) {
            m_server->unlock_table(table_name, m_client_fd);
        }
        throw;
    }
    
    if (!m_server->is_transaction_active(m_client_fd)) {
        // Unlock the table after a successful get in auto-commit mode
        table->unlock();
    }
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
    close_connection();
}
            
void ClientConnection::handle_exception(const std::exception& e) {
    // Log the error
    std::cerr << "Error during client communication: " << e.what() << std::endl;
    if (in_transaction) {
        rollback_transaction();
    }
    if (dynamic_cast<const OperationException*>(&e) || dynamic_cast<const FailedTransaction*>(&e)) {
        send_message(Message(MessageType::FAILED, {e.what()}));
    } else {
        send_message(Message(MessageType::ERROR, {e.what()}));
        close_connection();
    }
}


void ClientConnection::rollback_transaction() {
    if (in_transaction) {
        m_server->rollback_transaction(m_client_fd);
        send_message(Message(MessageType::FAILED, {"Transaction rolled back due to failure"}));
        in_transaction = false; // Reset the transaction flag
    }
}

void ClientConnection::close_connection() {
    if (m_client_fd >= 0) {
        Close(m_client_fd);  // Close function wraps close system call, handles errors.
        m_client_fd = -1;    // Invalidate the descriptor.
        std::cout << "Connection with client closed." << std::endl;
    }
}
