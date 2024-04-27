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
        Message request;
        while ((request = read_message()), request.get_message_type() != MessageType::BYE) {
            handle_request(request);
        }
        handle_bye_request(); // Properly close the connection after BYE message.
    } catch (const std::exception &e) {
        handle_exception(e); // This now decides to close connection or not based on error type
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
    if (rio_writen(m_client_fd, encoded_msg.c_str(), encoded_msg.length()) < 0) {
        throw CommException("Failed to send message to client");
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
        close_connection();
    } catch (const OperationException& e) {
        send_message(Message(MessageType::FAILED, {e.what()}));
    } catch (const FailedTransaction& e) {
        send_message(Message(MessageType::FAILED, {e.what()}));
        // Optional: rollback any transaction if necessary
    } catch (const CommException& e) {
        send_message(Message(MessageType::ERROR, {"Communication failure"}));
        close_connection();
    }
}

void ClientConnection::handle_login_request(const Message& request) {
    std::string username = request.get_username();
    if (username.empty()) {
        throw InvalidMessage("Invalid login format");
    }
    send_message(Message(MessageType::OK));
}

void ClientConnection::handle_create_request(const Message& request) {
    std::string table_name = request.get_table();
    try {
        m_server->create_table(table_name);
        send_message(Message(MessageType::OK));
    } catch (const OperationException& e) {
        send_message(Message(MessageType::FAILED, {e.what()}));
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
    if (!table) {
        send_message(Message(MessageType::ERROR, {"Unknown table"}));
        return;
    }
    
    if (stack.is_empty()) {
        send_message(Message(MessageType::FAILED, {"Stack is empty"}));
        return;
    }

    std::string value = stack.get_top();
    stack.pop(); // Ensure the pop is done after accessing the top.

    table->lock();
    try {
        table->set(request.get_key(), value);
        table->unlock();
        send_message(Message(MessageType::OK));
    } catch (const std::exception& e) {
        table->unlock();
        send_message(Message(MessageType::FAILED, {e.what()}));
    }
}

void ClientConnection::handle_get_request(const Message& request) {
    Table* table = m_server->find_table(request.get_table());
    if (!table) {
        send_message(Message(MessageType::ERROR, {"Unknown table"}));
        return;
    }

    table->lock();
    try {
        std::string value = table->get(request.get_key());
        stack.push(value); // Ensure the value is pushed onto the stack
        table->unlock();
        send_message(Message(MessageType::OK));
    } catch (const std::exception& e) {
        table->unlock();
        send_message(Message(MessageType::FAILED, {e.what()}));
    }
}


void ClientConnection::handle_push_request(const Message& request) {
    stack.push(request.get_arg(0));
    send_message(Message(MessageType::OK));
}

void ClientConnection::handle_pop_request() {
    stack.pop();
    send_message(Message(MessageType::OK));
}

void ClientConnection::handle_top_request() {
    if (stack.is_empty()) {
        send_message(Message(MessageType::ERROR, {"Stack is empty"}));
    } else {
        Message response(Message(MessageType::DATA, {stack.get_top()}));
        send_message(response);
    }
}


void ClientConnection::handle_arithmetic_request(const Message& request) {
    try {
        // Attempt to pop twice. If stack is underflowed, an exception should be thrown.
        int right = std::stoi(stack.get_top());
        stack.pop();
        int left = std::stoi(stack.get_top());
        stack.pop();

        int result;
        switch (request.get_message_type()) {
            case MessageType::ADD:
                result = left + right;
                break;
            case MessageType::MUL:
                result = left * right;
                break;
            case MessageType::SUB:
                result = left - right;
                break;
            case MessageType::DIV:
                if (right == 0) {
                    send_message(Message(MessageType::ERROR, {"Division by zero"}));
                    return;
                }
                result = left / right;
                break;
            default:
                send_message(Message(MessageType::ERROR, {"Unsupported arithmetic operation"}));
                return;
        }
        send_message(Message(MessageType::DATA, {std::to_string(result)}));
    } catch (const std::exception& e) {
        // This catch block assumes your stack throws std::exception on errors like underflow.
        send_message(Message(MessageType::ERROR, {e.what()}));
    }
}


void ClientConnection::handle_bye_request() {
    send_message(Message(MessageType::OK));
    close_connection();
}
            
void ClientConnection::handle_exception(const std::exception& e) {
    if (dynamic_cast<const OperationException*>(&e) || dynamic_cast<const FailedTransaction*>(&e)) {
        // Log error but continue session
        std::cerr << "Recoverable error: " << e.what() << std::endl;
        send_message(Message(MessageType::FAILED, {e.what()}));
    } else {
        // Log error and terminate session
        std::cerr << "Unrecoverable error: " << e.what() << std::endl;
        send_message(Message(MessageType::ERROR, {e.what()}));
        close_connection();
    }
}

void ClientConnection::rollback_transaction() {
    m_server->rollback_transaction(m_client_fd);
    // Assuming unlock all tables logic is implemented within server's rollback
}

void ClientConnection::close_connection() {
    if (m_client_fd >= 0) {
        Close(m_client_fd);  // Close function wraps close system call, handles errors.
        m_client_fd = -1;    // Invalidate the descriptor.
        std::cout << "Connection with client closed." << std::endl;
    }
}