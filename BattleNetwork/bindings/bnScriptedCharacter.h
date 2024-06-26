#pragma once
#ifdef BN_MOD_SUPPORT

#include <sol/sol.hpp>
#include "dynamic_object.h"
#include "../bnCharacter.h"
#include "../bnAI.h"
#include "../bnSolHelpers.h"
#include "bnScriptedCardAction.h"
#include "bnWeakWrapper.h"

class AnimationComponent;
class ScriptedCharacterState;
class ScriptedIntroState;

/**
 * @class ScriptedCharacter
 * @author mav
 * @date 02/10/21
 * @brief Character reads from lua files for behavior and settings
 */
class ScriptedCharacter final : public Character, public AI<ScriptedCharacter>, public dynamic_object {
  friend class ScriptedCharacterState;
  friend class ScriptedIntroState;
  float height{};
  std::shared_ptr<AnimationComponent> animation{ nullptr };
  bool bossExplosion{ false };
  double explosionPlayback{ 1.0 };
  int numOfExplosions{ 2 };
  WeakWrapper<ScriptedCharacter> weakWrap;
public:
  using DefaultState = ScriptedCharacterState;

  ScriptedCharacter(Character::Rank rank);
  ~ScriptedCharacter();
  void Init();
  void InitFromScript(sol::state& script);
  void OnSpawn(Battle::Tile& start) override;
  void OnBattleStart() override;
  void OnBattleStop() override;
  void OnUpdate(double _elapsed) override ;
  const float GetHeight() const override;
  void SetHeight(const float height);
  void OnDelete() override;
  bool CanMoveTo(Battle::Tile * next) override;
  void RegisterStatusCallback(const Hit::Flags& flag, const StatusCallback& callback);
  void ShakeCamera(double power, float duration);
  void OnCountered() override;
  Animation& GetAnimationObject();
  void SetExplosionBehavior(int num, double speed, bool isBoss);

  sol::object update_func;
  sol::object delete_func;
  sol::object on_spawn_func;
  sol::object battle_start_func;
  sol::object battle_end_func;
  sol::object can_move_to_func;
  sol::object on_countered_func;
};

class ScriptedCharacterState : public AIState<ScriptedCharacter> {
public:
  void OnEnter(ScriptedCharacter& s) override {
  }

  void OnUpdate(double elapsed, ScriptedCharacter& s) override {
    if (s.update_func.valid())
    {
      auto result = CallLuaCallback(s.update_func, s.weakWrap, elapsed);

      if (result.is_error()) {
        Logger::Log(LogLevel::critical, result.error_cstr());
      }
    }
  }

  void OnLeave(ScriptedCharacter& s) override {

  }
};

class ScriptedIntroState : public AIState<ScriptedCharacter> {
  void OnEnter(ScriptedCharacter& context) override {

  }

  void OnUpdate(double _elapsed, ScriptedCharacter& context) override {

  }

  void OnLeave(ScriptedCharacter& context) override {

  }
};

#endif