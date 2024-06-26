#include "bnDefenseBubbleWrap.h"
#include "bnEntity.h"
#include "bnField.h"
#include "bnSpell.h"
#include "bnHitboxSpell.h"

DefenseBubbleWrap::DefenseBubbleWrap() : popped(false), DefenseRule(Priority(0), DefenseOrder::always)
{
}

DefenseBubbleWrap::~DefenseBubbleWrap()
{
}

const bool DefenseBubbleWrap::IsPopped() const
{
  return popped;
}

Hit::Properties& DefenseBubbleWrap::FilterStatuses(Hit::Properties& statuses) {
  if(statuses.element == Element::elec)
    statuses.damage *= 2;
  return statuses;
}

void DefenseBubbleWrap::CanBlock(DefenseFrameStateJudge& judge, std::shared_ptr<Entity> attacker, std::shared_ptr<Entity> owner)
{
  if ((attacker->GetHitboxProperties().flags & Hit::impact) == 0) return;

  // weak obstacles will break like other bubbles
  auto hitbox = std::make_shared<HitboxSpell>(owner->GetTeam(), 0);
  owner->GetField()->AddEntity(hitbox, *owner->GetTile());

  auto props = attacker->GetHitboxProperties();
  if ((props.flags & Hit::impact) == Hit::impact) {

    if (attacker->GetElement() != Element::elec) {
      judge.BlockDamage();
    }
    else {
      judge.SignalDefenseWasPierced();
    }

    popped = true;
  }
}
