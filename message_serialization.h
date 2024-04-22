#ifndef MESSAGE_SERIALIZATION_H
#define MESSAGE_SERIALIZATION_H

#include "message.h"
#include <map>

namespace MessageSerialization {
    extern const std::map<MessageType, std::string> typeToStringMap;
    extern const std::map<std::string, MessageType> stringToTypeMap;

    std::string messageTypeToString(MessageType type);
    MessageType stringToMessageType(const std::string& typeStr);
    
    void encode(const Message &msg, std::string &encoded_msg);
    void decode(const std::string &encoded_msg, Message &msg);
};

#endif // MESSAGE_SERIALIZATION_H
