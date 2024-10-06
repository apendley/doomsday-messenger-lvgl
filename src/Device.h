#pragma once

#include <Adafruit_NeoPixel.h>
#include <Adafruit_ILI9341.h>
#include <lvgl.h>
#include <BBQ10Keyboard.h>
#include <TSC2004.h>
#include "MacAddress.h"
#include "Messenger.h"
#include "BatteryMonitor.h"
#include "SceneManager.h"
#include "Settings.h"
#include "MessageHistory.h"
#include "Color.h"

// More friendly names
typedef Adafruit_NeoPixel NeoPixel;
typedef Adafruit_ILI9341 Display;
typedef TSC2004 Touchpad;

struct Device {
// Metrics
public:
    static constexpr uint32_t displayWidth = 320;
    static constexpr uint32_t displayHeight = 240;

    enum class BarButton: uint8_t {
        one = 0,
        two,
        three,
        four
    };    

    static constexpr int32_t barButtonCount = 4;

// Construction
    Device(const MacAddress& _myMacAddress,
           SceneManager& _sceneManager,
           Settings& _settings,
           Messenger* _messenger,
           MessageHistory& _messageHistory,
           Display& _display,
           BBQ10Keyboard& _keyboard,
           Touchpad& _touchpad,
           NeoPixel& _pixel,
           BatteryMonitor& _batteryMonitor) :
        myMacAddress(_myMacAddress),
        sceneManager(_sceneManager),
        settings(_settings),
        messenger(_messenger),
        messageHistory(_messageHistory),
        display(_display),
        keyboard(_keyboard),
        touchpad(_touchpad),
        pixel(_pixel),
        batteryMonitor(_batteryMonitor)
    {
        assert(messenger != nullptr);
    }

// Public members
public:
    const MacAddress myMacAddress;
    SceneManager& sceneManager;
    Settings& settings;
    Messenger* messenger;
    MessageHistory& messageHistory;    

    Display& display;
    BBQ10Keyboard& keyboard;
    Touchpad& touchpad;
    NeoPixel& pixel;
    BatteryMonitor& batteryMonitor;

    // LVGL input device for keyboard. Included here so we can enable/disable it when showing modal pop-ups.
    lv_indev_t* lvglKeyboardIndev = nullptr;

    // Add an LVGL view (e.g. text area) to the keyboard group to subscribe to keyboard events.
    // Don't forget to remove it when you're done.
    lv_group_t* lvglKeyboardGroup = nullptr;

// Callback assignment for common device tasks provided by main program.
public:
    // Provide an implementation to flush input, if possible.
    void setFlushInputCallback(void (*cb)()) {
        flushInputCallback = cb;
    }

    // Provide an implementation to connect a bar button to its on-screen counterpart.
    void setConnectBarButtonCallback(void (*cb)(lv_obj_t* btn, BarButton bb)) {
        connectBarButtonCallback = cb;
    }

    // Provide an implementation to disconnect a bar button from its on-screen counterpart.
    void setDisconnectBarButtonCallback(void (*cb)(BarButton bb)) {
        disconnectBarButtonCallback = cb;
    }

    // Provide implementation for persisting settings
    void setSaveSettingsCallback(bool (*cb)()) {
        saveSettingsCallback = cb;
    }

    // Provide implementation for persisting message history
    void setSaveMessageHistoryCallback(bool (*cb)()) {
        saveMessageHistoryCallback = cb;
    }    

    // Provide implementation for new message indicator
    void setShowNewIndicatorCallback(void (*cb)(bool)) {
        showNewIndicatorCallback = cb;
    }

// Public helpers
public:
    void setPixelColor(const Color::RGB& c);

    inline void setPixelBlack() {
        setPixelColor(Color::RGB(0));
    }

    // Connect a bar button to its on-screen counterpart.
    void connectBarButton(lv_obj_t* btn, BarButton bb) {
        if (connectBarButtonCallback) {
            connectBarButtonCallback(btn, bb);
        }
    }

    // Disconnect a bar button from its on-screen counterpart.
    void disconnectBarButton(BarButton bb) {
        if (disconnectBarButtonCallback) {
            disconnectBarButtonCallback(bb);
        }
    }

    // Flush keyboard events.
    // Helpful for when long-running blocking operations occur, so if 
    // the keyboard fifo gets spammed we don't process each spammed key press.
    void flushInputEvents() {
        if (flushInputCallback) {
            flushInputCallback();
        }
    }

    // Save current settings to the SD card.
    bool saveSettings() {
        if (saveSettingsCallback == nullptr) {
            return false;
        }

        return saveSettingsCallback();
    }

    // Save current message history to the SD card.
    bool saveMessageHistory() {
        if (saveMessageHistoryCallback == nullptr) {
            return false;
        }

        return saveMessageHistoryCallback();
    }

    // Displays or hides the new message indicator
    void showNewIndicator(bool show) {
        if (showNewIndicatorCallback) {
            showNewIndicatorCallback(show);
        }
    }

    static const lv_point_t& getBarButtonIndicatorPosition(BarButton bb) {
        uint8_t index = uint8_t(bb);

        if (index >= barButtonCount) {
            static const lv_point_t zero = {0, 0};
            return zero;
        }

        return barButtonIndicatorPositions[index];
    }

// Private constants
private:
    static const lv_point_t barButtonIndicatorPositions[barButtonCount];

// Callbacks
private:
    bool (*saveSettingsCallback)() = nullptr;
    bool (*saveMessageHistoryCallback)() = nullptr;
    void (*flushInputCallback)() = nullptr;
    void (*connectBarButtonCallback)(lv_obj_t* btn, BarButton bb) = nullptr;
    void (*disconnectBarButtonCallback)(BarButton bb) = nullptr;
    void (*showNewIndicatorCallback)(bool) = nullptr;
};