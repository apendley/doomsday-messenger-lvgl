#pragma once

#include <Arduino.h>
#include <CircularBuffer.hpp>
#include "Message.h"

class MessageHistory {
public:
    // Message history file format:
    // 0x0000: message version (1 byte)
    // 0x0001: message count (1 byte)
    // ----- message 0
    // 0x0002: message sender (1 byte)
    // 0x0003: message string length (1 byte)
    // 0x0004 to 0x0004 + <message string length>: message string (<message string length> bytes)
    // ----- message 1 ... <message count-1>
    // etc
    // -----
    static constexpr uint8_t currentVersion = 1;
    static constexpr int maxMessages = 10;

public:
    void addMessage(Message::Sender sender, const char* text) {
        Message msg(sender);
        msg.setText(text);
        messages.push(msg);
    }

    inline bool isEmpty() const{
        return messages.isEmpty();
    }

    inline size_t size() const {
        return messages.size();
    }

    Message getMessage(int index) {
        size_t count = size();

        if (index >= count) {
            return messages[count - 1];
        }

        return messages[index];
    }

    void clear() {
        messages.clear();
    }

private:
    CircularBuffer<Message, maxMessages> messages;
};