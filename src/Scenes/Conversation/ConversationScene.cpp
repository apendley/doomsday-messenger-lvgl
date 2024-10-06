#include "ConversationScene.h"
#include <lvgl.h>
#include "GlobalTheme.h"
#include "config.h"

#include "SceneManager.h"
#include "Scenes/Compose/ComposeScene.h"
#include "Scenes/Settings/SettingsScene.h"

ConversationScene::ConversationScene(Device& d)
    : Scene(d)
{
    
}

void ConversationScene::willLoadScreen() {
    device.showNewIndicator(false);

    const int32_t messageBubbleWidth = 220;

    lv_style_init(&historyListStyle);
    lv_style_set_bg_color(&historyListStyle, lv_color_black());
    lv_style_set_border_width(&historyListStyle, 0);
    lv_style_set_radius(&historyListStyle, 0);

    lv_style_init(&myMessageStyle);
    lv_style_set_bg_color(&myMessageStyle, globalTheme.amber);
    lv_style_set_pad_top(&myMessageStyle, 4);
    lv_style_set_pad_bottom(&myMessageStyle, 4);
    lv_style_set_pad_left(&myMessageStyle, -4);
    lv_style_set_pad_right(&myMessageStyle, -4);    
    lv_style_set_text_font(&myMessageStyle, &lv_font_montserrat_18);
    lv_style_set_text_color(&myMessageStyle, lv_color_black());
    lv_style_set_radius(&myMessageStyle, 16);
    lv_style_set_border_color(&myMessageStyle, lv_color_black());
    lv_style_set_border_width(&myMessageStyle, 2);
    lv_style_set_width(&myMessageStyle, messageBubbleWidth);

    lv_style_init(&theirMessageStyle);
    lv_style_set_bg_color(&theirMessageStyle, globalTheme.green);
    lv_style_set_pad_top(&theirMessageStyle, 4);
    lv_style_set_pad_bottom(&theirMessageStyle, 4);    
    lv_style_set_pad_left(&theirMessageStyle, -4);
    lv_style_set_pad_right(&theirMessageStyle, -4);
    lv_style_set_margin_left(&theirMessageStyle, historyListWidth - messageBubbleWidth - 38);
    lv_style_set_text_font(&theirMessageStyle, &lv_font_montserrat_18);
    lv_style_set_text_color(&theirMessageStyle, lv_color_black());
    lv_style_set_radius(&theirMessageStyle, 16);
    lv_style_set_border_color(&theirMessageStyle, lv_color_black());
    lv_style_set_border_width(&theirMessageStyle, 2);
    lv_style_set_width(&theirMessageStyle, messageBubbleWidth);  

    // Title bar
    lv_obj_t* titleBarBg;
    {
        titleBarBg = lv_obj_create(screen);
        lv_obj_add_style(titleBarBg, &globalTheme.titleBg, 0);
        lv_obj_align_to(titleBarBg, screen, LV_ALIGN_TOP_MID, 0, 20);

        lv_obj_t* titleLabel = lv_label_create(screen);
        lv_obj_add_style(titleLabel, &globalTheme.titleText, 0);
        lv_label_set_text(titleLabel, "Messages");
        lv_obj_align_to(titleLabel, titleBarBg, LV_ALIGN_LEFT_MID, 0, 0);
    }

    // Delete message history button
    {
        deleteHistoryButton = lv_button_create(screen);
        lv_obj_add_style(deleteHistoryButton, &globalTheme.barButton, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_add_style(deleteHistoryButton, &globalTheme.pressedButton, LV_STATE_PRESSED);
        lv_obj_add_event_cb(deleteHistoryButton, deleteHistoryClicked, LV_EVENT_CLICKED, this);

        // Add a label to the button
        lv_obj_t* label = lv_label_create(deleteHistoryButton);
        lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
        lv_label_set_text(label, "Clear");
        lv_obj_align_to(deleteHistoryButton, titleBarBg, LV_ALIGN_RIGHT_MID, 0, 0);        
    }

    // Bar button background
    {
        lv_obj_t* bg = lv_obj_create(screen);
        lv_obj_add_style(bg, &globalTheme.buttonBarBg, 0);
    }

    // Compose button
    {
        composeButton = lv_button_create(screen);
        lv_obj_add_style(composeButton, &globalTheme.barButton, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_add_style(composeButton, &globalTheme.pressedButton, LV_STATE_PRESSED);
        lv_obj_align(composeButton, LV_ALIGN_BOTTOM_RIGHT, -2, -6);
        lv_obj_add_event_cb(composeButton, composeClicked, LV_EVENT_CLICKED, this);

        // Add a label to the button
        lv_obj_t* label = lv_label_create(composeButton);
        lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
        lv_label_set_text(label, "Compose");

        // Add the bar button indicator
        lv_obj_t* indicator = lv_obj_create(screen);
        lv_obj_add_style(indicator, &globalTheme.barButtonIndicatorFour, 0);
    }

    // Settings button
    {
        // Button
        settingsButton = lv_button_create(screen);
        lv_obj_add_style(settingsButton, &globalTheme.barButton, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_add_style(settingsButton, &globalTheme.pressedButton, LV_STATE_PRESSED);
        lv_obj_align(settingsButton, LV_ALIGN_BOTTOM_LEFT, 2, -6);
        lv_obj_add_event_cb(settingsButton, settingsClicked, LV_EVENT_CLICKED, this);

        // Label
        lv_obj_t* label = lv_label_create(settingsButton);
        lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
        lv_label_set_text(label, "Settings");

        // Physical button indicator
        lv_obj_t* indicator = lv_obj_create(screen);
        lv_obj_add_style(indicator, &globalTheme.barButtonIndicatorOne, 0);
    }    

    // Message list
    buildMessageList();
}

void ConversationScene::didLoadScreen() {
    device.connectBarButton(settingsButton, Device::BarButton::one);
    device.connectBarButton(composeButton, Device::BarButton::four);    
}

void ConversationScene::willUnloadScreen() {
    lv_group_remove_obj(historyList);

    device.disconnectBarButton(Device::BarButton::one);
    device.disconnectBarButton(Device::BarButton::four);
}

void ConversationScene::buildMessageList() {
    if (historyList != nullptr) {
        lv_obj_del(historyList);
    }

    historyList = lv_list_create(screen);
    lv_obj_add_style(historyList, &historyListStyle, 0);
    lv_obj_set_size(historyList, historyListWidth, 158);
    lv_obj_set_pos(historyList, (device.displayWidth - historyListWidth)/2, 52);
    lv_group_add_obj(device.lvglKeyboardGroup, historyList);

    char buffer[Message::bufferSize + 32];
    const uint32_t bufferSize = sizeof(buffer);

    const int count = device.messageHistory.size();

    if (count == 0) {
        lv_obj_add_flag(deleteHistoryButton, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    lv_obj_remove_flag(deleteHistoryButton, LV_OBJ_FLAG_HIDDEN);    

    lv_obj_t* lastItem = nullptr;    

    for (int i = 0; i < count; i++) {
        Message message = device.messageHistory.getMessage(i);

        if (message.sender == Message::Sender::me) {
            snprintf(buffer, bufferSize, "%s: %s", MY_NAME, message.text);
            buffer[bufferSize - 1] = 0;            

            lv_obj_t* label = lv_list_add_text(historyList, buffer);
            lv_obj_add_style(label, &myMessageStyle, 0);
            lastItem = label;
        } else {
            snprintf(buffer, bufferSize, "%s: %s", OTHER_NAME, message.text);
            buffer[bufferSize - 1] = 0;

            lv_obj_t* label = lv_list_add_text(historyList, buffer);
            lv_obj_add_style(label, &theirMessageStyle, 0);
            lastItem = label;
        }
    }

    if (lastItem) {
        lv_obj_scroll_to_view(lastItem, LV_ANIM_OFF);
    }
}

void ConversationScene::deleteHistoryClicked(lv_event_t* e) {
    ConversationScene* scene = (ConversationScene*)lv_event_get_user_data(e);

    scene->showConfirmationAlert("Delete history?", "You cannot undo this action",
                                 "Cancel", "Delete",
                                 deleteHistoryAlertCancelClicked,
                                 deleteHistoryAlertConfirmClicked);
}

void ConversationScene::deleteHistoryAlertCancelClicked(lv_event_t* e) {
    ConversationScene* scene = (ConversationScene*)lv_event_get_user_data(e);
    scene->closeConfirmationAlert();
}

void ConversationScene::deleteHistoryAlertConfirmClicked(lv_event_t* e) {
    ConversationScene* scene = (ConversationScene*)lv_event_get_user_data(e);

    scene->device.messageHistory.clear();
    scene->device.saveMessageHistory();
    scene->buildMessageList();

    scene->closeConfirmationAlert();
}

void ConversationScene::composeClicked(lv_event_t* e) {
    ConversationScene* scene = (ConversationScene*)lv_event_get_user_data(e);
    scene->device.sceneManager.gotoScene(new ComposeScene(scene->device));
}

void ConversationScene::settingsClicked(lv_event_t* e) {
    ConversationScene* scene = (ConversationScene*)lv_event_get_user_data(e);
    scene->device.sceneManager.gotoScene(new SettingsScene(scene->device));
}

void ConversationScene::receivedMessage(const char* message) {
    buildMessageList();
}

void ConversationScene::showConfirmationAlert(const char* title, 
                                              const char* message, 
                                              const char* cancelText, 
                                              const char* confirmText,
                                              void (*cancelCallback)(lv_event_t* e),
                                              void (*confirmCallback)(lv_event_t* e),
                                              int cancelButtonWidth,
                                              int confirmButtonWidth)
{
    if (messageBox != nullptr) {
        return;
    }

    messageBox = lv_msgbox_create(NULL);
    lv_obj_set_style_bg_color(messageBox, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_border_color(messageBox, lv_color_black(), LV_PART_MAIN);

    lv_obj_t* titleLabel = lv_msgbox_add_title(messageBox, title);
    lv_obj_add_style(titleLabel, &globalTheme.titleText, LV_PART_MAIN);

    lv_obj_t* header = lv_msgbox_get_header(messageBox);
    lv_obj_set_style_bg_color(header, globalTheme.amber, LV_PART_MAIN);

    if (message != nullptr) {
        lv_obj_t* label = lv_msgbox_add_text(messageBox, message);
        lv_obj_add_style(label, &globalTheme.bodyText, LV_PART_MAIN);
    }

    lv_obj_t* button = lv_msgbox_add_footer_button(messageBox, cancelText);
    lv_obj_add_style(button, &globalTheme.standardButton, LV_PART_MAIN);
    lv_obj_add_style(button, &globalTheme.pressedButton, LV_STATE_PRESSED);

    if (cancelButtonWidth > -1) {
        lv_obj_set_width(button, cancelButtonWidth);
    }

    lv_obj_add_event_cb(button, cancelCallback, LV_EVENT_CLICKED, this);

    button = lv_msgbox_add_footer_button(messageBox, confirmText);
    lv_obj_add_style(button, &globalTheme.standardButton, LV_PART_MAIN);
    lv_obj_add_style(button, &globalTheme.pressedButton, LV_STATE_PRESSED);

    if (confirmButtonWidth > -1) {
        lv_obj_set_width(button, confirmButtonWidth);
    }

    lv_obj_add_event_cb(button, confirmCallback, LV_EVENT_CLICKED, this);

    lv_indev_enable(device.lvglKeyboardIndev, false);    
}

void ConversationScene::closeConfirmationAlert() {
    if (messageBox == nullptr) {
        return;
    }

    lv_msgbox_close(messageBox);
    messageBox = nullptr;

    device.flushInputEvents();
    lv_indev_enable(device.lvglKeyboardIndev, true);    
}
