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

    stream << "\n";

    encoded_msg = stream.str();
    if (encoded_msg.length() > Message::MAX_ENCODED_LEN) {
        throw InvalidMessage("Error: encoded message exceeds maximum length.");
    }
}

void MessageSerialization::decode( const std::string &encoded_msg, Message &msg )
{
  // TODO: implement
    if (encoded_msg.empty() || encoded_msg.back() != '\n') {
        throw InvalidMessage("Error: encoded message does not terminate with new line.");
    }
    if (encoded_msg.length() > Message::MAX_ENCODED_LEN) {
        throw InvalidMessage("Error: encoded message exceeds maximum length.");
    }

    msg.clear_args();

    std::istringstream ss(encoded_msg);
    std::string messageTypeStr;
    ss >> messageTypeStr;
    MessageType type = stringToMessageType(messageTypeStr);
    msg.set_message_type(type);

    // Check for the start of a quoted text
    char nextChar = ss.peek();
    if (nextChar == ' ') ss.get(); // Skip space before the argument

    // Reading quoted text as a single argument
    if (ss.peek() == '\"') {
        std::string arg;
        std::getline(ss.ignore(), arg, '\"'); // Ignore the opening quote
        msg.push_arg(arg);
        std::getline(ss, arg); // Get rest of the line
    } else {
        std::string arg;
        while (ss >> arg) {
            msg.push_arg(arg);
        }
    }

    if (!msg.is_valid()) {
        throw InvalidMessage("Decoded message is invalid according to protocol specifications.");
    }
}
