#pragma once

#include <Arduino.h>

struct MacAddress {
    static constexpr size_t addressLength = 6;

    MacAddress() {
        memset(rawAddress, 0, addressLength);
    }

    MacAddress(uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4, uint8_t b5) {
        rawAddress[0] = b0;
        rawAddress[1] = b1;
        rawAddress[2] = b2;
        rawAddress[3] = b3;
        rawAddress[4] = b4;
        rawAddress[5] = b5;
    }

    MacAddress(const uint8_t addr[addressLength]) {
        memcpy(rawAddress, addr, addressLength);
    }

    MacAddress(const MacAddress& other) {
        memcpy(rawAddress, other.rawAddress, addressLength);
    }

    MacAddress(MacAddress&& other) noexcept {
        memcpy(rawAddress, other.rawAddress, addressLength);
    }

    MacAddress& operator=(const MacAddress& other) {
        if (this != &other) {
            memcpy(rawAddress, other.rawAddress, addressLength);
        }
        return *this;
    }

    MacAddress& operator=(MacAddress&& other) noexcept {
        if (this != &other) {
            memcpy(rawAddress, other.rawAddress, addressLength);
        }
        return *this;
    }

    bool operator==(const MacAddress& other) const {
        return memcmp(rawAddress, other.rawAddress, addressLength) == 0;
    }

    bool operator!=(const MacAddress& other) const {
        return !(*this == other);
    }
    
    uint8_t rawAddress[addressLength];
};

