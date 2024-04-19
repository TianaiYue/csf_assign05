#include <utility>
#include <sstream>
#include <cassert>
#include <map>
#include "exceptions.h"
#include "message_serialization.h"

std::string messageTypeToString(MessageType type) {
    switch (type) {
        case MessageType::LOGIN: return "LOGIN";
        case MessageType::CREATE: return "CREATE";
        case MessageType::PUSH: return "PUSH";
        case MessageType::POP: return "POP";
        case MessageType::TOP: return "TOP";
        case MessageType::SET: return "SET";
        case MessageType::GET: return "GET";
        case MessageType::ADD: return "ADD";
        case MessageType::SUB: return "SUB";
        case MessageType::MUL: return "MUL";
        case MessageType::DIV: return "DIV";
        case MessageType::BEGIN: return "BEGIN";
        case MessageType::COMMIT: return "COMMIT";
        case MessageType::BYE: return "BYE";
        case MessageType::OK: return "OK";
        case MessageType::FAILED: return "FAILED";
        case MessageType::ERROR: return "ERROR";
        case MessageType::DATA: return "DATA";
        case MessageType::NONE: return "NONE";
        default: return "UNKNOWN";
    }
}

MessageType stringToMessageType(const std::string& typeStr) {
    if (typeStr == "LOGIN") return MessageType::LOGIN;
    if (typeStr == "CREATE") return MessageType::CREATE;
    if (typeStr == "PUSH") return MessageType::PUSH;
    if (typeStr == "POP") return MessageType::POP;
    if (typeStr == "TOP") return MessageType::TOP;
    if (typeStr == "SET") return MessageType::SET;
    if (typeStr == "GET") return MessageType::GET;
    if (typeStr == "ADD") return MessageType::ADD;
    if (typeStr == "SUB") return MessageType::SUB;
    if (typeStr == "MUL") return MessageType::MUL;
    if (typeStr == "DIV") return MessageType::DIV;
    if (typeStr == "BEGIN") return MessageType::BEGIN;
    if (typeStr == "COMMIT") return MessageType::COMMIT;
    if (typeStr == "BYE") return MessageType::BYE;
    if (typeStr == "OK") return MessageType::OK;
    if (typeStr == "FAILED") return MessageType::FAILED;
    if (typeStr == "ERROR") return MessageType::ERROR;
    if (typeStr == "DATA") return MessageType::DATA;
    if (typeStr == "NONE") return MessageType::NONE;
    return MessageType::NONE;
}


void MessageSerialization::encode( const Message &msg, std::string &encoded_msg )
{
  // TODO: implement
  std::ostringstream stream;
    stream << messageTypeToString(msg.get_message_type());

    for (unsigned i = 0; i < msg.get_num_args(); i++){
      stream << " " << msg.get_arg(i);
    }

    stream << "\n";  // End with a newline character

    encoded_msg = stream.str();
    if (encoded_msg.length() > Message::MAX_ENCODED_LEN) {
        throw InvalidMessage("Encoded message exceeds maximum length.");
    }
}

void MessageSerialization::decode( const std::string &encoded_msg_, Message &msg )
{
  // TODO: implement
  if (encoded_msg_.empty() || encoded_msg_.back() != '\n') {
        throw InvalidMessage("Encoded message does not end with a newline character.");
    }

    msg.clear_args(); // Clear existing arguments

    std::istringstream iss(encoded_msg_);
    std::string messageTypeStr;
    iss >> messageTypeStr;
    MessageType type = stringToMessageType(messageTypeStr);
    msg.set_message_type(type);

    // Check for the start of a quoted text
    char nextChar = iss.peek();
    if (nextChar == ' ') iss.get(); // Skip initial space before the argument

    // Reading quoted text as a single argument
    if (iss.peek() == '\"') {
        std::string arg;
        std::getline(iss.ignore(), arg, '\"'); // Ignore the opening quote and read until the next quote
        msg.push_arg(arg);
        std::getline(iss, arg); // Consume rest of the line including ending quote and newline
    } else {
        std::string arg;
        while (iss >> arg) {
            msg.push_arg(arg);
        }
    }

    if (!msg.is_valid()) {
        throw InvalidMessage("Decoded message is invalid according to protocol specifications.");
    }
}
