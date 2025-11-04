#pragma once

#include <SFML/Graphics.hpp>

enum class Key : int {
    // Driving actions
    ACCELERATE,
    BRAKE,
    DRIFT,
    TURN_LEFT,
    TURN_RIGHT,
    ITEM_FRONT,
    ITEM_BACK,
    // Menu actions
    PAUSE,
    // CONTINUE,  // [[ deprecated ]]
    ACCEPT,
    CANCEL,
    MENU_UP,
    MENU_DOWN,
    MENU_LEFT,
    MENU_RIGHT,
    __COUNT
};

class Input {
   private:
    static Input instance;
    sf::Keyboard::Key map[(int)Key::__COUNT];
    const sf::RenderWindow *gameWindow;
    bool inputsDisabled;

    Input() : gameWindow(nullptr), inputsDisabled(false) {}

   public:

    static inline void disableInputs(bool disable) {
        instance.inputsDisabled = disable;
    }
    
    static inline bool areInputsDisabled() {
        return instance.inputsDisabled;
    }

    // set window for focus checks
    static inline void setGameWindow(const sf::RenderWindow &window) {
        instance.gameWindow = &window;
    }

    // Read/write the key map
    static inline void set(const Key action, const sf::Keyboard::Key code) {
        instance.map[(int)action] = code;
    }
    static inline const sf::Keyboard::Key &get(Key action) {
        return instance.map[(int)action];
    }

    // Check for key press/release/hold events
    static inline bool pressed(const Key action, const sf::Event &event) {
        return event.type == sf::Event::KeyPressed &&
               event.key.code == get(action);
    }
    static inline bool released(const Key action, const sf::Event &event) {
        return event.type == sf::Event::KeyReleased &&
               event.key.code == get(action);
    }
    static inline bool held(const Key action) {
            
            if (instance.inputsDisabled) {
                // Define which keys are DRIVING controls (to block)
                bool isDrivingControl = 
                    action == Key::ACCELERATE ||
                    action == Key::BRAKE ||
                    action == Key::DRIFT ||
                    action == Key::TURN_LEFT || 
                    action == Key::TURN_RIGHT ||
                    action == Key::ITEM_FRONT ||
                    action == Key::ITEM_BACK;
                
                if (isDrivingControl) {
                    return false; // Block driving inputs when disabled
                }
                // Allow menu controls (PAUSE, ACCEPT, etc.) even when disabled
            }

            if (!instance.gameWindow) {
        return sf::Keyboard::isKeyPressed(get(action));
        std::cout << "[DEBUG] gameWindow is nullptr\n";
    } else {
    std::cout << "[DEBUG] gameWindow addr = " << instance.gameWindow << ", isOpen=" 
              << instance.gameWindow->isOpen() << "\n";
}
            
            
            return sf::Keyboard::isKeyPressed(get(action)) &&
                instance.gameWindow->hasFocus();
        }
    // returns true if key is accepted in game
    static std::string getActionName(const Key action);

    // code based on:
    // https://en.sfml-dev.org/forums/index.php?topic=15226.0
    // returns true if key is accepted in game
    static std::string getKeyCodeName(const sf::Keyboard::Key code);
};