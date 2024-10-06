#pragma once

#include <Arduino.h>
#include <RH_RF95.h>
#include <RHEncryptedDriver.h>
#include <RHReliableDatagram.h>
#include <Speck.h>
#include "Messenger.h"

class LoRaMessenger: public Messenger {
public:
    LoRaMessenger(uint8_t _myAddress, uint8_t _otherAddress);

    bool begin(const uint8_t (&pmk)[16]);

    virtual void updateRx() override;
    virtual bool txWait(const uint8_t* payload, uint8_t len) override;
    virtual void ping() override;
    virtual bool settingsChanged(const Settings& settings, uint8_t changeFlags) override;

    // Called when a ping is received
    void setPingCallback(void (*cb)()) {
        pingCallback = cb;
    }

    // Called when a payload is received.
    void setPayloadReceivedCallback(void (*cb)(const uint8_t* payload, uint32_t len)) {
        payloadReceivedCallback = cb;
    }

private:
    // 0xFF broadcasts to everyone instead of a specific sender.
    static constexpr uint8_t broadcastAddress = 0xFF;

    // Ping bytes
    static constexpr uint8_t pingByte1 = 0x31;
    static constexpr uint8_t pingByte2 = 0x41;

    void setPMK(const uint8_t (&pmk)[16]) {
        cipher.setKey(pmk, 16);
    }

private:
    const uint8_t resetPin;
    const float freq;

    RH_RF95 device;
    Speck cipher;
    RHEncryptedDriver driver;
    RHReliableDatagram manager;

    void (*payloadReceivedCallback)(const uint8_t* payload, uint32_t len);
    void (*pingCallback)();

    uint8_t otherAddress;
    uint8_t rxBuffer[Message::maxLength] = {0};
    uint8_t txBuffer[Message::maxLength] = {0};
};