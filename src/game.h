#pragma once

class Game;
class StateBase;

#include <SFML/Graphics.hpp>
#define _USE_MATH_DEFINES
#include <cmath>
#include <memory>
#include <stack>
#include <atomic>

#include "states/statebase.h"
#include "ai/gradientdescent.h"
#include "entities/enums.h"
#include "entities/pipe.h"
#include "entities/thwomp.h"
#include "entities/vehicleproperties.h"
#include "map/coin.h"
#include "map/oilslick.h"
#include "map/questionpanel.h"
#include "map/ramphorizontal.h"
#include "map/rampvertical.h"
#include "map/zipper.h"


class Game {
   private:static const int WINDOW_STYLE = sf::Style::Titlebar | sf::Style::Close;
    const int baseWidth, baseHeight;

    int tryPop;
    std::stack<StatePtr> stateStack;

    protected: // <--- NEW SECTION: For derived classes like NetworkedGameClient
    sf::RenderWindow window;       
    int framerate;                   
    std::atomic<bool> gameEnded;            

    StatePtr getCurrentState() const; 
    void handleTryPop();              
    
    // THESE MUST BE VIRTUAL for the override keyword to work in NetworkedGameClient
    virtual void handleEvents(const StatePtr& currentState);

   public:
    Game(const int _bx, const int _by, const int _framerate = 60);
    // main game loop until game closed event
    virtual void run();

    void pushState(const StatePtr& statePtr);
    void popStatesUntil(unsigned int i);
    void popState();

    const sf::RenderWindow& getWindow() const;
    void getCurrentResolution(unsigned int& width, unsigned int& height);
    virtual void updateResolution();
    virtual bool fixedUpdate(sf::Time fixedUpdateStep) {
    return false; 
};
};