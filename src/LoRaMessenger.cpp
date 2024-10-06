#include <LoRaMessenger.h>
#include <RH_RF95.h>
#include <RHEncryptedDriver.h>
#include <RHReliableDatagram.h>
#include <Speck.h>

#include "config.h"

// #define LOGGER Serial
#include "Logger.h"

LoRaMessenger::LoRaMessenger(uint8_t _myAddress, uint8_t _otherAddress) :
    resetPin(RFM_RST),
    freq(RFM_FREQ),
    device(RFM_CS, RFM_IRQ),
    driver(device, cipher),
    manager(driver, _myAddress),
    otherAddress(_otherAddress)
{

}

bool LoRaMessenger::begin(const uint8_t (&pmk)[16]) {
    assert(pmk != nullptr);

    // Initialize reset pin
    pinMode(resetPin, OUTPUT);
    digitalWrite(resetPin, HIGH);
    delay(10);

    // Manually reset module.
    digitalWrite(resetPin, LOW);
    delay(10);
    digitalWrite(resetPin, HIGH);
    delay(10);

    // Init lora device
    if (!device.init()) {
        LOGLN("Failed to initialize LoRaMessenger radio.");
        return false;
    }

    // Set lora frequency
    if (!device.setFrequency(freq)) {
        LOGLN("Failed to set LoRaMessenger frequency.");
        return false;
    }

    // Initialize reliable datagram manager
    if (!manager.init()) {
        LOGLN("Failed to initialize reliable datagram manager.");
        return false;
    }

    // range from 13-23 for power
    device.setTxPower(23, false);

    // Set encryption key
    setPMK(pmk);

    // a few more retries
    manager.setRetries(6);
    manager.setTimeout(500);

    return true;
}

void LoRaMessenger::updateRx() {
    if (!manager.available()) {
        return;
    }

    uint8_t len = Message::maxLength;
    uint8_t from, to;

    if (manager.recvfromAck(rxBuffer, &len, &from, &to, nullptr, nullptr)) {
        // We only listen for messages from our paired device.
        if (from != otherAddress) {
            return;
        }

        // We only use the broadcast address for pings
        if (broadcastAddress == to) {
            if (rxBuffer[0] == pingByte1 && rxBuffer[1] == pingByte2) {
                pingCallback();
            }
        }
        // And we only accept messages meant for our address.
        else if (manager.thisAddress() == to) {
            if (payloadReceivedCallback) {
                payloadReceivedCallback(rxBuffer, len);
            }
        }
    }
}

void LoRaMessenger::ping() {
    LOGLN("Sending Ping");
    txBuffer[0] = pingByte1;
    txBuffer[1] = pingByte2;
    (void)manager.sendtoWait(txBuffer, 2, broadcastAddress);
}

bool LoRaMessenger::txWait(const uint8_t* payload, uint8_t len) {
    // Annoyingly sendToWait() does accepts a non-const buffer so
    // for now I'd rather just copy the payload into our own buffer
    // and pass that rather than casting the const-ness away.
    memcpy(txBuffer, payload, len);
    return manager.sendtoWait(txBuffer, len, otherAddress);
}

bool LoRaMessenger::settingsChanged(const Settings& settings, uint8_t changeFlags) {
    if (changeFlags & Settings::CHANGE_MY_ADDRESS) {
        LOGLN("Setting LoRa address");
        manager.setThisAddress(settings.myLoraAddress());
    }

    if (changeFlags & Settings::CHANGE_OTHER_ADDRESS) {
        LOGLN("Setting other LoRa address");
        otherAddress = settings.otherLoraAddress();        
    }
    
    if (changeFlags & Settings::CHANGE_PRIMARY_KEY) {
        LOGLN("Setting LoRa primary key");
        Settings::pmk_t pmk;
        settings.pmk(pmk);
        setPMK(pmk);
    }

    return true;
}