#pragma once

#include <Arduino.h>
#include <lvgl.h>
#include "Scene.h"
#include "MacAddress.h"
#include "Settings.h"
#include "config.h"

class SettingsScene: public Scene {
public:
    SettingsScene(Device& d);
    virtual ~SettingsScene() = default;

    virtual void willLoadScreen() override;
    virtual void didLoadScreen() override;
    virtual void willUnloadScreen() override;
    
    virtual void receivedMessage(const char* message) override;

private:
    bool areOtherMacAddressCellsValid();
    void validateEntryData();
    uint8_t getSyncFlags();
    void syncEntryData();

    static void saveClicked(lv_event_t* e);
    static void nextTabClicked(lv_event_t* e);

    static void tabViewValueChanged(lv_event_t* e);    

    static void displayBrightnessSliderValueChanged(lv_event_t* e);
    static void keysBrightnessSliderValueChanged(lv_event_t* e);

    static void rebootClicked(lv_event_t* e);
    static void rebootAlertRebootClicked(lv_event_t* e);
    static void rebootAlertCancelClicked(lv_event_t* e);    

    static void primaryKeyTextAreaValueChanged(lv_event_t* e);
    static void localKeyTextAreaValueChanged(lv_event_t* e);

    static void otherMacTextAreaValueChanged(lv_event_t* e);

    static void switchToEspNowClicked(lv_event_t* e);
    static void switchToEspNowAlertSwitchClicked(lv_event_t* e);
    static void switchToEspNowAlertCancelClicked(lv_event_t* e);

    static void myLoraAddressTextAreaValueChanged(lv_event_t* e);
    static void otherLoraAddressTextAreaValueChanged(lv_event_t* e);

    static void switchToLoraClicked(lv_event_t* e);
    static void switchToLoRaAlertSwitchClicked(lv_event_t* e);
    static void switchToLoRaAlertCancelClicked(lv_event_t* e);

    void setOneByteTextAreaStyle(lv_obj_t* textArea, const char* text);
    void setEncryptionTextAreaStyle(lv_obj_t* textArea, const char* text);

    void showConfirmationAlert(const char* title, 
                               const char* message, 
                               const char* cancelText, 
                               const char* confirmText,
                               void (*cancelCallback)(lv_event_t* e),
                               void (*confirmCallback)(lv_event_t* e),
                               int cancelButtonWidth = -1,
                               int confirmButtonWidth = -1);

    void closeConfirmationAlert();

private:
    enum class Tab: uint8_t {
        general,
        encryption,
        espNow,

#if defined(USE_LORA)
        lora,
#endif
    };

#if defined(USE_LORA)
    static constexpr uint8_t tabCount = 4;
#else
    static constexpr uint8_t tabCount = 3;
#endif

    static const char* tabNames[tabCount];

    inline lv_obj_t* getViewForTab(Tab t) {
        return tabs[uint8_t(t)];
    }

    inline bool isTabLoaded(Tab t) {
        return tabLoaded[uint8_t(t)];
    }

    inline void setTabLoaded(Tab t) {
        tabLoaded[uint8_t(t)] = true;;
    }    

    void buildGeneralTab();
    void buildEncryptionTab();
    void buildEspNowTab();

#if defined(USE_LORA)
    void buildLoraTab();
#endif

private:
    // Styles
    lv_style_t textAreaStyle;
    lv_style_t addressContainerStyle;

    // Button bar
    lv_obj_t* saveButton = nullptr;
    lv_obj_t* nextTabButton = nullptr;    

    // Tab view
    lv_obj_t* tabViewContainer = nullptr;
    lv_obj_t* tabView = nullptr;
    Tab currentTab = Tab::general;
    lv_obj_t* tabs[tabCount] = {nullptr};
    bool tabLoaded[tabCount] = {false};

    // General tab
    lv_obj_t* displayBrightnessSlider = nullptr;
    lv_obj_t* keysBrightnessSlider = nullptr;
    lv_obj_t* rebootButton = nullptr;    

    // Encryption tab
    lv_obj_t* primaryKeyTextArea = nullptr;
    lv_obj_t* localKeyTextArea = nullptr;

    // ESP-NOW tab
    lv_obj_t* macAddressLabel;
    lv_obj_t* macTextAreas[MacAddress::addressLength] = {nullptr};
    lv_obj_t* switchToEspNowButton = nullptr;

    // Lora tab
    lv_obj_t* myLoraAddressTextArea = nullptr;
    lv_obj_t* otherLoraAddressTextArea = nullptr;
    lv_obj_t* switchToLoRaButton = nullptr;

    // Alert
    lv_obj_t* messageBox = nullptr;

    // Cached copies of entered data so we can detect changes.
    // The save button will be disabled/hidden if any of these fail validation checks.
    Settings::pmk_string_t pmkEntry = {0};
    Settings::lmk_string_t lmkEntry = {0};
    MacAddress otherMacEntry;
    uint8_t myLoraAddressEntry = 0;
    uint8_t otherLoraAddressEntry = 0;
};