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

void ClientConnection::chat_with_client()
{
    // TODO: implement
    try {
        while (true) {
            Message request = read_message();
            handle_request(request);
        }
    } catch (const std::exception &e) {
        std::cerr << "Exception in client connection: " << e.what() << std::endl;
        send_message(Message(MessageType::ERROR, {"Internal server error"})); 
    }
    close_connection();
}

// TODO: additional member functions
Message ClientConnection::read_message() {
    char buf[Message::MAX_ENCODED_LEN];
    if (rio_readlineb(&m_fdbuf, buf, sizeof(buf)) <= 0) {
        throw CommException("Failed to read from client");
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
    if (!request.is_valid()) {
        send_message(Message(MessageType::ERROR, {"Invalid message format"}));
        return;
    }
    
    try {
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
                send_message(Message(MessageType::ERROR, {"Unsupported operation"}));
                break;
        }
    } catch (const std::exception& e) {
        handle_exception(e);
    }
}

void ClientConnection::handle_login_request(const Message& request) {
    std::string username = request.get_username();
    if (!username.empty()) {
        send_message(Message(MessageType::OK));
    } else {
        send_message(Message(MessageType::ERROR, {"Invalid login format"}));
    }
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

    table->lock();
    table->set(request.get_key(), request.get_value());
    table->commit_changes();
    table->unlock();
    send_message(Message(MessageType::OK));
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
        table->unlock();
        send_message(Message(MessageType::DATA, {value}));
    } catch (const OperationException& e) {
        table->unlock();
        send_message(Message(MessageType::ERROR, {e.what()}));
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
    // Create a Message object with the required type and data
    Message response(MessageType::DATA, {stack.get_top()});
    // Pass the created Message object to the send_message function
    send_message(response);
}


void ClientConnection::handle_arithmetic_request(const Message& request) {
    try {
        int left = std::stoi(stack.get_top());
        int right = std::stoi(stack.get_top());
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
    } catch (const std::invalid_argument& e) {
        send_message(Message(MessageType::ERROR, {"Invalid operands"}));
    } catch (const std::out_of_range& e) {
        send_message(Message(MessageType::ERROR, {"Operand out of range"}));
    }
}

void ClientConnection::handle_bye_request() {
    send_message(Message(MessageType::OK));
    return;
}
            
void ClientConnection::handle_exception(const std::exception& e) {
    std::cerr << "Exception in client connection: " << e.what() << std::endl;
    send_message(Message(MessageType::ERROR, {"Internal server error"}));
    m_server->rollback_transaction(m_client_fd);
}

void ClientConnection::close_connection()
{
    Close(m_client_fd);
    std::cout << "Connection with client " << m_client_fd << " closed." << std::endl;
}