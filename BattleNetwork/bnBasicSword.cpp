#include <random>
#include <time.h>

#include "bnBasicSword.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnPlayer.h"
#include "bnProgsMan.h"
#include "bnMettaur.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"

#define COOLDOWN 40.0/1000.0

#define BULLET_ANIMATION_SPRITES 3
#define BULLET_ANIMATION_WIDTH 30
#define BULLET_ANIMATION_HEIGHT 27

BasicSword::BasicSword(Field* _field, Team _team, int _damage) : 
  damage(_damage),
  hitHeight(0),
  Spell(_field, _team){
  hit = false;
  cooldown = 0;

  HighlightTile(Battle::Tile::Highlight::solid);

  auto  props = GetHitboxProperties();;
  props.damage = _damage;
  props.flags |= Hit::flinch;
  SetHitboxProperties(props);
}

BasicSword::~BasicSword(void) {
}

void BasicSword::OnUpdate(double _elapsed) {
  if (cooldown >= COOLDOWN) {
    Delete();
    return;
  }

  tile->AffectEntities(this);

  cooldown += _elapsed;
}

bool BasicSword::Move(Direction _direction) {
  return false;
}

void BasicSword::Attack(Character* _entity) {
  hit = hit || _entity->Hit(GetHitboxProperties());
  hitHeight = _entity->GetHeight();
}

void BasicSword::OnDelete()
{
  Remove();
}
