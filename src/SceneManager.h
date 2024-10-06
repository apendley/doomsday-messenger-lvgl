#pragma once

#include <Arduino.h>

class Scene;

class SceneManager {
public:
    ~SceneManager();

    // Update the current scene.
    void update(uint32_t dt);

    // Transition to another scene
    void gotoScene(Scene* s);

    // Event handling
    void receivedMessage(const char* message);

private:
    Scene* currentScene = nullptr;
};
