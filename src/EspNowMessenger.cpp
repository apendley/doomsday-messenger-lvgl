#include "EspNowMessenger.h"
#include <WiFi.h>
#include <esp_crc.h>
#include "config.h"

// #define LOGGER Serial
#include "Logger.h"

namespace {
    // Hack. The first created instance of EspNowManager will set this pointer, so it can receive data from static callbacks.
    // This only works properly if there's only one instance of EspNowManager, which should always be the case anyway.
    EspNowMessenger* pInstance = nullptr;
}

EspNowMessenger::EspNowMessenger() { 
    assert(pInstance == nullptr);

    if (pInstance == nullptr) {
        pInstance = this;
    }
}

EspNowMessenger::~EspNowMessenger() { 
    if (pInstance == this) {
        pInstance = nullptr;
    }
}

bool EspNowMessenger::begin(const MacAddress& _otherAddress, const uint8_t (&pmk)[16], const uint8_t (&lmk)[16]) {
    WiFi.mode(WIFI_MODE_STA);
    WiFi.channel(1);

    esp_err_t initResult = esp_now_init();
    if (initResult != ESP_OK) {
        LOGFMT("Failed to initialize ESP-NOW, error code: %02X\n", initResult);
        return false;
    }

    // Set up primary encryption key
    if (!setPMK(pmk)) {
        return false;
    }

    // Set up peer
    if (!setPeer(_otherAddress, lmk)) {
        return false;
    }

    // Register rx and tx callbacks.
    esp_now_register_recv_cb(dataReceived);    
    esp_now_register_send_cb(dataSent);

    // Randomize; if we always start at 1, then if you turn off a device and turn
    // it back on, there's an increased chance the new messages will be seen as
    // duplicate sends.
    nextPacketIdentifier = random(10000, 20000);
    
    return true;
}

void EspNowMessenger::updateRx() {
    if (receivedByteCount == 0) {
        return;
    }

    // First let's read the packet header in and figure out what kind of message this is.
    // Let's make sure we received enough bytes first.
    if (receivedByteCount < sizeof(PacketHeader)) {
        LOGLN("Received packet is too small.");
        receivedByteCount = 0;
        return;
    }

    PacketHeader hdr;
    memcpy(&hdr, rxBuffer, sizeof(PacketHeader));

    if (hdr.packetType == PacketType::ping) {
        if (pingCallback) {
            pingCallback();
        }

        receivedByteCount = 0;
        return;
    }

    if (hdr.packetType == PacketType::ack) {
        // This should not be happening.
        LOGLN("Received unexpected Ack in updateRx()");
        receivedByteCount = 0;
    }

    // Only remaining packet type is PacketType::message.
    // Send an acknowledgement, and then process the message.
    LOGFMT("Message packet received (id: %d), sending ack...\n", hdr.packetIdentifier);

    // Size of packet is not correct. Ignore it.
    if (receivedByteCount != sizeof(PacketHeader) + hdr.payloadSize) {
        LOGLN("Incorrect message length reported.");
        receivedByteCount = 0;
        return;
    }

    // Validate checksum
    uint16_t crc = esp_rom_crc16_le(0xFFFF, &rxBuffer[sizeof(PacketHeader)], hdr.payloadSize);

    if (crc != hdr.checksum) {
        LOGFMT("Checksums do not match. Packet: %d, calculated: %d\n", hdr.checksum, crc);
        receivedByteCount = 0;
        return;        
    }

    // Prepare the ack header and copy to our send buffer.
    PacketHeader ackHeader;
    ackHeader.packetType = PacketType::ack;
    ackHeader.packetIdentifier = hdr.packetIdentifier;
    ackHeader.payloadSize = 0;
    ackHeader.checksum = 0;
    memcpy(txBuffer, &ackHeader, sizeof(ackHeader));

    sendStatus = SendStatus::sending;
    sendPacketType = PacketType::ack;
    esp_err_t result = esp_now_send(rxAddress.rawAddress, txBuffer, sizeof(PacketHeader));

    if (result != ESP_OK) {
        LOGFMT("ESP-NOW send failed sending Ack, error code: %02X\n", result);
    }
    else {
        while(sendStatus == SendStatus::sending) {
            yield();
        }

        if (sendStatus == SendStatus::success) {
            LOGLN("Ack send succeeded");
        }
        else {
            LOGLN("Ack send failed");
        }
    }

    sendStatus = SendStatus::none;

    if (isPacketIdentifierRecognized(hdr.packetIdentifier)) {
        LOGFMT("Received packet with recognized ID, ignoring");
    }
    else if (payloadReceivedCallback) {
        payloadReceivedCallback(&rxBuffer[sizeof(PacketHeader)], hdr.payloadSize);

        PacketIdentifierMemo memo;
        memo.packetIdentifier = hdr.packetIdentifier;
        memo.timestamp = millis();
        packetIdentifierMemos.push(memo);        
    }

    receivedByteCount = 0;
}

bool EspNowMessenger::txWait(const uint8_t* payload, uint8_t len) {
    if (len > Message::maxLength) {
        // Message length is too long.
        return false;
    }

    // Get the next packet identifier.
    nextPacketIdentifier++;
    if (nextPacketIdentifier == 0) {
        nextPacketIdentifier = 1;
    }

    uint16_t packetIdentifier = nextPacketIdentifier;

    // Prepare the header and copy to our send buffer.
    PacketHeader header;
    header.packetType = PacketType::message;
    header.packetIdentifier = nextPacketIdentifier;
    header.payloadSize = len;
    header.checksum = esp_rom_crc16_le(0xFFFF, payload, len);
    memcpy(txBuffer, &header, sizeof(header));

    // Copy payload to the buffer.
    memcpy(&txBuffer[sizeof(header)], payload, len);

    uint8_t retryCount = 0;
    while (retryCount <= maxSendRetries) {
        sendStatus = SendStatus::sending;
        sendPacketType = PacketType::message;

        LOGFMT("Send attempt %d, packet ID: %d\n", retryCount + 1, packetIdentifier);

        esp_err_t result = esp_now_send(otherAddress.rawAddress, txBuffer, sizeof(PacketHeader) + len);

        if (result != ESP_OK) {
            LOGFMT("ESP-NOW send failed, error code: %02X\n", result);
            sendStatus = SendStatus::none;
            return false;
        }

        while(sendStatus == SendStatus::sending) {
            yield();
        }
        
        // At this point, our sendStatus should either be ::failure or ::waitForAck.
        // If it's failure, we'll just give up now. This means the other device
        // was not detected and we know we can't send a message to it right now.
        if (sendStatus == SendStatus::failure) {
            LOGFMT("ESP-NOW send failed: paired device not detected.");    
            sendStatus = SendStatus::none;
            return false;
        }

        // Now we wait for our acknowledgment
        if (sendStatus != SendStatus::waitingForACK) {
            // Any other status is an anomaly and should be considered invalid.
            LOGFMT("ESP-NOW send failed: unexpected status after sending.");    
            sendStatus = SendStatus::none;
            return false;
        }

        LOGLN("Send success, waiting for ack...");

        uint32_t timeoutStart = millis();

        while (millis() - timeoutStart < sendTimeout) {
            // We got a packet, was it the one we're looking for?
            // Gotta be at least the size of a header. If not, there's a problem; ignore this packet.
            if (receivedByteCount >= sizeof(PacketHeader)) {
                PacketHeader ackHeader;
                memcpy(&ackHeader, rxBuffer, sizeof(ackHeader));

                if (ackHeader.packetType == PacketType::ack && ackHeader.packetIdentifier == packetIdentifier) {
                    LOGLN("Ack received!");
                    // This was the ack we're looking for! We're done.
                    receivedByteCount = 0;
                    return true;
                }
            }
            else {
                // Reset so we can receive another packet.
                receivedByteCount = 0;
            }

            yield();            
        }

        LOGLN("Wait for ack timed out");
        retryCount++;
        timeoutStart = millis();
    }

    LOGLN("Wait for ack failed. Aborting send attempt.");

    // If we get here, we failed to send the message and receive an acknowledgement.
    receivedByteCount = 0;
    return false;
}


void EspNowMessenger::dataReceived(const uint8_t* mac, const uint8_t* incomingData, int len) {
    if (pInstance == nullptr) {
        return;
    }

    // This message is not from our paired device, ignore it.
    if (memcmp(mac, pInstance->otherAddress.rawAddress, 6) != 0) {
        LOGLN("Message received from unknown device. Ignoring.");
        return;
    }

    // We should really be putting these received packets on a queue, instead of just assuming we'll only
    // ever get one packet at a time and will only need the one rx buffer. As it is right now, if we receive
    // a second packet before we've been able to process the first, the second packet will be ignored.    
    if (pInstance->receivedByteCount > 0) {
        LOGLN("Messenger not ready, discarding received payload.");
        return;
    }

    LOGFMT("Received payload, length: %d\n", len);
    memcpy(pInstance->rxBuffer, incomingData, len);
    memcpy(pInstance->rxAddress.rawAddress, mac, 6);
    pInstance->receivedByteCount = len;
}
void EspNowMessenger::dataSent(const uint8_t *mac, esp_now_send_status_t status) {
    if (pInstance == nullptr) {
        return;
    }

    switch (pInstance->sendPacketType) {
        case PacketType::message:
            LOGFMT("Message packet send result: %s\n", (status == ESP_NOW_SEND_SUCCESS) ? "success" : "failure");
            pInstance->sendStatus = (status == ESP_NOW_SEND_SUCCESS) ? SendStatus::waitingForACK : SendStatus::failure;
            break;

        case PacketType::ack:
            LOGFMT("Ack packet send result: %s\n", (status == ESP_NOW_SEND_SUCCESS) ? "success" : "failure");
            pInstance->sendStatus = (status == ESP_NOW_SEND_SUCCESS) ? SendStatus::success : SendStatus::failure;
            break;

        case PacketType::ping:
            LOGFMT("Ping packet send result: %s\n", (status == ESP_NOW_SEND_SUCCESS) ? "success" : "failure");
            pInstance->sendStatus = (status == ESP_NOW_SEND_SUCCESS) ? SendStatus::success : SendStatus::failure;        
            break;
    }
}

bool EspNowMessenger::isPacketIdentifierRecognized(uint16_t id) {
    uint32_t now = millis();

    for (int i = 0; i < maxPacketIdentifierMemos; i++) {
        uint32_t age = now - packetIdentifierMemos[i].timestamp;
        if (age < memoLifetime && id == packetIdentifierMemos[i].packetIdentifier) {
            return true;
        }
    }

    return false;
}

void EspNowMessenger::ping() {
    PacketHeader pingHeader;
    pingHeader.packetType = PacketType::ping;

    LOGLN("Sending Ping");
    sendStatus = SendStatus::sending;
    sendPacketType = PacketType::ping;
    esp_err_t result = esp_now_send(otherAddress.rawAddress, (uint8_t*)&pingHeader, sizeof(PacketHeader));

    if (result != ESP_OK) {
        LOGLN("Failed to send ping");
    }
    else {
        while(sendStatus == SendStatus::sending) {
            yield();
        }

        if (sendStatus == SendStatus::success) {
            LOGLN("Ping succeeded");
        }
        else {
            LOGLN("Ping failed");
        }
    }

    // We don't wait for an Ack for pings.
}

bool EspNowMessenger::setPMK(const uint8_t (&pmk)[16]) {
    uint8_t pmkBuffer[17];
    const char pmkBufferSize = sizeof(pmkBuffer);
    memcpy(pmkBuffer, pmk, pmkBufferSize - 1);
    pmkBuffer[pmkBufferSize - 1] = 0;


    esp_err_t setPmkResult = esp_now_set_pmk(pmkBuffer);
    if (setPmkResult != ERR_OK) {
        LOGLN("Failed to ESP-NOW pmk encryption key");
        return false;
    }

    return true;
}

bool EspNowMessenger::setPeer(const MacAddress& mac, const uint8_t (&lmk)[16]) {
    deleteAllPeers();

    otherAddress = mac;

    LOGFMT("new Peer address: %02X:%02X:%02X:%02X:%02X:%02X\n", 
        mac.rawAddress[0], 
        mac.rawAddress[1], 
        mac.rawAddress[2], 
        mac.rawAddress[3], 
        mac.rawAddress[4], 
        mac.rawAddress[5]);
        
    esp_now_peer_info_t peerInfo;        
    memcpy(peerInfo.peer_addr, mac.rawAddress, ESP_NOW_ETH_ALEN);
    peerInfo.channel = 1;
    peerInfo.encrypt = true;
    peerInfo.ifidx = WIFI_IF_STA;
    memcpy(peerInfo.lmk, lmk, ESP_NOW_KEY_LEN);

    esp_err_t addPeerResult = esp_now_add_peer(&peerInfo);
    if (addPeerResult == ESP_OK) {
        if (esp_now_is_peer_exist(peerInfo.peer_addr)) {
            LOGLN("Add peer success");
        } else {
            LOGLN("Add peer failed - not found after adding");
            return false;
        }
    }
    else {
        LOGFMT("Failed to add peer, error code: %02X\n", addPeerResult);
        return false;
    }

    return true;
}

bool EspNowMessenger::settingsChanged(const Settings& settings, uint8_t changeFlags) {
    bool setPMKSuccess = true;
    bool setPeerSuccess = true;
    bool updatePeer = false;

    if (changeFlags & Settings::CHANGE_PRIMARY_KEY) {
        Settings::pmk_t pmk;
        settings.pmk(pmk);

        LOGLN("Setting ESP-NOW primary key");

        if (!setPMK(pmk)) {
            setPMKSuccess = false;
        }

        // Note that if the primary key changes we need to update the peer, 
        // because the local key is derived from the primary key.
        updatePeer = true;
        LOGLN("peer needs update (primary key changed)");
    }

    if (changeFlags & Settings::CHANGE_LOCAL_KEY) {
        LOGLN("peer needs update (local key changed)");
        updatePeer = true;
    }

    if (changeFlags & Settings::CHANGE_OTHER_ADDRESS) {
        LOGLN("peer needs update (other address changed)");        
        updatePeer = true;
    }    

    if (updatePeer) {
        LOGLN("Updating ESP-NOW peer");

        Settings::lmk_t lmk;
        settings.lmk(lmk);

        setPeerSuccess = setPeer(settings.otherMacAddress(), lmk);
    }

    return (setPMKSuccess && setPeerSuccess);
}

void EspNowMessenger::deleteAllPeers() {
    esp_now_peer_num_t totalPeers;
    if (!esp_now_get_peer_num(&totalPeers) == ESP_OK) {
        LOGLN("Failed to get peer count");
        return;
    }
    
    if (totalPeers.total_num == 0) {
        LOGLN("No peers to delete");
        return;
    }

    LOGFMT("Deleting %d peers...\n", totalPeers.total_num);
    
    // Array to hold MAC addresses of peers
    esp_now_peer_info_t peerInfo[totalPeers.total_num];

    // Retrieve all peers
    if (esp_now_fetch_peer(totalPeers.total_num, peerInfo) != ESP_OK) {
        LOGLN("Failed to fetch peer information.");
        return;
    }

    // Loop through and delete each peer
    char macStr[32];

    for (int i = 0; i < totalPeers.total_num; i++) {
        snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
                peerInfo[i].peer_addr[0], peerInfo[i].peer_addr[1], peerInfo[i].peer_addr[2],
                peerInfo[i].peer_addr[3], peerInfo[i].peer_addr[4], peerInfo[i].peer_addr[5]);

        LOGFMT("Deleting peer: %s\n", macStr);

        // Delete the peer
        if (esp_now_del_peer(peerInfo[i].peer_addr) != ESP_OK) {
            LOGFMT("Failed to delete peer: %s\n", macStr);
        }
    }
}