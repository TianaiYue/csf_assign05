/*
 * A5MS1
 * The Message Serialization function
 * Cassie Zhang xzhan304
 * Tianai Yue tyue4
 */

#include <utility>
#include <sstream>
#include <cassert>
#include <map>
#include <string>
#include "exceptions.h"
#include "message_serialization.h"

const std::map<MessageType, std::string> MessageSerialization::typeToStringMap = {
    {MessageType::LOGIN, "LOGIN"},
    {MessageType::CREATE, "CREATE"},
    {MessageType::PUSH, "PUSH"},
    {MessageType::POP, "POP"},
    {MessageType::TOP, "TOP"},
    {MessageType::SET, "SET"},
    {MessageType::GET, "GET"},
    {MessageType::ADD, "ADD"},
    {MessageType::SUB, "SUB"},
    {MessageType::MUL, "MUL"},
    {MessageType::DIV, "DIV"},
    {MessageType::BEGIN, "BEGIN"},
    {MessageType::COMMIT, "COMMIT"},
    {MessageType::BYE, "BYE"},
    {MessageType::OK, "OK"},
    {MessageType::FAILED, "FAILED"},
    {MessageType::ERROR, "ERROR"},
    {MessageType::DATA, "DATA"}
};

const std::map<std::string, MessageType> MessageSerialization::stringToTypeMap = {
    {"LOGIN", MessageType::LOGIN},
    {"CREATE", MessageType::CREATE},
    {"PUSH", MessageType::PUSH},
    {"POP", MessageType::POP},
    {"TOP", MessageType::TOP},
    {"SET", MessageType::SET},
    {"GET", MessageType::GET},
    {"ADD", MessageType::ADD},
    {"SUB", MessageType::SUB},
    {"MUL", MessageType::MUL},
    {"DIV", MessageType::DIV},
    {"BEGIN", MessageType::BEGIN},
    {"COMMIT", MessageType::COMMIT},
    {"BYE", MessageType::BYE},
    {"OK", MessageType::OK},
    {"FAILED", MessageType::FAILED},
    {"ERROR", MessageType::ERROR},
    {"DATA", MessageType::DATA},
    {"NONE", MessageType::NONE}
};

std::string MessageSerialization::messageTypeToString(MessageType type) {
    auto it = typeToStringMap.find(type);
    if (it != typeToStringMap.end()) {
        return it->second;
    }
    return "NONE";
}

MessageType MessageSerialization::stringToMessageType(const std::string& typeStr) {
    auto it = stringToTypeMap.find(typeStr);
    if (it != stringToTypeMap.end()) {
        return it->second;
    }
    return MessageType::NONE;
}

void MessageSerialization::encode( const Message &msg, std::string &encoded_msg )
{
    // implement
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
    // implement
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

    // Check for start of a quoted text
    if (ss.peek() == ' ') {
        ss.get(); // Skip space before argument
    }

    if (ss.peek() == '\"') {
        std::string arg;
        std::getline(ss.ignore(), arg, '\"'); // Ignore opening quote
        msg.push_arg(arg);
        std::getline(ss, arg); // Get rest of line
    } else {
        std::string arg;
        while (ss >> arg) {
            msg.push_arg(arg);
        }
    }

    if (!msg.is_valid()) {
        throw InvalidMessage("Decoded message is invalid.");
    }
}
