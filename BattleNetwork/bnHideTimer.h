#pragma once

#include "bnComponent.h"
#include <functional>
#include <SFML/System.hpp>

class Character;

namespace Battle {
    class Tile;
}

/*! \brief Removes any entity from the field for a duration of time
 * 
 * This component injects itself into the battle scene loop directly
 * Once the timer runs down, it ejects itself from the battle scene loop
 * and deletes itself
*/
class HideTimer : public Component {
private:
  double duration; /*!< How long to hide a character for */
  double elapsed; /*!< time elapsed in seconds */
  Battle::Tile* temp; /*!< The tile to return the character to */
  std::function<void()> respawn;
public:
  /**
   * @brief Registers the owner and sets the time in seconds
   */
  HideTimer(std::weak_ptr<Character> owner, double secs);

  /**
   * @brief When time is up, eject component from battle scene loop and add entity back
   * @param _elapsed in seconds
   */
  void OnUpdate(double _elapsed) override;
  
  /**
   * @brief Remove ownership from character to the battle scene and remove entity from play
   */
  void Inject(BattleSceneBase&) override;
}; 
