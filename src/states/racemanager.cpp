#include "racemanager.h"

// #define NO_ANIMATIONS  // remove race begin/end animations from the game

void StateRaceManager::resetBeforeRace() {
    Lakitu::reset();
    Gui::reset(false);
    EndRanks::reset(&positions);
    StateRace::currentTime = sf::Time::Zero;
    for (unsigned int i = 0; i < positions.size(); i++) {
        sf::Vector2f pos;
        if (ccOption == CCOption::NO_BOTS && i > 0) {
            // Place disabled bots at the same position as player but they won't move
            // Since they're DISABLED, they won't render or update
            pos = Map::getPlayerInitialPosition(1);
            pos.x = pos.x / MAP_ASSETS_WIDTH;
            pos.y = pos.y / MAP_ASSETS_HEIGHT;
        } else {
            pos = Map::getPlayerInitialPosition(i + 1);
            pos.x = pos.x / MAP_ASSETS_WIDTH;
            pos.y = pos.y / MAP_ASSETS_HEIGHT;
        }
        positions[i]->setPositionAndReset(pos);
        
        // Set invisible state AFTER reset (setPositionAndReset calls reset() which clears state)
        if (ccOption == CCOption::NO_BOTS && i > 0) {
            // Use a very large time value to keep them invisible for the entire race
            positions[i]->pushStateEnd(DriverState::INVISIBLE, sf::seconds(999999.0f));
        }
    }
}

void StateRaceManager::setPlayer() {
    constexpr int count = (int)MenuPlayer::__COUNT;
    // apply player character multiplier to player vehicle
    drivers[(int)selectedPlayer]->vehicle =
        &drivers[(int)selectedPlayer]->vehicle->makePlayer();
    
    if (ccOption == CCOption::NO_BOTS) {
        // NO_BOTS mode: Only the player participates
        // Disable all bots (they'll be made invisible in resetBeforeRace)
        for (int i = 0; i < count; i++) {
            if (i == (int)selectedPlayer) {
                drivers[i]->controlType = DriverControlType::PLAYER;
            } else {
                drivers[i]->controlType = DriverControlType::DISABLED;
            }
        }
        // Put player in first position
        positions[0] = drivers[(int)selectedPlayer].get();
        // Fill remaining positions with the disabled bots
        int posIdx = 1;
        for (int i = 0; i < count; i++) {
            if (i != (int)selectedPlayer) {
                positions[posIdx++] = drivers[i].get();
            }
        }
    } else {
        // Normal mode with bots
        for (int i = 0; i < count; i++) {
#ifdef NO_ANIMATIONS
            drivers[i]->controlType = DriverControlType::DISABLED;
#else
            drivers[i]->controlType = DriverControlType::AI_GRADIENT;
#endif
        }
        // move selected player to last position
        std::swap(positions[(int)selectedPlayer], positions[count - 1]);
        std::random_shuffle(positions.begin(), positions.begin() + (count - 1));
    }
}

void StateRaceManager::init(const float _speedMultiplier,
                            const float _playerCharacterMultiplier,
                            const RaceCircuit _circuit) {
    VehicleProperties::setScaleFactor(_speedMultiplier,
                                      _playerCharacterMultiplier);
    Lakitu::reset();
    Gui::reset(true);
    currentCircuit = _circuit;

    unsigned int modifiersIndexer[(unsigned int)MenuPlayer::__COUNT] = {0, 1, 2, 3, 4, 5, 6, 7};
    std::shuffle(std::begin(modifiersIndexer), std::end(modifiersIndexer), randGen);

    for (unsigned int i = 0; i < (unsigned int)MenuPlayer::__COUNT; i++) {
        DriverPtr driver(new Driver(
            DRIVER_ASSET_NAMES[i].c_str(), sf::Vector2f(0.0f, 0.0f),
            M_PI_2 * -1.0f, MAP_ASSETS_WIDTH, MAP_ASSETS_HEIGHT,
            DriverControlType::DISABLED, *DRIVER_PROPERTIES[i], MenuPlayer(i),
            positions, true, FAR_VISIONS[(int)ccOption][modifiersIndexer[i]],
            ITEM_PROB_MODS[(int)ccOption][modifiersIndexer[i]],
            IMPEDIMENTS[(int)ccOption][modifiersIndexer[i]]));
        drivers[i] = driver;
        positions[i] = driver.get();
        grandPrixRanking[i] = std::make_pair(driver.get(), 0);
    };
    currentState = RaceState::NO_PLAYER;
}

bool StateRaceManager::update(const sf::Time &) {
    switch (currentState) {
        case RaceState::NO_PLAYER:
            Audio::play(Music::MENU_PLAYER_CIRCUIT);
            game.pushState(
                StatePtr(new StatePlayerSelection(game, selectedPlayer)));
            currentState = RaceState::YES_PLAYER;
            break;
        case RaceState::YES_PLAYER:
            setPlayer();
            currentState = RaceState::RACING;
            break;
        case RaceState::RACING: {
            int i = (int)currentCircuit;
            currentCircuit = RaceCircuit(i + 1);
            drivers[(unsigned int)selectedPlayer]->controlType =
                DriverControlType::PLAYER;
            Map::loadCourse(CIRCUIT_ASSET_NAMES[i]);
            Audio::loadCircuit(CIRCUIT_ASSET_NAMES[i]);
            resetBeforeRace();
            unsigned int currentPlayerPosition = 0;
            for (unsigned int i = 0; i < positions.size(); i++) {
                if (positions[i]->getPj() == selectedPlayer) {
                    currentPlayerPosition = i;
                    break;
                }
            }
#ifndef NO_ANIMATIONS
            game.pushState(StatePtr(
                new StateRaceEnd(game, drivers[(unsigned int)selectedPlayer],
                                 drivers, selectedPlayer, positions)));
#endif
            StateRace::ccOption = ccOption;
            Driver::realPlayer = drivers[(unsigned int)selectedPlayer];
            game.pushState(StatePtr(
                new StateRace(game, drivers[(unsigned int)selectedPlayer],
                              drivers, positions)));
#ifndef NO_ANIMATIONS
            Audio::play(Music::CIRCUIT_ANIMATION_START, false);

            sf::Vector2f cameraInitPosition =
                Map::getPlayerInitialPosition(currentPlayerPosition + 1);
            Audio::updateListener(cameraInitPosition, -M_PI_2, 0.0f);
            for (unsigned int i = 0; i < drivers.size(); i++) {
                // Skip disabled bots in NO_BOTS mode
                if (ccOption == CCOption::NO_BOTS && drivers[i]->controlType == DriverControlType::DISABLED) {
                    continue;
                }
                Audio::updateEngine(i, drivers[i]->position, drivers[i]->height,
                                    0.0f, 0.0f);
            }
            Audio::playEngines((int)selectedPlayer, false);

            game.pushState(StatePtr(new StateRaceStart(
                game, drivers[(unsigned int)selectedPlayer], drivers,
                cameraInitPosition, RaceCircuit(i), ccOption)));
#endif
            currentState = RaceState::STANDINGS;
        } break;
        case RaceState::STANDINGS: {
            Audio::stopEngines();
            RaceCircuit lastCircuit =
                RaceCircuit((unsigned int)currentCircuit - 1);
            if ((mode == RaceMode::GRAND_PRIX_1 &&
                 currentCircuit == RaceCircuit::__COUNT) ||
                mode == RaceMode::VERSUS) {
                if (mode == RaceMode::VERSUS) {
                    // "grand prix ranking" equals the race ranking
                    unsigned int numActiveDrivers = (ccOption == CCOption::NO_BOTS) ? 1 : positions.size();
                    for (unsigned int i = 0; i < numActiveDrivers; i++) {
                        grandPrixRanking[i].first = positions[i];
                    }
                }
                currentState = RaceState::CONGRATULATIONS;
            }
            if (mode == RaceMode::GRAND_PRIX_1) {
                game.pushState(StatePtr(
                    new StateGPStandings(game, positions, grandPrixRanking,
                                         lastCircuit, selectedPlayer)));
                if (currentCircuit != RaceCircuit::__COUNT) {
                    currentState = RaceState::RACING;
                }
            }
        } break;
        case RaceState::CONGRATULATIONS: {
            RaceCircuit lastCircuit =
                RaceCircuit((unsigned int)currentCircuit - 1);
            game.pushState(StatePtr(
                new StateCongratulations(game, lastCircuit, mode, ccOption,
                                         selectedPlayer, grandPrixRanking)));
            currentState = RaceState::DONE;
        } break;
        case RaceState::DONE:
            // pop player selection & racemanager
            game.popState();
            game.popState();
            break;
    }

    return true;
}