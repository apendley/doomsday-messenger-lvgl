#pragma once

#include <Arduino.h>
#include "Message.h"
#include "Settings.h"

// Base class interface for messengers.
class Messenger {
public: 
    // Call often, for example in loop().
    virtual void updateRx() = 0;

    // Send payload to paired address and wait (blocking).
    virtual bool txWait(const uint8_t* payload, uint8_t len) = 0;

    // Send a ping to the paired device (non-blocking).
    virtual void ping() = 0;

    // Call this when settings have been updated by user to update addresses and encryption keys.
    // Return false if any settings failed to be updated.
    virtual bool settingsChanged(const Settings& settings, uint8_t changeFlags) = 0;
};
