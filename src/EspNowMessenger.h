#pragma once

#include <Arduino.h>
#include <esp_now.h>
#include "MacAddress.h"
#include "Messenger.h"
#include <CircularBuffer.hpp>

class EspNowMessenger: public Messenger {
public:
    EspNowMessenger();
    ~EspNowMessenger();

    bool begin(const MacAddress& _otherAddress, const uint8_t (&pmk)[16], const uint8_t (&lmk)[16]);

    virtual void updateRx() override;
    virtual bool txWait(const uint8_t* payload, uint8_t len) override;
    virtual void ping() override;
    virtual bool settingsChanged(const Settings& settings, uint8_t changeFlags) override;    

    // Called when a payload is received.
    void setPayloadReceivedCallback(void (*cb)(const uint8_t* payload, uint32_t len)) {
        payloadReceivedCallback = cb;
    }

    // Called when a ping is received
    void setPingCallback(void (*cb)()) {
        pingCallback = cb;
    } 

private:
    // Callbacks from ESP-NOW
    static void dataReceived(const uint8_t* mac, const uint8_t* incomingData, int len);
    static void dataSent(const uint8_t *mac, esp_now_send_status_t status);

    // Have we seen a packet with this ID recently?
    bool isPacketIdentifierRecognized(uint16_t identifier);

    // Set primary encryption key. (used by settingsChanged())
    bool setPMK(const uint8_t (&pmk)[16]);

    // Set the peer (used by settingsChanged())
    // We need both a mac and an LMK to set the peer.
    bool setPeer(const MacAddress& mac, const uint8_t (&lmk)[16]);    

    // Delete all known peers.
    void deleteAllPeers();

private:
    enum class SendStatus: uint8_t {
        none,
        sending,
        waitingForACK,
        success,
        failure,
    };

    enum class PacketType: uint8_t {
        message = 0x15,
        ack = 0x84,
        ping = 0xB1,
    };    

    struct PacketHeader {
        PacketType packetType = PacketType::message;
        uint16_t packetIdentifier = 0;
        uint8_t payloadSize = 0;
        uint16_t checksum = 0; 
    } __attribute__((packed));

    struct PacketIdentifierMemo {
        uint16_t packetIdentifier;
        uint32_t timestamp;
    } __attribute__((packed));;

    static constexpr uint32_t maxSendRetries = 3;
    static constexpr uint32_t sendTimeout = 500;

    // The mac address of the paired device.
    MacAddress otherAddress;

    // When a packet has been received, this will be set to non-zero,
    // signalling that the packet is ready to be processed.
    volatile uint8_t receivedByteCount = 0;  

    // Address that the last message was received from
    MacAddress rxAddress;

    // Call this to forward received payload to the application.
    void (*payloadReceivedCallback)(const uint8_t* payload, uint32_t len) = nullptr;

    // Call this callback to forward a ping reception to the application.
    void (*pingCallback)() = nullptr;

    // Status of the current send operation
    volatile SendStatus sendStatus = SendStatus::none;
    PacketType sendPacketType = PacketType::message;

    // Send and receive buffers.
    // Message::maxLength is 239, and the max ESP-NOW packet size is 250,
    // so we can easily inject our packet headers into the messages.
    uint8_t txBuffer[Message::maxLength + sizeof(PacketHeader)] = {0};
    uint8_t rxBuffer[Message::maxLength + sizeof(PacketHeader)] = {0};

    // Increment this for each message packet sent.
    uint16_t nextPacketIdentifier = 0;

    // Avoid processing the same packet multiple times in less-than-ideal conditions.
    static constexpr size_t maxPacketIdentifierMemos = 16;
    static constexpr uint32_t memoLifetime = 30 * 1000;
    CircularBuffer<PacketIdentifierMemo, maxPacketIdentifierMemos> packetIdentifierMemos;
};
