#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_ILI9341.h>
#include <BBQ10Keyboard.h>
#include <TSC2004.h>
#include <SdFat.h>
#include <lvgl.h>
#include "BatteryMonitor.h"
#include "MessageHistory.h"
#include "GlobalTheme.h"
#include "Color.h"
#include "SceneManager.h"
#include "Device.h"

// Radio messengers
#include "EspNowMessenger.h"

#if defined(USE_LORA)
#include "LoRaMessenger.h"
#endif

// The first scene we'll instantiate.
#include "Scenes/Conversation/ConversationScene.h"

// Device config
#include "config.h"

// Uncomment to enable logging in this file.
// #define LOGGER Serial
#include "Logger.h"

////////////////////////////////////
// Hardware drivers
////////////////////////////////////
Adafruit_NeoPixel pixel(1, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_ILI9341 display(TFT_CS, TFT_DC);
BBQ10Keyboard keyboard;
TSC2004 touchpad;
BatteryMonitor batteryMonitor;

////////////////////////////////////
// LVGL
////////////////////////////////////
// Draw buffer is screen size / 10.
const uint32_t drawBufferSize = Device::displayWidth * Device::displayHeight * (LV_COLOR_DEPTH / 8) / 10;

// We'll allocate the draw buffer in setup()
uint8_t* lvglDrawBuffer = nullptr;

// Common styles
GlobalTheme globalTheme;

// Battery indicator views
const int32_t batteryIndicatorCanvasWidth = 36;
const int32_t batteryIndicatorCanvasHeight = 20;
const uint32_t batteryIndicatorCanvasBufferSize = batteryIndicatorCanvasWidth * batteryIndicatorCanvasHeight * (LV_COLOR_DEPTH / 8);

// We'll allocate the battery canvas buffer and assign 
// references to the canvas and label in setup()
uint8_t* batteryIndicatorCanvasBuffer = nullptr;
lv_obj_t* batteryStatusLabel = nullptr;
lv_obj_t* batteryIndicatorCanvas = nullptr;

// Bar buttons constants and state
const char barButtonKeys[Device::barButtonCount] = {6, 17, 7, 18};    
bool lvglBarButtonDown[Device::barButtonCount] = {false};
lv_indev_t* barButtonIndevs[Device::barButtonCount] = {nullptr};
lv_point_t barButtonIndevTouchPoints[Device::barButtonCount] = {0};

// Simulate repeated keypresses
const uint32_t lvglKeyRepeatInterval = 75;
uint32_t lvglKeyHeld = false;
uint32_t lvglLastKeyPressed;
uint32_t lvglKeyPressedTimestamp = 0;

////////////////////////////////////
// Status bar labels
////////////////////////////////////
lv_obj_t* radioModeStatusLabel = nullptr;
lv_obj_t* pingStatusLabel = nullptr;
lv_obj_t* newMessageStatusLabel = nullptr;

// Ping
const uint32_t pingIndicatorTimeout = 5 * 1000;
bool pingIndicatorActive = false;
uint32_t pingIndicatorTimer = 0;

const int32_t minPingBroadcastTimeout = 60 * 1000;
const int32_t maxPingBroadcastTimeout = 120 * 1000;
int32_t pingBroadcastTimer = 0;

////////////////////////////////////
// SD Card
////////////////////////////////////
SdFat sdCard;
SdSpiConfig sdConfig(SD_CS, 0, SD_SCK_MHZ(50));
bool sdCardInitialized = false;

////////////////////////////////////
// Settings
////////////////////////////////////
const char* settingsFilename = "settings.cfg";
Settings settings;
bool settingsInitialized = false;

////////////////////////////////////
// Message history
////////////////////////////////////
const char* messageHistoryFilename = "messages.hst";
MessageHistory messageHistory;

//////////////////////////////////////////
// Device
//////////////////////////////////////////
// Allocated in setup() after all hardware has been configured.
Device* device = nullptr;

//////////////////////////////////////////
// Scene manager
//////////////////////////////////////////
SceneManager sceneManager;

//////////////////////////////////////////
// Loop timing
//////////////////////////////////////////
uint32_t lastMillis = 0;

//////////////////////////////////////////
// General implementation forward reference
//////////////////////////////////////////
void fatalError(const char* message);
bool loadSettings();
bool saveSettings();
bool loadMessageHistory();
bool saveMessageHistory();
bool deleteMessageHistory();

//////////////////////////////////////////
// Device callbacks forward reference
//////////////////////////////////////////
void flushInputEvents();
void connectBarButton(lv_obj_t* btn, Device::BarButton bb);
void disconnectBarButton(Device::BarButton bb);
void showNewIndicator(bool visible);

//////////////////////////////////////////
// LVGL implementation forward reference
//////////////////////////////////////////
void initLVGL();
uint32_t lvglTick();

void drawBatteryIndicator();

void lvglFlushDisplay(lv_display_t* disp, const lv_area_t* area, uint8_t* pixels);
void lvglTouchpadRead(lv_indev_t* indev, lv_indev_data_t* data);
void lvglKeyboardRead(lv_indev_t* indev, lv_indev_data_t* data);
void lvglBarButtonRead(lv_indev_t* indev, lv_indev_data_t* data);

//////////////////////////////////////////
// Battery events forward reference
//////////////////////////////////////////
void batteryPercentageChanged(uint8_t p);

//////////////////////////////////////////
// Radio events forward reference
//////////////////////////////////////////
void messengerPingCallback();
void messengerPayloadReceived(const uint8_t* payload, uint32_t len);

//////////////////////////////////////////
// Setup
//////////////////////////////////////////
void setup() {
    // Set up and turn off FeatherWing neopixel.
    pixel.begin();
    pixel.setBrightness(8);
    pixel.show();

    // Start serial monitor
    Serial.begin(115200);

    // Wait for serial monitor to be ready
#if defined (WAIT_FOR_SERIAL_CONSOLE_ON_BOOT) 
    while(!Serial) { delay(100); }
    LOGLN("Serial console started");
#endif    

    // Turn on I2C power
    pinMode(PIN_I2C_POWER, INPUT);
    delay(1);
    bool polarity = digitalRead(PIN_I2C_POWER);
    pinMode(PIN_I2C_POWER, OUTPUT);
    digitalWrite(PIN_I2C_POWER, !polarity);

    // Set up keyboard and turn off back lights to obscure the initial screen weirdness as much as possible.
    keyboard.begin();
    keyboard.setDisplayBrightness(0.0);
    keyboard.setKeyboardBrightness(0.0);

    // Turn off Feather Neopixel power. We aren't using it.
    pinMode(NEOPIXEL_POWER, OUTPUT);
    digitalWrite(NEOPIXEL_POWER, LOW);

    // Display; we goin' fast.
    display.begin(80000000);
    display.setRotation(1);
    display.setTextWrap(false);
    display.fillScreen(0);
    display.setTextSize(2);    
    display.setTextColor(Color::RGB(globalTheme.amber).packed565());    
    display.setCursor(0, 0);

    // Set brightness to defaults so we can see the intro sequence.
    keyboard.setDisplayBrightness(0.5);
    keyboard.setKeyboardBrightness(0.05);
    display.println("display OK");
    display.println("keyboard OK");

    // Touchscreen
    touchpad.begin();
    display.println("touchscreen OK");

    // Set up battery monitor    
    if (batteryMonitor.begin()) {
        batteryMonitor.setPercentChangedCallback(batteryPercentageChanged);
        display.println("battery monitor OK");
    }
    else {
        LOGLN("Failed to initialize battery monitor.");
        display.println("battery monitor FAIL");
    }    

    // Set default settings.
    settings.setDefaults();

    // SD card reader and settings
    if (sdCard.begin(sdConfig)) {
        sdCardInitialized = true;
        display.println("storage OK");

        if (loadSettings()) {
            settingsInitialized = true;
            display.println("config OK");
        }
        else {
            LOGLN("Error initializing settings, using defaults.");
            display.println("config FAIL");
        }
    }   
    else {
        LOGLN("Error initializing SD card reader, using default settings.");
        display.println("storage FAIL");
    }

    // Set brightness from settings
    keyboard.setDisplayBrightness(settings.displayBrightness() / 100.0f);
    keyboard.setKeyboardBrightness(settings.keyboardBrightness() / 100.0f);    

    settings.logCurrent();

    // Get mac address of this device
    MacAddress myMacAddress;
    if (!esp_read_mac(myMacAddress.rawAddress, ESP_MAC_WIFI_STA) == ESP_OK) {
        display.println("mac address FAIL");
        fatalError("Error retrieving MAC address");
    }

    display.println("mac address OK");

    // Get other mac address from settings.
    MacAddress otherMacAddress = settings.otherMacAddress();

    // Create the correct messenger type based on build config and/or selected radio type.
    Messenger* messenger = nullptr;

    {
        // Both messenger types use this key.
        Settings::pmk_t pmk;
        settings.pmk(pmk);

#if defined(USE_LORA)
        uint8_t myLoraAddress = settings.myLoraAddress();
        uint8_t otherLoraAddress = settings.otherLoraAddress();

        if (settings.activeRadio() == Settings::RadioType::lora) {
            // LoRa
            LoRaMessenger* lora = new LoRaMessenger(myLoraAddress, otherLoraAddress);

            if (!lora->begin(pmk)) {
                display.println("LoRa FAIL");
                fatalError("Error initializing LoRa radio");
            }

            display.println("LoRa OK");

            lora->setPingCallback(messengerPingCallback);
            lora->setPayloadReceivedCallback(messengerPayloadReceived);
            messenger = lora;
        }
        else {
#endif
            // ESP-NOW
            EspNowMessenger* espNow = new EspNowMessenger();

            Settings::lmk_t lmk;
            settings.lmk(lmk);        

            if (!espNow->begin(otherMacAddress, pmk, lmk)) {
                display.println("ESP-NOW FAIL");
                fatalError("Error initializing ESP-NOW radio");
            }

            display.println("ESP-NOW OK");

            espNow->setPayloadReceivedCallback(messengerPayloadReceived);    
            espNow->setPingCallback(messengerPingCallback);
            messenger = espNow;
#if defined(USE_LORA)
        }
#endif
    }

    // Global device object.
    device = new Device(myMacAddress, sceneManager, settings, messenger, messageHistory, 
                        display, keyboard, touchpad, pixel, batteryMonitor);

    device->setSaveSettingsCallback(saveSettings);
    device->setSaveMessageHistoryCallback(saveMessageHistory);
    device->setFlushInputCallback(flushInputEvents);
    device->setConnectBarButtonCallback(connectBarButton);
    device->setDisconnectBarButtonCallback(disconnectBarButton);
    device->setShowNewIndicatorCallback(showNewIndicator);

    // Good place to delete this if you ever hose it.
    // deleteMessageHistory();

    if (loadMessageHistory()) {
        display.println("message history OK");
    }
    else {
        display.println("message history FAIL");
        LOGLN("Failed to load message history");
    }

    // LVGL begin
    initLVGL();

    // Go to startup scene.
    sceneManager.gotoScene(new ConversationScene(*device));

    // Set first ping broadcast timer to happen soon after launch.
    // Randomize to reduce chances that two turned on at the same time
    // will send their first ping at the exact same time.
    pingBroadcastTimer = random(3000, 6000);

    // Finally, get the current timestamp, and we're good to go.
    lastMillis = millis();
}

//////////////////////////////////////////
// Loop
//////////////////////////////////////////
void loop() {
    uint32_t now = millis();
    uint32_t dt = now - lastMillis;
    lastMillis = now;

    // Update radio and battery monitor
    device->messenger->updateRx();
    batteryMonitor.update();

    // Then update LVGL
    lv_timer_handler();

    // and finally, update our scene.
    sceneManager.update(dt);

    // Update ping indicator and display if necessary.
    if (pingIndicatorActive) {
        pingIndicatorTimer += dt;
        if (pingIndicatorTimer >= pingIndicatorTimeout) {
            pingIndicatorActive = false;
            lv_obj_add_flag(pingStatusLabel, LV_OBJ_FLAG_HIDDEN);
            lv_obj_remove_flag(radioModeStatusLabel, LV_OBJ_FLAG_HIDDEN);
        } 
        else {
            Color::RGB c(globalTheme.amber);
            c.darkenTo((float)pingIndicatorTimer / pingIndicatorTimeout);
            lv_obj_set_style_text_color(pingStatusLabel, c.toLV(), 0);
        }
    }

    // Update auto-ping broadcast.
    pingBroadcastTimer -= dt;
    
    if (pingBroadcastTimer <= 0) {
        pingBroadcastTimer = random(minPingBroadcastTimeout, maxPingBroadcastTimeout);    
        device->messenger->ping();
    }
}

//////////////////////////////////////////
// Implementation and events
//////////////////////////////////////////
void fatalError(const char* message) {
    LOGLN(message);

    keyboard.setDisplayBrightness(0.5);
    keyboard.setKeyboardBrightness(0.05);

    display.setTextWrap(true);
    display.setTextSize(1);
    display.fillScreen(0);
    display.setTextColor(Color::RGB(globalTheme.amber).packed565());
    display.setCursor(0, 0);

    display.println(message);

    while(true) { 
        delay(100); 
    }
}

bool createSettingsFile() {
    File32 file;
    
    if (!file.open(settingsFilename, O_CREAT | O_RDWR)) {
        LOGLN("Failed to create settings.cfg");
        return false;
    }

    size_t bytesWritten = file.write((uint8_t*)&settings, sizeof(settings));

    file.close();

    if (bytesWritten == 0) {
        LOGLN("Failed to write to settings.cfg");
        return false;
    }

    return true;
}

bool restoreSettingsDefaults() {
    settings.setDefaults();
    
    if (!sdCard.remove(settingsFilename)) {
        LOGLN("Failed to delete corrupt settings.cfg");
        return false;
    }

    if (!createSettingsFile()) {
        LOGLN("Failed to create new settings.cfg");
        return false;
    }

    return true;
}

bool loadSettings() {
    if (sdCard.exists(settingsFilename)) {
        LOGLN("settings.cfg found, reading...");

        File32 file;
        if (!file.open(settingsFilename, O_RDONLY)) {
            LOGLN("Failed to open settings.cfg");
            return false;            
        }

        size_t bytesRead = file.readBytes((uint8_t*)&settings, sizeof(settings));
        file.close();

        if (bytesRead != sizeof(settings)) {
            LOGLN("Corrupted settings.cfg detected. Restoring settings to defaults.");
            return restoreSettingsDefaults();
        }

        if (settings.version() != settings.currentVersion) {
            LOGFMT("Settings version mismatch, loaded: %d, expected: %d. Restoring settings to defaults.\n", settings.version(), settings.currentVersion);
            return restoreSettingsDefaults();
        }

        if (settings.configuredRadio() != Settings::configuredRadioType) {
            LOGFMT("Base radio type mismatch, loaded: %s, expected: %s. Restoring settings to defaults.\n", 
                (settings.configuredRadio() == Settings::RadioType::espNow) ? "ESP-NOW" : "LoRa", 
                (settings.configuredRadioType == Settings::RadioType::espNow) ? "ESP-NOW" : "LoRa");

            return restoreSettingsDefaults();
        }        
    } 
    else {
        LOGLN("settings.cfg not found, creating...");
        return createSettingsFile();
    }

    return true;
}

bool saveSettings() {
    if (!settingsInitialized) {
        return false;
    }

    if (sdCard.exists(settingsFilename)) {
        LOGLN("settings.cfg found, deleting...");

        if (!sdCard.remove(settingsFilename)) {
            LOGLN("Failed to delete settings.cfg, aborting save.");
            return false;
        }
    }

    return createSettingsFile();
}

bool saveMessageHistory() {
    if (!sdCardInitialized) {
        LOGLN("Failed to save message history: SD card reader not initialized");
        return false;
    }

    if (sdCard.exists(messageHistoryFilename)) {
        LOGLN("Deleting existing message history...");

        if (!deleteMessageHistory()) {
            LOGLN("Failed to delete existing message history.");
            return false;
        }
    }    

    if (messageHistory.isEmpty()) {
        // Nothing else to do!
        return true;
    }

    File32 file;
    
    if (!file.open(messageHistoryFilename, O_CREAT | O_RDWR)) {
        LOGLN("Failed to create messages.hst");
        return false;
    }
    
    const int messageCount = messageHistory.size();
    bool wroteAllMessages = true;
    Message msg;
    size_t bytesWritten;

    // Write version
    bytesWritten = file.write(MessageHistory::currentVersion);

    if (bytesWritten != 1) {
        LOGLN("Failed to write message history version");
    }

    // Write message count
    bytesWritten = file.write(messageCount);

    if (bytesWritten != 1) {
        LOGLN("Failed to write message count");
        file.close();
        return false;
    }

    for (int i = 0; i < messageCount; i++) {
        msg = messageHistory.getMessage(i);

        // Write the message sender
        bytesWritten = file.write(uint8_t(msg.sender));

        if (bytesWritten != 1) {
            LOGFMT("Failed to write sender for message %d\n", i);
            file.close();
            return false;
        }

        // Write the message length
        uint8_t messageLength = strlen(msg.text);
        bytesWritten = file.write(messageLength);

        if (bytesWritten != 1) {
            LOGFMT("Failed to write message length for message %d\n", i);
            file.close();
            return false;
        }

        // Write the message
        bytesWritten = file.write(&msg.text, messageLength);

        if (bytesWritten != messageLength) {
            LOGFMT("Failed to write string for message %d\n", i);
            file.close();
            return false;
        }
    }

    file.close();

    LOGLN("Message history saved!");
    return true;
}

bool deleteMessageHistory() {
    if (!sdCardInitialized) {
        LOGLN("Failed to delete message history: SD card reader not initialized");
        return false;
    }

    return sdCard.remove(messageHistoryFilename);
}

void closeMessageHistoryFileAndDelete(File32& f) {
    f.close();

    if (!sdCard.remove(messageHistoryFilename)) {
        LOGLN("Failed to delete old message history");
    }    
}

bool loadMessageHistory() {
    if (!sdCardInitialized) {
        LOGLN("Failed to load message history: SD card reader not initialized");
        return false;
    }

    if (!sdCard.exists(messageHistoryFilename)) {
        LOGLN("Message history does not yet exist.");
        return true;        
    }

    LOGLN("messages.hst found, reading...");
    
    File32 file;
    if (!file.open(messageHistoryFilename, O_RDONLY)) {
        LOGLN("Failed to open messages.hst");
        return false;            
    }

    uint8_t version = 0;
    uint8_t messageCount = 0;

    // First byte is version.
    int readByte = file.read();

    if (readByte == -1) {
        LOGLN("Failed to read message history version. Deleting corrupt message history.");
        closeMessageHistoryFileAndDelete(file);
        return false;
    }

    version = readByte;

    // If the version doesn't match the current version, delete the file and return.
    if (version != MessageHistory::currentVersion) {
        LOGFMT("Message history file version mismatch. Loaded %d, expected %d. Deleting old version.\n", version, MessageHistory::currentVersion);
        closeMessageHistoryFileAndDelete(file);
        return false;
    }

    // Next byte is the message count
    readByte = file.read();

    if (readByte == -1) {
        LOGLN("Failed to read message count. Deleting corrupt message history.");        
        closeMessageHistoryFileAndDelete(file);
        return false;
    }

    messageCount = readByte;

    // Now it's time for the messages
    Message::Sender sender = Message::Sender::me;
    uint8_t messageLength = 0;
    char messageBuffer[Message::bufferSize];

    for (int i = 0; i < messageCount; i++) {
        // Message sender is the first byte
        readByte = file.read();

        if (readByte == -1) {
            LOGFMT("Failed to read message sender for message %d. Deleting corrupt message history.\n", i);
            closeMessageHistoryFileAndDelete(file);
            return false;
        }

        sender = Message::Sender((uint8_t)readByte);

        // Somehow we got an invalid sender value, the file is corrupt.
        if (sender != Message::Sender::me && sender != Message::Sender::them) {
            LOGFMT("Read invalid sender for message %d. Deleting corrupt message history.\n", i);
            closeMessageHistoryFileAndDelete(file);
            return false;            
        }

        // Next byte is the message string length
        readByte = file.read();

        if (readByte == -1) {
            LOGFMT("Failed to read message length for message %d. Deleting corrupt message history.\n", i);
            closeMessageHistoryFileAndDelete(file);
            return false;
        }

        messageLength = readByte;

        // Now read in 'messageLength' bytes. That's our message!
        size_t bytesRead = file.read(messageBuffer, messageLength);

        if (bytesRead != messageLength) {
            LOGFMT("Failed to read message text for message %d. Deleting corrupt message history.\n", i);
            closeMessageHistoryFileAndDelete(file);
            return false;            
        }

        // Null terminate it
        messageBuffer[messageLength] = 0;

        // And add it to the message history
        messageHistory.addMessage(sender, messageBuffer);
    }

    file.close();

    LOGLN("Message history loaded!");

    return true;
}

void initLVGL() {
    lv_init();
    lv_tick_set_cb(lvglTick);

    globalTheme.init();

    // Set up lvgl display
    {
        lvglDrawBuffer = (uint8_t*)malloc(drawBufferSize);
        lv_display_t* disp = lv_display_create(device->displayWidth, device->displayHeight);
        lv_display_set_flush_cb(disp, lvglFlushDisplay);
        lv_display_set_buffers(disp, lvglDrawBuffer, NULL, drawBufferSize, LV_DISPLAY_RENDER_MODE_PARTIAL);    
    }    

    // lvgl input device and keyboard group for keyboard input.
    {
        device->lvglKeyboardIndev = lv_indev_create();
        lv_indev_set_type(device->lvglKeyboardIndev, LV_INDEV_TYPE_KEYPAD);
        lv_indev_set_read_cb(device->lvglKeyboardIndev, lvglKeyboardRead);

        device->lvglKeyboardGroup = lv_group_create();
        lv_indev_set_group(device->lvglKeyboardIndev, device->lvglKeyboardGroup);
    }

    // lvgl input device for touch screen
    {
        lv_indev_t* indev = lv_indev_create();
        lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
        lv_indev_set_read_cb(indev, lvglTouchpadRead);
    }

    // lvgl input devices for each bar button
    {
        for (int i = 0; i < device->barButtonCount; i++) {
            lv_indev_t* indev = lv_indev_create();
            lv_indev_set_type(indev, LV_INDEV_TYPE_BUTTON);
            lv_indev_set_read_cb(indev, lvglBarButtonRead);
            lv_indev_set_button_points(indev, &barButtonIndevTouchPoints[i]);            
            // Leave indev disabled; it will be enabled when a button "connects" to it.
            lv_indev_enable(indev, false);            
            barButtonIndevs[i] = indev;
        }
    }

    // Radio mode label
    {
        lv_obj_t* label = lv_label_create(lv_layer_top());
        lv_obj_add_style(label, &globalTheme.statusBarText, 0);
        lv_obj_align(label, LV_ALIGN_TOP_LEFT, 3, 3);

         if (settings.activeRadio() == Settings::RadioType::lora) {
            lv_label_set_text(label, "LoRa");
         }
         else {
            lv_label_set_text(label, "ESP-NOW");
         }

         radioModeStatusLabel = label;
    }    

    // Ping label
    {
        lv_obj_t* label = lv_label_create(lv_layer_top());
        lv_obj_add_style(label, &globalTheme.statusBarText, 0);
        lv_obj_align(label, LV_ALIGN_TOP_LEFT, 3, 3);
        lv_label_set_text(label, "Ping!");
        lv_obj_add_flag(label, LV_OBJ_FLAG_HIDDEN);
        pingStatusLabel = label;
    }

    // New message label
    {
        lv_obj_t* label = lv_label_create(lv_layer_top());
        lv_obj_add_style(label, &globalTheme.statusBarText, 0);
        lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 3);
        lv_label_set_text(label, "New!");
        lv_obj_add_flag(label, LV_OBJ_FLAG_HIDDEN);
        newMessageStatusLabel = label;
    }    

    // Battery indicator
    {
        batteryIndicatorCanvasBuffer = (uint8_t*)malloc(batteryIndicatorCanvasBufferSize);

        lv_obj_t* canvas = lv_canvas_create(lv_layer_top());
        lv_canvas_set_buffer(canvas, batteryIndicatorCanvasBuffer, batteryIndicatorCanvasWidth, batteryIndicatorCanvasHeight, LV_COLOR_FORMAT_RGB565);
        lv_obj_align(canvas, LV_ALIGN_TOP_RIGHT, 0, 0);
        batteryIndicatorCanvas = canvas;

        lv_obj_t* label = lv_label_create(lv_layer_top());
        lv_obj_add_style(label, &globalTheme.statusBarText, 0);
        lv_obj_align(label, LV_ALIGN_TOP_RIGHT, -batteryIndicatorCanvasWidth, 3);
        batteryStatusLabel = label;
        drawBatteryIndicator();
    }
}

void drawBatteryIndicator() {
    uint8_t pct = batteryMonitor.getPercentage();

    // Update text
    char batteryBuffer[16] = {0};
    const int32_t batteryBufferSize = sizeof(batteryBuffer);
    snprintf(batteryBuffer, batteryBufferSize, "%d%%", pct);
    // snprintf(batteryBuffer, batteryBufferSize, "%d", batteryMonitor.getVoltage());
    batteryBuffer[batteryBufferSize - 1] = 0;
    lv_label_set_text(batteryStatusLabel, batteryBuffer);

    // Draw battery image to canvas
    lv_canvas_fill_bg(batteryIndicatorCanvas, lv_color_black(), LV_OPA_COVER);

    // Get set up to draw some rectangles into the canvas.
    lv_layer_t layer;
    lv_canvas_init_layer(batteryIndicatorCanvas, &layer);
    lv_draw_rect_dsc_t dsc;

    // Draw body
    lv_draw_rect_dsc_init(&dsc);
    dsc.bg_color = lv_color_black();
    dsc.border_color = globalTheme.amber;
    dsc.border_width = 1;
    dsc.radius = 4;

    lv_area_t coords = {3, 5, 29, 15};
    lv_draw_rect(&layer, &dsc, &coords);

    // Draw fill
    int32_t xFill = map(pct, 0, 100, 5, 27);
    lv_draw_rect_dsc_init(&dsc);
    dsc.border_width = 0;
    dsc.radius = 2;    
    dsc.bg_color = (pct > 20) ? globalTheme.amber : lv_color_make(255, 0, 0);
    coords = {5, 7, xFill, 13};
    lv_draw_rect(&layer, &dsc, &coords);

    // Draw terminal
    lv_draw_rect_dsc_init(&dsc);
    dsc.bg_color = globalTheme.amber;
    dsc.border_width = 0;
    dsc.radius = 0;
    coords = {31, 9, 32, 11};
    lv_draw_rect(&layer, &dsc, &coords);    

    // We're done!
    lv_canvas_finish_layer(batteryIndicatorCanvas, &layer);    
}

// Device callbacks
void flushInputEvents() {
    while (keyboard.keyCount() > 0) {
        (void)keyboard.keyEvent();
    }

    keyboard.clearInterruptStatus();

    while (touchpad.touched()) {
        (void)touchpad.getPoint();
    }    

    for (int i = 0; i < Device::barButtonCount; i++) {
        lvglBarButtonDown[i] = false;
    }
}


// LVGL callbacks and event handlers
uint32_t lvglTick() {
    return millis();
}

void lvglFlushDisplay(lv_display_t* disp, const lv_area_t* area, uint8_t* pixels) {
    uint32_t w = lv_area_get_width(area);
    uint32_t h = lv_area_get_height(area);    

    // If we receive a LoRa transmission while drawing large batches
    // of pixels on the SPI display, one of the libraries/hardware
    // systems seems to be hanging, triggering the interrupt watchdog
    // timer. According to the comments in RH_RF95.h, disabling
    // interrupts when interacting with other SPI devices may be
    // needed. So far I've not had issues with the SD card,
    // but I can pretty reliably get the device to crash by
    // changing to a new screen and sending a message on the other
    // device in close intervals. Disabling interrupts while
    // performing the actual draw operation seems to work around the
    // issue. In theory we shouldn't miss any transmissions since
    // we're using RHReliableDatagram, and as soon as the redraw is 
    // complete the radio can service it's interrupts again and
    // dispatch the payload to our callback.
    cli();
    display.drawRGBBitmap(area->x1, area->y1, (uint16_t*)pixels, w, h);
    sei();

    lv_display_flush_ready(disp);
}

void lvglTouchpadRead(lv_indev_t* indev, lv_indev_data_t* data ) {
    if (touchpad.touched()) {
        const TS_Point p = touchpad.getPoint();
        int16_t x = map(p.y, TS_MINY, TS_MAXY, 0, device->displayWidth);        
        int16_t y = map(p.x, TS_MINX, TS_MAXX, 0, device->displayHeight);
        data->point.x = x;
        data->point.y = device->displayHeight - y;
        data->state = LV_INDEV_STATE_PRESSED;  
        // LOGFMT("raw: %d, %d, mapped: %d, %d\n", p.x, p.y, x, y);                  
    }
    else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

lv_indev_t* getBarButtonIndev(uint8_t index) {
    if (index >= Device::barButtonCount) {
        return nullptr;
    }

    return barButtonIndevs[index];
}

// Enables indev for the specified button, and sets 
// its touch point to the center of the provide lvgl object.
void connectBarButton(lv_obj_t* btn, Device::BarButton bb) {
    uint8_t index = uint8_t(bb);

    if (index >= Device::barButtonCount) {
        LOGLN("connectBarButton: Bar button index out of range");
        return;
    }

    if (barButtonIndevs[index] == nullptr) {
        LOGLN("connectBarButton: Bar button indev does not exist");
        return;
    }

    lv_coord_t x = lv_obj_get_x(btn);
    lv_coord_t y = lv_obj_get_y(btn);
    lv_coord_t w = lv_obj_get_width(btn);
    lv_coord_t h = lv_obj_get_height(btn);

    lv_coord_t xCenter = x + (w / 2);
    lv_coord_t yCenter = y + (h / 2);

    // Update touch points for button indev
    barButtonIndevTouchPoints[index].x = xCenter;
    barButtonIndevTouchPoints[index].y = yCenter;
    // LOGFMT("bar button %d touch point: %d, %d\n", index, xCenter, yCenter);

    lv_indev_enable(barButtonIndevs[index], true);
}

// Disables the indev for given bb.
void disconnectBarButton(Device::BarButton bb) {
    uint8_t index = uint8_t(bb);

    if (index >= Device::barButtonCount) {
        return;
    }

    if (barButtonIndevs[index] == nullptr) {
        return;
    }
        
    lv_indev_enable(barButtonIndevs[index], false);
}

void showNewIndicator(bool visible) {
    if (visible) {
        lv_obj_remove_flag(newMessageStatusLabel, LV_OBJ_FLAG_HIDDEN);
    }
    else {
        lv_obj_add_flag(newMessageStatusLabel, LV_OBJ_FLAG_HIDDEN);
    }
}

void lvglBarButtonRead(lv_indev_t* indev, lv_indev_data_t* data) {
    for (int barButtonIndex = 0; barButtonIndex < Device::barButtonCount; barButtonIndex++) {
        if (getBarButtonIndev(barButtonIndex) == indev) {
            if (lvglBarButtonDown[barButtonIndex]) {
                data->state = LV_INDEV_STATE_PRESSED;
            }
            else {
                data->state = LV_INDEV_STATE_RELEASED;
            }

            break;
        }
    }
}

int32_t lvglGetBarButtonKeyIndex(char key) {
    for (int barButtonIndex = 0; barButtonIndex < Device::barButtonCount; barButtonIndex++) {
        if (barButtonKeys[barButtonIndex] == key) {
            return barButtonIndex;
        }
    }

    return -1;
}

void lvglSetBarButtonState(uint8_t barButtonIndex, const BBQ10Keyboard::KeyEvent& e) {
    if (e.state == BBQ10Keyboard::StatePress) {
        lvglBarButtonDown[barButtonIndex] = true;
    }
    else if (e.state == BBQ10Keyboard::StateRelease) {
        lvglBarButtonDown[barButtonIndex] = false;
    }                            
}

void lvglKeyboardRead(lv_indev_t* indev, lv_indev_data_t* data) {
    if (keyboard.keyCount() == 0) {
        if (lvglKeyHeld) {
            uint32_t now = millis();

            if (now - lvglKeyPressedTimestamp >= lvglKeyRepeatInterval) {
                lvglKeyPressedTimestamp = now;
                data->key = lvglLastKeyPressed;
                data->state = LV_INDEV_STATE_PRESSED;
            }
        }

        return;
    }

    const BBQ10Keyboard::KeyEvent e = keyboard.keyEvent();
    bool sendToLVGL = true;
    bool keyRepeats = true;
    int32_t barButtonIndex = lvglGetBarButtonKeyIndex(e.key);

    // "Bar buttons"
    if (barButtonIndex != -1) {
        sendToLVGL = false;
        keyRepeats = false;
        lvglSetBarButtonState(barButtonIndex, e);
    }
    // Up
    else if (e.key == 1) {
        data->key = LV_KEY_UP;
    }
    // Down
    else if (e.key == 2) {
        data->key = LV_KEY_DOWN;
    }        
    // Left
    else if (e.key == 3) {
        data->key = LV_KEY_LEFT;
    }
    // Right
    else if (e.key == 4) {
        data->key = LV_KEY_RIGHT;
    }
    // Center button
    else if (e.key == 5) {
        keyRepeats = false;
        data->key = LV_KEY_NEXT;
    }
    // All other keys
    else {
        data->key = e.key;
    }

    if (sendToLVGL) {
        if (e.state == BBQ10Keyboard::StatePress) {
            data->state = LV_INDEV_STATE_PRESSED;
        }
        else if (e.state == BBQ10Keyboard::StateLongPress) {
            if (keyRepeats && !lvglKeyHeld) {
                lvglKeyHeld = true;
                lvglKeyPressedTimestamp = millis();
                lvglLastKeyPressed = data->key;
            }
        }
        else if (e.state == BBQ10Keyboard::StateRelease) {
            data->state = LV_INDEV_STATE_RELEASED;
            lvglKeyHeld = false;
        }
    }
}

// Other event handlers and callbacks
void batteryPercentageChanged(uint8_t pct) {
    LOGFMT("battery pct: %d\n", pct);    
    drawBatteryIndicator();
}

void messengerPayloadReceived(const uint8_t* payload, uint32_t len) {
    char message[len+1];
    memcpy(message, payload, len);
    message[len] = 0;

    device->messageHistory.addMessage(Message::Sender::them, message);
    (void)saveMessageHistory();
    sceneManager.receivedMessage(message);
}

void messengerPingCallback() {
    pingIndicatorActive = true;
    pingIndicatorTimer = 0;

    lv_obj_set_style_text_color(pingStatusLabel, globalTheme.amber, 0);
    lv_obj_remove_flag(pingStatusLabel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(radioModeStatusLabel, LV_OBJ_FLAG_HIDDEN);
}