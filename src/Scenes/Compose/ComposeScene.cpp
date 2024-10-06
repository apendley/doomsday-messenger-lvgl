#include "ComposeScene.h"
#include "GlobalTheme.h"
#include "Color.h"
#include "Message.h"

#include "SceneManager.h"
#include "Scenes/Conversation/ConversationScene.h"

// #define LOGGER Serial
#include "Logger.h"

namespace {
    // Hack: Use a static buffer to store contents of work-in-progress message,
    // so that it will remain if the user cancels and returns.
    char composeBuffer[Message::bufferSize] = {0};
}

ComposeScene::ComposeScene(Device& d)
    : Scene(d)
{
    
}

void ComposeScene::willLoadScreen() {
    lv_style_init(&cursorStyle);
    lv_style_set_border_color(&cursorStyle, lv_color_white());

    lv_style_init(&textAreaStyle);
    lv_style_set_bg_color(&textAreaStyle, lv_color_black());
    lv_style_set_text_color(&textAreaStyle, lv_color_white());    
    lv_style_set_border_width(&textAreaStyle, 0);
    lv_style_set_radius(&textAreaStyle, 0);
    lv_style_set_text_font(&textAreaStyle, &lv_font_montserrat_18);

    // Title bar
    {
        titleBg = lv_obj_create(screen);
        lv_obj_add_style(titleBg, &globalTheme.titleBg, 0);
        lv_obj_align_to(titleBg, screen, LV_ALIGN_TOP_MID, 0, 20);

        lv_obj_t* titleLabel = lv_label_create(screen);
        lv_obj_add_style(titleLabel, &globalTheme.titleText, 0);
        lv_label_set_text(titleLabel, "Compose");
        lv_obj_align_to(titleLabel, titleBg, LV_ALIGN_LEFT_MID, 0, 0);

        characterCountLabel = lv_label_create(screen);
        lv_obj_add_style(characterCountLabel, &globalTheme.titleText, 0);
        updateCharacterCountLabel(strlen(composeBuffer));
    }

    // Bar button background
    {
        lv_obj_t* bg = lv_obj_create(screen);
        lv_obj_add_style(bg, &globalTheme.buttonBarBg, 0);
    }        

    // Text area
    {                                                                                 
        textArea = lv_textarea_create(screen);
        lv_obj_add_style(textArea, &cursorStyle, LV_PART_CURSOR | LV_STATE_FOCUSED);        
        lv_obj_add_style(textArea, &textAreaStyle, 0);
        lv_obj_set_size(textArea, device.displayWidth, 160);
        lv_textarea_set_text(textArea, composeBuffer);
        lv_obj_set_pos(textArea, 0, 50);
        lv_textarea_set_max_length(textArea, Message::maxLength);
        lv_obj_add_event_cb(textArea, textAreaValueChanged, LV_EVENT_VALUE_CHANGED, this);

        // Subscribe to keyboard input events.
        lv_group_add_obj(device.lvglKeyboardGroup, textArea);
    } 

    // Send button
    {
        // Physical button indicator
        lv_obj_t* indicator = lv_obj_create(screen);
        lv_obj_add_style(indicator, &globalTheme.barButtonIndicatorThree, 0);

        // Button
        sendButton = lv_button_create(screen);
        lv_obj_add_style(sendButton, &globalTheme.barButton, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_add_style(sendButton, &globalTheme.pressedButton, LV_STATE_PRESSED);
        lv_obj_add_event_cb(sendButton, sendClicked, LV_EVENT_CLICKED, this);

        // Label
        lv_obj_t* label = lv_label_create(sendButton);
        lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
        lv_label_set_text(label, "Send");

        // Do this after we set the text or else the position will be aligned using empty text.
        lv_obj_align_to(sendButton, indicator, LV_ALIGN_BOTTOM_MID, 0, 8);
    }

    // Back button
    {
        // Button
        backButton = lv_button_create(screen);
        lv_obj_add_style(backButton, &globalTheme.barButton, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_add_style(backButton, &globalTheme.pressedButton, LV_STATE_PRESSED);
        lv_obj_align(backButton, LV_ALIGN_BOTTOM_LEFT, 2, -6);
        lv_obj_add_event_cb(backButton, backButtonEvent, LV_EVENT_CLICKED, this);

        // Label
        lv_obj_t* label = lv_label_create(backButton);
        lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
        lv_label_set_text(label, "Messages");
        backButtonLabel = label;

        // Physical button indicator
        lv_obj_t* indicator = lv_obj_create(screen);
        lv_obj_add_style(indicator, &globalTheme.barButtonIndicatorOne, 0);
    }
}

void ComposeScene::didLoadScreen() {
    device.connectBarButton(backButton, Device::BarButton::one);
    device.connectBarButton(sendButton, Device::BarButton::three);
}

void ComposeScene::willUnloadScreen() {
    device.setPixelBlack();

    lv_group_remove_obj(textArea);
    device.disconnectBarButton(Device::BarButton::one);
    device.disconnectBarButton(Device::BarButton::three);
}

void ComposeScene::updateCharacterCountLabel(int32_t currentLength) {
    char buffer[64];
    const int32_t bufferSize = sizeof(buffer);
    snprintf(buffer, bufferSize, "%d / %d", currentLength, Message::maxLength);
    buffer[bufferSize-1] = 0;
    lv_label_set_text(characterCountLabel, buffer);
    lv_obj_align_to(characterCountLabel, titleBg, LV_ALIGN_RIGHT_MID, 0, 0);
}

void ComposeScene::sendClicked(lv_event_t* e) {
    ComposeScene* scene = (ComposeScene*)lv_event_get_user_data(e);
    Device& device = scene->device;

    const char* text = lv_textarea_get_text(scene->textArea);
    int32_t sendLength = strlen(text);

    if (sendLength > 0) {
        LOGFMT("Sending message: %s\n", text);
        
        // Send message
        device.setPixelColor(Color::RGB(0, 0, 255));

        if (device.messenger->txWait((uint8_t*)text, sendLength)) {
            device.setPixelBlack();

            // Update message history
            device.messageHistory.addMessage(Message::Sender::me, text);

            // Save it
            (void)device.saveMessageHistory();

            // Reset compose buffer now that we've sent the message.
            composeBuffer[0] = 0;                

            // Flush keyboard events in case user spammed send button
            device.flushInputEvents();

            // Return to conversation scene.
            device.sceneManager.gotoScene(new ConversationScene(device));
            return;
        }
        else {
            device.setPixelColor(Color::RGB(255, 0, 0));

            // Flush keyboard events in case user spammed send button
            device.flushInputEvents();
            LOGLN("Failed to send message");
        }
    }
}

void ComposeScene::backButtonEvent(lv_event_t* e) {
    ComposeScene* scene = (ComposeScene*)lv_event_get_user_data(e);

    // Save any text from the text area into the compose buffer
    const char* text = lv_textarea_get_text(scene->textArea);
    strncpy(composeBuffer, text, Message::bufferSize);
    composeBuffer[Message::maxLength] = 0;

    scene->device.sceneManager.gotoScene(new ConversationScene(scene->device));
}

void ComposeScene::textAreaValueChanged(lv_event_t * e) {
    lv_obj_t* textArea = (lv_obj_t*)lv_event_get_target(e);
    ComposeScene* scene = (ComposeScene*)lv_event_get_user_data(e);

    const char* text = lv_textarea_get_text(textArea);
    scene->updateCharacterCountLabel(strlen(text));
}

void ComposeScene::receivedMessage(const char* message) {
    device.showNewIndicator(true);
    device.setPixelColor(Color::RGB(0, 255, 0));

    if (backButtonLabel) {
        lv_label_set_text(backButtonLabel, "New Messages");
    }    
}
