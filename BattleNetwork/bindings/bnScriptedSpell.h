#pragma once
#ifdef BN_MOD_SUPPORT

#include <sol/sol.hpp>
#include "dynamic_object.h"
#include "../bnSpell.h"
#include "../bnAnimationComponent.h"
#include "bnWeakWrapper.h"

using sf::Texture;

/**
 * @class ScriptedSpell
*/
class ScriptedSpell final : public Spell, public dynamic_object {
public:
  ScriptedSpell(Team _team);
  ~ScriptedSpell();
  
  void Init() override;
  void OnUpdate(double _elapsed) override;
  void OnDelete() override;
  void OnCollision(const std::shared_ptr<Entity> other) override;
  bool CanMoveTo(Battle::Tile * next) override;
  void Attack(std::shared_ptr<Entity> e) override;
  void OnSpawn(Battle::Tile& spawn) override;
  const float GetHeight() const;
  void SetHeight(const float height);
  void ShowShadow(const bool shadow);

  void SetAnimation(const std::string& path);
  Animation& GetAnimationObject();
  // duration in seconds
  void ShakeCamera(double power, float duration);
  void NeverFlip(bool enabled);

  std::function<void(WeakWrapper<ScriptedSpell>, Battle::Tile&)> spawnCallback;
  std::function<void(WeakWrapper<ScriptedSpell>, WeakWrapper<Entity>)> attackCallback;
  std::function<void(WeakWrapper<ScriptedSpell>, WeakWrapper<Entity>)> collisionCallback;
  std::function<bool(Battle::Tile&)> canMoveToCallback;
  std::function<void(WeakWrapper<ScriptedSpell>)> deleteCallback;
  std::function<void(WeakWrapper<ScriptedSpell>, double)> updateCallback;
private:
  bool flip{true};
  float height{};
  sf::Vector2f scriptedOffset{};
  SpriteProxyNode* shadow{ nullptr };
  std::shared_ptr<AnimationComponent> animComponent{ nullptr };
};
#endif