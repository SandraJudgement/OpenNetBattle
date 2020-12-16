#include "bnShineExplosion.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnField.h"
#include "bnTile.h"

using sf::IntRect;

ShineExplosion::ShineExplosion(Field* _field, Team _team) : Artifact(_field)
{
  SetLayer(0);
  field = _field;
  team = _team;
  setTexture(LOAD_TEXTURE(MOB_BOSS_SHINE));
  setScale(2.f, 2.f);

  animationComponent = CreateComponent<AnimationComponent>(this);
  animationComponent->SetPath("resources/mobs/boss_shine.animation");
  animationComponent->Load();
  animationComponent->SetAnimation("SHINE", Animator::Mode::Loop);
  animationComponent->OnUpdate(0.0f);
}

void ShineExplosion::OnUpdate(double _elapsed) {
  setPosition(GetTile()->getPosition().x, GetTile()->getPosition().y - 30.f);
}

void ShineExplosion::OnDelete()
{
}

bool ShineExplosion::Move(Direction _direction)
{
  return false;
}

ShineExplosion::~ShineExplosion()
{
}