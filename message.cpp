/*
 * A5MS1
 * The Message class
 * Cassie Zhang xzhan304
 * Tianai Yue tyue4
 */

#include <set>
#include <map>
#include <regex>
#include <cassert>
#include "message.h"

Message::Message()
  : m_message_type(MessageType::NONE)
{
}

Message::Message( MessageType message_type, std::initializer_list<std::string> args )
  : m_message_type( message_type )
  , m_args( args )
{
}

Message::Message( const Message &other )
  : m_message_type( other.m_message_type )
  , m_args( other.m_args )
{
}

Message::~Message()
{
}

Message &Message::operator=( const Message &rhs )
{
  // TODO: implement
  if (this != &rhs) {
    m_message_type = rhs.m_message_type;
    m_args = rhs.m_args;
  }
  return *this;
}

MessageType Message::get_message_type() const
{
  return m_message_type;
}

void Message::set_message_type(MessageType message_type)
{
  m_message_type = message_type;
}

std::string Message::get_username() const
{
  // TODO: implement
  if (m_message_type == MessageType::LOGIN && !m_args.empty()) {
    return m_args[0];
  }
  return "";
}

std::string Message::get_table() const
{
  // TODO: implement
  if ((m_message_type == MessageType::CREATE || m_message_type == MessageType::SET || m_message_type == MessageType::GET) && m_args.size() >= 1) {
    return m_args[0];
  }
  return "";
}

std::string Message::get_key() const
{
  // TODO: implement
  if ((m_message_type == MessageType::SET || m_message_type == MessageType::GET) && m_args.size() >= 2) {
    return m_args[1];
  }
  return "";
}

std::string Message::get_value() const
{
  // TODO: implement
  if ((m_message_type == MessageType::DATA || m_message_type == MessageType::PUSH) && !m_args.empty()) {
    return m_args[0];
  }
  return "";
}

std::string Message::get_quoted_text() const
{
  // TODO: implement
  if ((m_message_type == MessageType::FAILED || m_message_type == MessageType::ERROR) && !m_args.empty()) {
    return m_args[0];
  }
  return "";
}

void Message::push_arg( const std::string &arg )
{
  m_args.push_back( arg );
}

void Message::clear_args() 
{
    m_args.clear();
}

bool Message::is_valid() const
{
  // TODO: implement
  std::regex identifier_regex("^[a-zA-Z][a-zA-Z0-9_]*$");
  switch (m_message_type) {
    case MessageType::LOGIN:
    case MessageType::PUSH:
      return m_args.size() == 1;  
    case MessageType::CREATE:
      return m_args.size() == 1 && std::regex_match(m_args[0], identifier_regex);
    case MessageType::SET:
    case MessageType::GET:
      if (m_args.size() == 1 && m_message_type == MessageType::CREATE) {
        return std::regex_match(m_args[0], identifier_regex);
      } else if (m_args.size() == 2) {
        return std::regex_match(m_args[0], identifier_regex) && !m_args[1].empty();
      }
      return false;
    case MessageType::DATA:
      return m_args.size() == 1 && !m_args[0].empty();
    case MessageType::FAILED:
    case MessageType::ERROR:
      return !m_args.empty() && !m_args[0].empty();
    default:
      return true;
  }
  return false;
}

