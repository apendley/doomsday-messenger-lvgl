#include "SettingsScene.h"
#include <lvgl.h>
#include "Messenger.h"
#include "GlobalTheme.h"
#include "Color.h"
#include "config.h"

#include "SceneManager.h"
#include "Scenes/Conversation/ConversationScene.h"

// #define LOGGER Serial
#include "Logger.h"

const char* SettingsScene::tabNames[tabCount] = {
    "General",
    "Encryption",
    "ESP-NOW",
#if defined(USE_LORA)
    "LoRa"
#endif
};

SettingsScene::SettingsScene(Device& d)
    : Scene(d)
{
    
}

void SettingsScene::willLoadScreen() {
    // Copy editable data from settings
    device.settings.pmkString(pmkEntry);
    device.settings.lmkString(lmkEntry);
    otherMacEntry = device.settings.otherMacAddress();
    myLoraAddressEntry = device.settings.myLoraAddress();
    otherLoraAddressEntry = device.settings.otherLoraAddress();

    // Address container style
    lv_style_init(&addressContainerStyle);
    lv_style_set_radius(&addressContainerStyle, 0);
    lv_style_set_bg_color(&addressContainerStyle, lv_color_black());
    lv_style_set_border_width(&addressContainerStyle, 0);
    lv_style_set_layout(&addressContainerStyle, LV_LAYOUT_FLEX);
    lv_style_set_flex_flow(&addressContainerStyle, LV_FLEX_FLOW_ROW);
    lv_style_set_pad_all(&addressContainerStyle, 0);

    // Text area style for address entry
    lv_style_init(&textAreaStyle);
    lv_style_set_text_align(&textAreaStyle, LV_TEXT_ALIGN_CENTER);
    lv_style_set_pad_all(&textAreaStyle, 2);
    lv_style_set_bg_color(&textAreaStyle, lv_color_black());
    lv_style_set_text_color(&textAreaStyle, globalTheme.amber);
    lv_style_set_border_color(&textAreaStyle, globalTheme.amber);
    lv_style_set_border_width(&textAreaStyle, 2);

    // Title bar
    {
        lv_obj_t* bg = lv_obj_create(screen);
        lv_obj_add_style(bg, &globalTheme.titleBg, 0);
        lv_obj_align_to(bg, screen, LV_ALIGN_TOP_MID, 0, 20);

        lv_obj_t* titleLabel = lv_label_create(screen);
        lv_obj_add_style(titleLabel, &globalTheme.titleText, 0);
        lv_label_set_text(titleLabel, "Settings");
        lv_obj_align_to(titleLabel, bg, LV_ALIGN_LEFT_MID, 0, 0);
    }
    
    // Bar button background
    {
        lv_obj_t* bg = lv_obj_create(screen);
        lv_obj_add_style(bg, &globalTheme.buttonBarBg, 0);
    }

    // Save button
    {
        saveButton = lv_button_create(screen);
        lv_obj_add_style(saveButton, &globalTheme.barButton, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_add_style(saveButton, &globalTheme.pressedButton, LV_STATE_PRESSED);
        lv_obj_align(saveButton, LV_ALIGN_BOTTOM_RIGHT, -2, -6);
        lv_obj_add_event_cb(saveButton, saveClicked, LV_EVENT_CLICKED, this);

        // Add a label to the button
        lv_obj_t* label = lv_label_create(saveButton);
        lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
        lv_label_set_text(label, "Save & Exit");

        // Add the bar button indicator
        lv_obj_t* indicator = lv_obj_create(screen);
        lv_obj_add_style(indicator, &globalTheme.barButtonIndicatorFour, 0);
    }

    // Next tab button
    {
        nextTabButton = lv_button_create(screen);
        lv_obj_add_style(nextTabButton, &globalTheme.barButton, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_add_style(nextTabButton, &globalTheme.pressedButton, LV_STATE_PRESSED);
        lv_obj_align(nextTabButton, LV_ALIGN_BOTTOM_LEFT, 2, -6);
        lv_obj_add_event_cb(nextTabButton, nextTabClicked, LV_EVENT_CLICKED, this);

        // Add a label to the button
        lv_obj_t* label = lv_label_create(nextTabButton);
        lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
        lv_label_set_text(label, "Next Tab");

        // Add the bar button indicator
        lv_obj_t* indicator = lv_obj_create(screen);
        lv_obj_add_style(indicator, &globalTheme.barButtonIndicatorOne, 0);
    }

    // Tab view container 
    {
        tabViewContainer = lv_obj_create(screen);
        lv_obj_add_style(tabViewContainer, &globalTheme.tabViewContainerStyle, LV_PART_MAIN);
        lv_obj_set_pos(tabViewContainer, 0, 50);
        lv_obj_set_size(tabViewContainer, Device::displayWidth, 160);
    }

    // Tab view
    {
        tabView = lv_tabview_create(tabViewContainer);
        lv_obj_add_style(tabView, &globalTheme.tabViewStyle, LV_PART_MAIN);
        
        lv_tabview_set_tab_bar_position(tabView, LV_DIR_LEFT);
        lv_tabview_set_tab_bar_size(tabView, 90);
        lv_obj_add_event_cb(tabView, tabViewValueChanged, LV_EVENT_VALUE_CHANGED, this);

        // Seems like we can add the default style for the buttons here,
        // but for some reason not the selected state style.
        lv_obj_t* tabButtons = lv_tabview_get_tab_bar(tabView);
        lv_obj_add_style(tabButtons, &globalTheme.tabButtonsStyle, LV_PART_MAIN);

        // Add a blank view for each tab. We'll build the tab sub-views as needed.
        for (int i = 0; i < tabCount; i++) {
            tabs[i] = lv_tabview_add_tab(tabView, tabNames[i]);
        }

        lv_obj_remove_flag(lv_tabview_get_content(tabView), LV_OBJ_FLAG_SCROLLABLE);
    }

    // Build the first tab
    buildGeneralTab();

    // Tab view value changed event doesn't get triggered on the starting tab, 
    // so add the general controls to the keyboard group manually.
    lv_group_add_obj(device.lvglKeyboardGroup, displayBrightnessSlider);    
    lv_group_add_obj(device.lvglKeyboardGroup, keysBrightnessSlider);
    lv_group_focus_obj(displayBrightnessSlider);
}

void SettingsScene::didLoadScreen() {
    device.connectBarButton(nextTabButton, Device::BarButton::one);
    device.connectBarButton(saveButton, Device::BarButton::four);
}

void SettingsScene::willUnloadScreen() {
    device.setPixelBlack();

    if (isTabLoaded(Tab::general)) {
        lv_group_remove_obj(displayBrightnessSlider);
        lv_group_remove_obj(keysBrightnessSlider);
    }

    if (isTabLoaded(Tab::encryption)) {
        lv_group_remove_obj(primaryKeyTextArea);
        lv_group_remove_obj(localKeyTextArea);
    }

    if (isTabLoaded(Tab::espNow)) {
        for (int i = 0; i < MacAddress::addressLength; i++) {
            lv_group_remove_obj(macTextAreas[i]);
        }
    }

#if defined(USE_LORA)
    if (isTabLoaded(Tab::lora)) {
        lv_group_remove_obj(myLoraAddressTextArea);
        lv_group_remove_obj(otherLoraAddressTextArea);    
    }
#endif

    device.disconnectBarButton(Device::BarButton::one);
    device.disconnectBarButton(Device::BarButton::four);
}

void SettingsScene::buildGeneralTab() {
    const Tab tab = Tab::general;

    if (isTabLoaded(tab)) {
        return;
    }

    // Non-scrollable
    lv_obj_t* view = getViewForTab(tab);
    lv_obj_remove_flag(view, LV_OBJ_FLAG_SCROLLABLE);

    // None of the example code that sets these properties on the button container
    // seems to actually work (bug or am I misunderstanding?), but styling the buttons directly does.
    lv_obj_t* tabButtons = lv_tabview_get_tab_bar(tabView);
    lv_obj_t* button = lv_obj_get_child(tabButtons, uint8_t(tab));
    lv_obj_add_style(button, &globalTheme.tabButtonsSelectedStyle, LV_STATE_CHECKED);

    // Brightness label
    {
        lv_obj_t* label = lv_label_create(view);
        lv_obj_add_style(label, &globalTheme.bodyText, 0);
        lv_label_set_text(label, "Brightness");
        lv_obj_align(label, LV_ALIGN_TOP_MID, 0, -8);
    }

    // Display brightness slider
    {
        int yPos = 22;

        lv_obj_t* label = lv_label_create(view);
        lv_obj_add_style(label, &globalTheme.bodyText, 0);
        lv_label_set_text(label, "Display");
        lv_obj_align(label, LV_ALIGN_TOP_LEFT, 0, yPos);

        lv_obj_t* slider = lv_slider_create(view);
        lv_obj_add_style(slider, &globalTheme.sliderTrack, LV_PART_MAIN);        
        lv_obj_add_style(slider, &globalTheme.sliderTrack, LV_PART_INDICATOR);
        lv_obj_add_style(slider, &globalTheme.sliderKnob, LV_PART_KNOB);
        lv_obj_add_style(slider, &globalTheme.sliderFocus, LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
        lv_obj_set_width(slider, 120);
        lv_slider_set_range(slider, 1, 100);
        lv_slider_set_value(slider, device.keyboard.displayBrightness() * 100, LV_ANIM_OFF);
        lv_obj_align(slider, LV_ALIGN_TOP_RIGHT, 0, yPos);
        lv_obj_add_event_cb(slider, displayBrightnessSliderValueChanged, LV_EVENT_VALUE_CHANGED, this);
        displayBrightnessSlider = slider;
    }        

    // Keys brightness slider
    {
        int yPos = 56;
        
        lv_obj_t* label = lv_label_create(view);
        lv_obj_add_style(label, &globalTheme.bodyText, 0);
        lv_label_set_text(label, "Keyboard");
        lv_obj_align(label, LV_ALIGN_TOP_LEFT, 0, yPos);

        lv_obj_t* slider = lv_slider_create(view);
        lv_obj_add_style(slider, &globalTheme.sliderTrack, LV_PART_MAIN);
        lv_obj_add_style(slider, &globalTheme.sliderTrack, LV_PART_INDICATOR);        
        lv_obj_add_style(slider, &globalTheme.sliderKnob, LV_PART_KNOB);
        lv_obj_add_style(slider, &globalTheme.sliderFocus, LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
        lv_obj_set_width(slider, 120);
        lv_obj_align(slider, LV_ALIGN_TOP_RIGHT, 0, yPos);
        lv_slider_set_range(slider, 0, 100);
        lv_slider_set_value(slider, device.keyboard.keyboardBrightness() * 100, LV_ANIM_OFF);
        lv_obj_add_event_cb(slider, keysBrightnessSliderValueChanged, LV_EVENT_VALUE_CHANGED, this);
        keysBrightnessSlider = slider;
    }

    // Reboot button
    {
        // Button
        rebootButton = lv_button_create(view);
        lv_obj_add_style(rebootButton, &globalTheme.standardButton, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_add_style(rebootButton, &globalTheme.pressedButton, LV_STATE_PRESSED);
        lv_obj_add_event_cb(rebootButton, rebootClicked, LV_EVENT_CLICKED, this);

        // Label
        lv_obj_t* label = lv_label_create(rebootButton);
        lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
        lv_label_set_text(label, "Reboot");

        // Do this after we set the text or else the position will be aligned using empty text.
        lv_obj_align(rebootButton, LV_ALIGN_BOTTOM_MID, 0, 0);
    }

    setTabLoaded(tab);
}    

void SettingsScene::buildEncryptionTab() {
    const Tab tab = Tab::encryption;

    if (isTabLoaded(tab)) {
        return;
    }

    // Non-scrollable
    lv_obj_t* view = getViewForTab(tab);
    lv_obj_remove_flag(view, LV_OBJ_FLAG_SCROLLABLE);

    // None of the example code that sets these properties on the button container
    // seems to actually work (bug?), but styling the buttons directly does.
    lv_obj_t* tabButtons = lv_tabview_get_tab_bar(tabView);
    lv_obj_t* button = lv_obj_get_child(tabButtons, uint8_t(tab));
    lv_obj_add_style(button, &globalTheme.tabButtonsSelectedStyle, LV_STATE_CHECKED);    

    // Primary key (ESP-NOW and LoRa)
    {
        lv_obj_t* label = lv_label_create(view);
        lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
        lv_obj_add_style(label, &globalTheme.bodyText, LV_PART_MAIN);
#if defined(USE_LORA)
        lv_label_set_text(label, "Primary key (16 bytes)\nESP-NOW & LoRa");
#else
        lv_label_set_text(label, "Primary key (16 bytes)");
#endif
        lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 0);

        primaryKeyTextArea = lv_textarea_create(view);
        setEncryptionTextAreaStyle(primaryKeyTextArea, pmkEntry);
        lv_obj_align_to(primaryKeyTextArea, label, LV_ALIGN_OUT_BOTTOM_MID, 0, 4);

        lv_obj_add_event_cb(primaryKeyTextArea, primaryKeyTextAreaValueChanged, LV_EVENT_VALUE_CHANGED, this);
    }

    // Secondary key (ESP-NOW only)
    {
        lv_obj_t* label = lv_label_create(view);
        lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
        lv_obj_add_style(label, &globalTheme.bodyText, LV_PART_MAIN);
#if defined(USE_LORA)
        lv_label_set_text(label, "Local key (16 bytes)\nESP-NOW only");
#else
        lv_label_set_text(label, "Local key (16 bytes)");
#endif

        lv_obj_align_to(label, primaryKeyTextArea, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);

        localKeyTextArea = lv_textarea_create(view);
        setEncryptionTextAreaStyle(localKeyTextArea, lmkEntry);
        lv_obj_align_to(localKeyTextArea, label, LV_ALIGN_OUT_BOTTOM_MID, 0, 4);

        lv_obj_add_event_cb(localKeyTextArea, localKeyTextAreaValueChanged, LV_EVENT_VALUE_CHANGED, this);
    }

    setTabLoaded(tab);
}    

void SettingsScene::buildEspNowTab() {
    const Tab tab = Tab::espNow;

    if (isTabLoaded(tab)) {
        return;
    }

    // Non-scrollable
    lv_obj_t* view = getViewForTab(tab);
    lv_obj_remove_flag(view, LV_OBJ_FLAG_SCROLLABLE);

    // None of the example code that sets these properties on the button container
    // seems to actually work (bug?), but styling the buttons directly does.
    lv_obj_t* tabButtons = lv_tabview_get_tab_bar(tabView);
    lv_obj_t* button = lv_obj_get_child(tabButtons, uint8_t(tab));
    lv_obj_add_style(button, &globalTheme.tabButtonsSelectedStyle, LV_STATE_CHECKED);

    // MAC address label
    {
        const MacAddress& macAddress = device.myMacAddress;

        char macBuffer[64];
        const int32_t macBufferSize = sizeof(macBuffer);
        snprintf(macBuffer, macBufferSize, "My MAC:  %02X %02X %02X %02X %02X %02X",
            macAddress.rawAddress[0], 
            macAddress.rawAddress[1], 
            macAddress.rawAddress[2], 
            macAddress.rawAddress[3], 
            macAddress.rawAddress[4], 
            macAddress.rawAddress[5]);

        macBuffer[macBufferSize - 1] = 0;

        // Mac address label
        macAddressLabel = lv_label_create(view);
        lv_obj_add_style(macAddressLabel, &globalTheme.bodyText, 0);
        lv_obj_set_style_text_color(macAddressLabel, globalTheme.green, LV_PART_MAIN);
        lv_label_set_text(macAddressLabel, macBuffer);
        lv_obj_align(macAddressLabel, LV_ALIGN_TOP_MID, 0, 0);
    }

    // Other mac address text entry
    {
        // Title label
        lv_obj_t* title = lv_label_create(view);
        lv_obj_add_style(title, &globalTheme.bodyText, LV_PART_MAIN);
        lv_label_set_text(title, "Other MAC");
        lv_obj_align_to(title, macAddressLabel, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);

        // Container for mac entry text areas
        lv_obj_t* macEntryContainer = lv_obj_create(view);
        lv_obj_add_style(macEntryContainer, &addressContainerStyle, LV_PART_MAIN);
        lv_obj_set_size(macEntryContainer, 214, 24);
        lv_obj_align_to(macEntryContainer, title, LV_ALIGN_OUT_BOTTOM_MID, 0, 6);
        lv_obj_remove_flag(macEntryContainer, LV_OBJ_FLAG_SCROLLABLE);

        // Mac entry text areas
        char macBuffer[3];
        const int32_t macBufferSize = sizeof(macBuffer);

        for (int i = 0; i < MacAddress::addressLength; i++) {
            snprintf(macBuffer, macBufferSize, "%02X", otherMacEntry.rawAddress[i]);
            macBuffer[macBufferSize - 1] = 0;

            lv_obj_t* textArea = lv_textarea_create(macEntryContainer);
            macTextAreas[i] = textArea;                
            setOneByteTextAreaStyle(textArea, macBuffer);
            lv_obj_set_style_margin_left(textArea, 0, LV_PART_MAIN);
            lv_obj_set_style_margin_right(textArea, -1, LV_PART_MAIN);                

            lv_obj_add_event_cb(textArea, otherMacTextAreaValueChanged, LV_EVENT_VALUE_CHANGED, this);
        }

#if defined(USE_LORA)
        // Switch to ESP-NOW button
        if (device.settings.activeRadio() == Settings::RadioType::lora) {
            // Button
            switchToEspNowButton = lv_button_create(view);
            lv_obj_add_style(switchToEspNowButton, &globalTheme.standardButton, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_add_style(switchToEspNowButton, &globalTheme.pressedButton, LV_STATE_PRESSED);

            // Label
            lv_obj_t* label = lv_label_create(switchToEspNowButton);
            lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
            lv_label_set_text(label, "Switch to ESP-NOW");
            lv_obj_align(switchToEspNowButton, LV_ALIGN_BOTTOM_MID, 0, 0);

            lv_obj_add_event_cb(switchToEspNowButton, switchToEspNowClicked, LV_EVENT_CLICKED, this);
        }
#endif
    }

    setTabLoaded(tab);
}    

#if defined(USE_LORA)
void SettingsScene::buildLoraTab() {
    const Tab tab = Tab::lora;

    if (isTabLoaded(tab)) {
        return;
    }

    // Non-scrollable
    lv_obj_t* view = getViewForTab(tab);
    lv_obj_remove_flag(view, LV_OBJ_FLAG_SCROLLABLE);

    // None of the example code that sets these properties on the button container
    // seems to actually work (bug?), but styling the buttons directly does.
    lv_obj_t* tabButtons = lv_tabview_get_tab_bar(tabView);
    lv_obj_t* button = lv_obj_get_child(tabButtons, uint8_t(tab));
    lv_obj_add_style(button, &globalTheme.tabButtonsSelectedStyle, LV_STATE_CHECKED);    

    lv_obj_t* myAddressContainer = nullptr;

    // My address text area
    {
        char loraBuffer[3];
        const int32_t loraBufferSize = sizeof(loraBuffer);
        snprintf(loraBuffer, loraBufferSize, "%02X", myLoraAddressEntry),
        loraBuffer[loraBufferSize - 1] = 0;

        // Container for label and text area
        lv_obj_t* container = lv_obj_create(view);
        lv_obj_add_style(container, &addressContainerStyle, LV_PART_MAIN);
        lv_obj_set_size(container, 150, 24);
        lv_obj_align(container, LV_ALIGN_TOP_MID, 0, 0);
        lv_obj_set_flex_align(container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_remove_flag(container, LV_OBJ_FLAG_SCROLLABLE);        
        myAddressContainer = container;

        // Label
        lv_obj_t* title = lv_label_create(container);
        lv_obj_add_style(title, &globalTheme.bodyText, LV_PART_MAIN);
        lv_label_set_text(title, "My Address");

        // Text area
        myLoraAddressTextArea = lv_textarea_create(container);
        setOneByteTextAreaStyle(myLoraAddressTextArea, loraBuffer);
        lv_obj_add_event_cb(myLoraAddressTextArea, myLoraAddressTextAreaValueChanged, LV_EVENT_VALUE_CHANGED, this);
    }

    // Other address text area
    {
        char loraBuffer[3];
        const int32_t loraBufferSize = sizeof(loraBuffer);
        snprintf(loraBuffer, loraBufferSize, "%02X", otherLoraAddressEntry),
        loraBuffer[loraBufferSize - 1] = 0;

        // Container for label and text area
        lv_obj_t* container = lv_obj_create(view);
        lv_obj_add_style(container, &addressContainerStyle, LV_PART_MAIN);
        lv_obj_set_size(container, 150, 24);
        lv_obj_align_to(container, myAddressContainer, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
        lv_obj_set_flex_align(container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_remove_flag(container, LV_OBJ_FLAG_SCROLLABLE);        

        // Label
        lv_obj_t* title = lv_label_create(container);
        lv_obj_add_style(title, &globalTheme.bodyText, LV_PART_MAIN);
        lv_label_set_text(title, "Other Address");

        // Text area
        otherLoraAddressTextArea = lv_textarea_create(container);
        setOneByteTextAreaStyle(otherLoraAddressTextArea, loraBuffer);
        lv_obj_add_event_cb(otherLoraAddressTextArea, otherLoraAddressTextAreaValueChanged, LV_EVENT_VALUE_CHANGED, this);
    }

    // Switch to LoRa button
    if (device.settings.activeRadio() == Settings::RadioType::espNow) {
        // Button
        switchToLoRaButton = lv_button_create(view);
        lv_obj_add_style(switchToLoRaButton, &globalTheme.standardButton, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_add_style(switchToLoRaButton, &globalTheme.pressedButton, LV_STATE_PRESSED);

        // Label
        lv_obj_t* label = lv_label_create(switchToLoRaButton);
        lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
        lv_label_set_text(label, "Switch to LoRa");
        lv_obj_align(switchToLoRaButton, LV_ALIGN_BOTTOM_MID, 0, 0);

        lv_obj_add_event_cb(switchToLoRaButton, switchToLoraClicked, LV_EVENT_CLICKED, this);
    }

    setTabLoaded(tab);
}
#endif

void SettingsScene::tabViewValueChanged(lv_event_t* e) {
    lv_obj_t* tabView = (lv_obj_t*)lv_event_get_target(e);
    SettingsScene* scene = (SettingsScene*)lv_event_get_user_data(e);
    Tab nextTab = Tab(lv_tabview_get_tab_active(tabView));

    // Remove outgoing tab's views from keyboard group
    switch (scene->currentTab) {
        case Tab::general:
            lv_group_remove_obj(scene->displayBrightnessSlider);
            lv_group_remove_obj(scene->keysBrightnessSlider);        
            break;

        case Tab::encryption:    
            lv_group_remove_obj(scene->primaryKeyTextArea);
            lv_group_remove_obj(scene->localKeyTextArea);
            break;            

        case Tab::espNow:
            for (int i = 0; i < MacAddress::addressLength; i++) {
                lv_group_remove_obj(scene->macTextAreas[i]);
            }
            break;

#if defined(USE_LORA)
        case Tab::lora:
            lv_group_remove_obj(scene->myLoraAddressTextArea);
            lv_group_remove_obj(scene->otherLoraAddressTextArea);
            break;
#endif
    }

    // Add incoming tab's views to keyboard group
    switch (nextTab) {
        case Tab::general:
            scene->buildGeneralTab();

            lv_group_add_obj(scene->device.lvglKeyboardGroup, scene->displayBrightnessSlider);                
            lv_group_add_obj(scene->device.lvglKeyboardGroup, scene->keysBrightnessSlider);
            lv_group_focus_obj(scene->displayBrightnessSlider);
            break;

        case Tab::encryption:    
            scene->buildEncryptionTab();

            lv_group_add_obj(scene->device.lvglKeyboardGroup, scene->primaryKeyTextArea);
            lv_group_add_obj(scene->device.lvglKeyboardGroup, scene->localKeyTextArea);
            lv_group_focus_obj(scene->primaryKeyTextArea);
            break;

        case Tab::espNow:
            scene->buildEspNowTab();

            for (int i = 0; i < MacAddress::addressLength; i++) {
                lv_group_add_obj(scene->device.lvglKeyboardGroup, scene->macTextAreas[i]);
            }
            break;

#if defined(USE_LORA)
        case Tab::lora:
            scene->buildLoraTab();

            lv_group_add_obj(scene->device.lvglKeyboardGroup, scene->myLoraAddressTextArea);
            lv_group_add_obj(scene->device.lvglKeyboardGroup, scene->otherLoraAddressTextArea);
            lv_group_focus_obj(scene->myLoraAddressTextArea);
            break;
#endif
    }

    scene->currentTab = nextTab;
}

bool SettingsScene::areOtherMacAddressCellsValid() {
    // If we've never loaded the view, then they're valid.
    if (!isTabLoaded(Tab::espNow)) {
        return true;
    }

    for (int i = 0; i < MacAddress::addressLength; i++) {
        size_t len = strlen(lv_textarea_get_text(macTextAreas[i]));
        
        if (len == 0) {
            return false;
        }
    }

    return true;
}

void SettingsScene::validateEntryData() {
    // First make sure all entered data is valid
    bool isValid = true;

    // Check to make sure primary key is long enough
    if (strlen(pmkEntry) != Settings::pmkSize) {
        isValid = false;
    }

    // Check to make sure local key is long enough
    if (isValid && strlen(lmkEntry) != Settings::pmkSize) {
        isValid = false;
    }    

    // Check to make sure all mac cells have a value in them
    if (isValid) {
        isValid = areOtherMacAddressCellsValid();
    }

#if defined(USE_LORA)
    // Check my lora address for validity
    if (isValid) {
        if (myLoraAddressEntry == 0xFF) {
            isValid = false;
        }
        else {
            const char* text = lv_textarea_get_text(myLoraAddressTextArea);
            size_t len = strlen(text);
            isValid = (len > 0);
        }
    }

    // Check other lora address for validity
    if (isValid) {
        if (otherLoraAddressEntry == 0xFF) {
            isValid = false;
        }
        else {
            const char* text = lv_textarea_get_text(otherLoraAddressTextArea);
            size_t len = strlen(text);
            isValid = (len > 0);
        }
    }    
#endif

    // Update save button state based on validity and reboot conditions.
    // Also, both of the "switch" buttons will be enabled/disabled,
    // since they both also save any edited changes before rebooting.
    if (isValid) {
        lv_obj_remove_state(saveButton, LV_STATE_DISABLED);
        
        if (switchToEspNowButton) {
            lv_obj_remove_state(switchToEspNowButton, LV_STATE_DISABLED);
        }

        if (switchToLoRaButton) {
            lv_obj_remove_state(switchToLoRaButton, LV_STATE_DISABLED);
        }
    }
    else {
        lv_obj_add_state(saveButton, LV_STATE_DISABLED);

        if (switchToEspNowButton) {
            lv_obj_add_state(switchToEspNowButton, LV_STATE_DISABLED);
        }

        if (switchToLoRaButton) {
            lv_obj_add_state(switchToLoRaButton, LV_STATE_DISABLED);
        }
    }
}

// Only call this after verifying all of the data is valid
void SettingsScene::syncEntryData() {
    // Get sync flags before we update settings so we can tell the messenger what's changed.
    uint8_t syncFlags = getSyncFlags();

    // Update the settings.
    Settings& settings = device.settings;
    settings.setPMK(pmkEntry);
    settings.setLMK(lmkEntry);
    settings.setOtherMacAddress(otherMacEntry);

#if defined(USE_LORA)
    settings.setMyLoraAddress(myLoraAddressEntry);
    settings.setOtherLoraAddress(otherLoraAddressEntry);
#endif

    // Save the settings
    if (!device.saveSettings()) {
        LOGLN("Failed to save settings when synching changes.");
    }

    // Sync up the messegner with the settings changes, if necessary.
    if (syncFlags != 0) {
        if (device.messenger->settingsChanged(settings, syncFlags)) {
            LOGLN("Failed to propagate changed settings to messenger object.");
        }
    }

    settings.logCurrent();    
}

uint8_t SettingsScene::getSyncFlags() {
    uint8_t flags = 0;

    // Primary key.
    Settings::pmk_string_t pmkSettings;
    device.settings.pmkString(pmkSettings);

    if (memcmp(pmkEntry, pmkSettings, Settings::pmkSize) != 0) {
        LOGLN("Primary key changed");
        flags |= Settings::CHANGE_PRIMARY_KEY;
    }

    // ESP-NOW specific changes
    if (device.settings.activeRadio() == Settings::RadioType::espNow) {
        Settings::lmk_string_t lmkSettings;
        device.settings.lmkString(lmkSettings);

        // Local key changed
        if (memcmp(lmkEntry, lmkSettings, Settings::lmkSize) != 0) {
            LOGLN("Local key changed");
            flags |= Settings::CHANGE_LOCAL_KEY;
        }

        // Other address changed
        if( otherMacEntry != device.settings.otherMacAddress()) {
            LOGLN("Other mac address changed (ESP-NOW)");
            flags |= Settings::CHANGE_OTHER_ADDRESS;
        }
    }
    // LoRa-specific changes
    else {
#if defined(USE_LORA)
        // My address changed
        if (myLoraAddressEntry != device.settings.myLoraAddress()) {
            LOGLN("My address changed (LoRa)");
            flags |= Settings::CHANGE_MY_ADDRESS;
        }

        // Other address changed
        if (otherLoraAddressEntry != device.settings.otherLoraAddress()) {
            LOGLN("Other address changed (LoRa)");
            flags |= Settings::CHANGE_OTHER_ADDRESS;
        }
#endif
    }

    return flags;
}

void SettingsScene::displayBrightnessSliderValueChanged(lv_event_t* e) {
    lv_obj_t* slider = (lv_obj_t*)lv_event_get_target(e);
    SettingsScene* scene = (SettingsScene*)lv_event_get_user_data(e);

    int32_t sliderValue = (int32_t)lv_slider_get_value(slider);
    scene->device.settings.setDisplayBrightness(sliderValue);
    float b = sliderValue / 100.f;
    scene->device.keyboard.setDisplayBrightness(b);
}

void SettingsScene::keysBrightnessSliderValueChanged(lv_event_t* e) {
    lv_obj_t* slider = (lv_obj_t*)lv_event_get_target(e);
    SettingsScene* scene = (SettingsScene*)lv_event_get_user_data(e);

    int32_t sliderValue = (int32_t)lv_slider_get_value(slider);
    scene->device.settings.setKeyboardBrightness(sliderValue);
    float b = sliderValue / 100.f;
    scene->device.keyboard.setKeyboardBrightness(b);
}

void SettingsScene::primaryKeyTextAreaValueChanged(lv_event_t* e) {
    lv_obj_t* textArea = (lv_obj_t*)lv_event_get_target(e);
    SettingsScene* scene = (SettingsScene*)lv_event_get_user_data(e);

    strcpy(scene->pmkEntry, lv_textarea_get_text(textArea));
    scene->validateEntryData();
}

void SettingsScene::localKeyTextAreaValueChanged(lv_event_t* e) {
    lv_obj_t* textArea = (lv_obj_t*)lv_event_get_target(e);
    SettingsScene* scene = (SettingsScene*)lv_event_get_user_data(e);

    strcpy(scene->lmkEntry, lv_textarea_get_text(textArea));
    scene->validateEntryData();
}

void SettingsScene::otherMacTextAreaValueChanged(lv_event_t* e) {
    lv_obj_t* textArea = (lv_obj_t*)lv_event_get_target(e);
    SettingsScene* scene = (SettingsScene*)lv_event_get_user_data(e);

    int index = -1;

    for (int i = 0; i < MacAddress::addressLength; i++) {
        if (textArea == scene->macTextAreas[i]) {
            index = i;
            break;
        }
    }

    if (index == -1) {
        return;
    }

    scene->otherMacEntry.rawAddress[index] = strtol(lv_textarea_get_text(textArea), NULL, 16);

    LOGFMT("Edited MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
        scene->otherMacEntry.rawAddress[0],
        scene->otherMacEntry.rawAddress[1],
        scene->otherMacEntry.rawAddress[2],
        scene->otherMacEntry.rawAddress[3],
        scene->otherMacEntry.rawAddress[4],
        scene->otherMacEntry.rawAddress[5]
    );    

    scene->validateEntryData();
}

void SettingsScene::saveClicked(lv_event_t* e) {
    SettingsScene* scene = (SettingsScene*)lv_event_get_user_data(e);
    scene->syncEntryData();
    scene->device.sceneManager.gotoScene(new ConversationScene(scene->device));        
}

void SettingsScene::nextTabClicked(lv_event_t* e) {
    SettingsScene* scene = (SettingsScene*)lv_event_get_user_data(e);

    int index = lv_tabview_get_tab_active(scene->tabView) + 1;

    if (index >= tabCount) {
        index = 0;
    }

    lv_tabview_set_active(scene->tabView, index, LV_ANIM_OFF);
}

void SettingsScene::receivedMessage(const char* message) {
    device.showNewIndicator(true);
    device.setPixelColor(Color::RGB(0, 255, 0));
}

void SettingsScene::showConfirmationAlert(const char* title, 
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

void SettingsScene::closeConfirmationAlert() {
    if (messageBox == nullptr) {
        return;
    }

    lv_msgbox_close(messageBox);
    messageBox = nullptr;

    device.flushInputEvents();
    lv_indev_enable(device.lvglKeyboardIndev, true);    
}

void SettingsScene::rebootClicked(lv_event_t* e) {
    SettingsScene* scene = (SettingsScene*)lv_event_get_user_data(e);
    scene->showConfirmationAlert("Reboot?", "Any edited settings will not be saved", 
                                 "Cancel", "Reboot", 
                                 rebootAlertCancelClicked, rebootAlertRebootClicked);
}

void SettingsScene::rebootAlertCancelClicked(lv_event_t* e) {
    SettingsScene* scene = (SettingsScene*)lv_event_get_user_data(e);
    scene->closeConfirmationAlert();
}

void SettingsScene::rebootAlertRebootClicked(lv_event_t* e) {
    SettingsScene* scene = (SettingsScene*)lv_event_get_user_data(e);

    // We still save the settings, but we don't sync edited changes to them.
    // This is mainly to ensure display and keyboard brightnesss are saved.
    if (!scene->device.saveSettings()) {
        LOGLN("Failed to save settings when rebooting");
    }

    delay(100);
    ESP.restart();
}

void SettingsScene::switchToEspNowClicked(lv_event_t* e) {
    lv_obj_t* toggle = (lv_obj_t*)lv_event_get_target(e);
    SettingsScene* scene = (SettingsScene*)lv_event_get_user_data(e);

    // I'm not entirely sure how to de-initialize the radios seamlessly (especially ESP-NOW), 
    // so for now we force a reboot to switch radios.
    scene->showConfirmationAlert("Switch radio?", "This will switch the device to ESP-NOW and reboot.", 
                                "Cancel", "Switch",
                                switchToEspNowAlertCancelClicked, switchToEspNowAlertSwitchClicked);

}
void SettingsScene::switchToEspNowAlertSwitchClicked(lv_event_t* e) {
    SettingsScene* scene = (SettingsScene*)lv_event_get_user_data(e);

    scene->device.settings.setActiveRadio(Settings::RadioType::espNow);
    scene->syncEntryData();

    delay(100);
    ESP.restart();    
}

void SettingsScene::switchToEspNowAlertCancelClicked(lv_event_t* e) {
    SettingsScene* scene = (SettingsScene*)lv_event_get_user_data(e);
    scene->closeConfirmationAlert();
}

void SettingsScene::myLoraAddressTextAreaValueChanged(lv_event_t* e) {
    lv_obj_t* textArea = (lv_obj_t*)lv_event_get_target(e);
    SettingsScene* scene = (SettingsScene*)lv_event_get_user_data(e);

    scene->myLoraAddressEntry = strtol(lv_textarea_get_text(textArea), NULL, 16);
    scene->validateEntryData();
}

void SettingsScene::otherLoraAddressTextAreaValueChanged(lv_event_t* e) {
    lv_obj_t* textArea = (lv_obj_t*)lv_event_get_target(e);
    SettingsScene* scene = (SettingsScene*)lv_event_get_user_data(e);

    scene->otherLoraAddressEntry = strtol(lv_textarea_get_text(textArea), NULL, 16);
    scene->validateEntryData();
}

void SettingsScene::switchToLoraClicked(lv_event_t* e) {
    lv_obj_t* toggle = (lv_obj_t*)lv_event_get_target(e);
    SettingsScene* scene = (SettingsScene*)lv_event_get_user_data(e);

    // I'm not entirely sure how to de-initialize the radios seamlessly (especially ESP-NOW), 
    // so for now we force a reboot to switch radios.
    scene->showConfirmationAlert("Switch radio?", "This will switch the device to LoRa and reboot.", 
                                 "Cancel", "Switch",
                                 switchToLoRaAlertCancelClicked, switchToLoRaAlertSwitchClicked);
}

void SettingsScene::switchToLoRaAlertCancelClicked(lv_event_t* e) {
    SettingsScene* scene = (SettingsScene*)lv_event_get_user_data(e);
    scene->closeConfirmationAlert();
}

void SettingsScene::switchToLoRaAlertSwitchClicked(lv_event_t* e) {
    SettingsScene* scene = (SettingsScene*)lv_event_get_user_data(e);

    scene->device.settings.setActiveRadio(Settings::RadioType::lora);
    scene->syncEntryData();

    delay(100);
    ESP.restart();
}

void SettingsScene::setOneByteTextAreaStyle(lv_obj_t* textArea, const char* text) {
    lv_obj_set_size(textArea, 30, 30);
    lv_textarea_set_text(textArea, text);
    lv_textarea_set_one_line(textArea, true);                
    lv_textarea_set_max_length(textArea, 2);
    lv_textarea_set_accepted_chars(textArea, "0123456789abcdefABCDEF");

    lv_obj_add_style(textArea, &textAreaStyle, LV_PART_MAIN);
    lv_obj_set_style_border_color(textArea, globalTheme.amber, LV_PART_CURSOR | LV_STATE_FOCUSED);
    lv_obj_set_style_outline_width(textArea, 0, LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);

    lv_obj_remove_flag(textArea, LV_OBJ_FLAG_SCROLLABLE);
}

void SettingsScene::setEncryptionTextAreaStyle(lv_obj_t* textArea, const char* text) {
    lv_obj_set_size(textArea, 200, 30);
    lv_textarea_set_text(textArea, text);
    lv_textarea_set_one_line(textArea, true);                
    lv_textarea_set_max_length(textArea, 16);

    lv_obj_add_style(textArea, &textAreaStyle, LV_PART_MAIN);
    lv_obj_set_style_border_color(textArea, globalTheme.amber, LV_PART_CURSOR | LV_STATE_FOCUSED);
    lv_obj_set_style_outline_width(textArea, 0, LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);

    lv_obj_remove_flag(textArea, LV_OBJ_FLAG_SCROLLABLE);    
}
