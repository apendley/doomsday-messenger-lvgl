#pragma once

#include <Arduino.h>
#include "Scene.h"
#include <lvgl.h>

class ComposeScene: public Scene {
public:
    ComposeScene(Device& d);
    virtual ~ComposeScene() = default;

    virtual void willLoadScreen() override;
    virtual void didLoadScreen() override;
    virtual void willUnloadScreen() override;
    
    virtual void receivedMessage(const char* message) override;

private:
    void updateCharacterCountLabel(int32_t currentLength);

    static void sendClicked(lv_event_t* e);
    static void backButtonEvent(lv_event_t* e);
    static void textAreaValueChanged(lv_event_t* e);

private:
    lv_style_t cursorStyle;
    lv_style_t textAreaStyle;

    lv_obj_t* titleBg = nullptr;
    lv_obj_t* characterCountLabel = nullptr;
    lv_obj_t* sendButton = nullptr;
    lv_obj_t* backButton = nullptr;
    lv_obj_t* backButtonLabel = nullptr;
    lv_obj_t* textArea = nullptr;
};