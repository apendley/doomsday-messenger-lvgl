#pragma once

#include <Arduino.h>
#include <lvgl.h>
#include "Scene.h"

class ConversationScene: public Scene {
public:
    ConversationScene(Device& d);
    virtual ~ConversationScene() = default;

    virtual void willLoadScreen() override;
    virtual void didLoadScreen() override;
    virtual void willUnloadScreen() override;
    
    virtual void receivedMessage(const char* message) override;

private:
    void buildMessageList();

private:
    static void deleteHistoryClicked(lv_event_t* e);
    static void composeClicked(lv_event_t* e);
    static void settingsClicked(lv_event_t* e);

    static void deleteHistoryAlertCancelClicked(lv_event_t* e);    
    static void deleteHistoryAlertConfirmClicked(lv_event_t* e);    

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
    static constexpr int historyListWidth = Device::displayWidth - 8;

    lv_style_t historyListStyle;
    lv_style_t myMessageStyle;
    lv_style_t theirMessageStyle;

    lv_obj_t* historyList = nullptr;
    lv_obj_t* composeButton = nullptr;
    lv_obj_t* settingsButton = nullptr;
    lv_obj_t* deleteHistoryButton = nullptr;

    // Alert
    lv_obj_t* messageBox = nullptr;    
};