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
            if (request.get_message_type() == MessageType::BYE) {
                send_message(Message(MessageType::OK));
                break;
            }
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
            case MessageType::LOGIN: {
                std::string username = request.get_username();
                if (!username.empty()) {
                    send_message(Message(MessageType::OK));
                } else {
                    send_message(Message(MessageType::ERROR, {"Invalid login format"}));
                }
                break;
            }
            case MessageType::BEGIN: {
                m_server->begin_transaction(m_client_fd);
                send_message(Message(MessageType::OK));
                break;
            }
            case MessageType::COMMIT: {
                m_server->commit_transaction(m_client_fd);
                send_message(Message(MessageType::OK));
                break;
            }
            case MessageType::SET: {
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
                break;
            }
            case MessageType::GET: {
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
                break;
            }
            default:
                send_message(Message(MessageType::ERROR, {"Unsupported operation"}));
                break;
            }
    } catch (const std::exception& e) {
        send_message(Message(MessageType::ERROR, {"Internal server error"}));
        m_server->rollback_transaction(m_client_fd);
    }
}


void ClientConnection::close_connection() {
    Close(m_client_fd);
    std::cout << "Connection with client " << m_client_fd << " closed." << std::endl;
}