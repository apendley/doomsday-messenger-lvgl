#pragma once

#include <Arduino.h>

struct Message {
    // Regardless of what the RF95 comments say about RH_RF95_MAX_MESSAGE_LEN, any attempt to send more than 239 bytes fails.
    // ESP-NOW has a max packet size of 250 bytes.
    // For simplicity we'll use the lowest common demoninator and limit message size to 239 bytes,
    // but eventually we could upgrade the message system to allow for variable-length message buffers.
    static constexpr uint32_t maxLength = 239;

    // Add space for a terminating null character so we can use this with string-related functions.
    static constexpr uint32_t bufferSize = maxLength + 1;

    // Message sender identification.
    enum class Sender: uint8_t {
        me,
        them
    };

    Message() {
        text[0] = 0;
    }

    Message(Sender s) : sender(s) { 
        text[0] = 0;
    }

    // Expects a null terminated string. If string length exceeds Message::maxLength it will be truncated
    void setText(const char* str) {
        const size_t strBufferSize = min(bufferSize, strlen(str) + 1);
        strncpy(text, str, bufferSize); 
        text[strBufferSize - 1] = 0;
    }

    // Pass the string and the string length. Will automatically null-terminate.
    void setText(const char* str, size_t strLen) {
        const size_t strBufferSize = min(bufferSize, strLen + 1);
        strncpy(text, str, bufferSize); 
        text[strBufferSize - 1] = 0;
    }    

    // Who sent the message?
    Sender sender = Sender::me;

    // Message storage.
    char text[bufferSize];
} __attribute__((packed));
