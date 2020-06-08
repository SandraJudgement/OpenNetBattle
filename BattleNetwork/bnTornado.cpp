#include "bnTornado.h"
#include "bnBusterHit.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"

Tornado::Tornado(Field* _field, Team _team, int damage) : damage(damage), Spell(_field, _team) {
  SetLayer(-1);

  setTexture(Textures().GetTexture(TextureType::SPELL_TORNADO));
  setScale(2.f, 2.f);

  //When the animation ends, delete this
  auto onFinish = [this]() {
    Delete();
  };

  animation = Animation("resources/spells/spell_tornado.animation");
  animation.SetAnimation("DEFAULT");
  animation << onFinish;
  animation.Update(0, getSprite());

  HighlightTile(Battle::Tile::Highlight::solid);

  auto props = GetHitboxProperties();
  props.flags &= ~Hit::recoil;
  props.flags |= Hit::impact;
  props.damage = damage;
  props.element = Element::wind;
  SetHitboxProperties(props);
}

Tornado::~Tornado() {
}

void Tornado::OnUpdate(float _elapsed) {
  setPosition(tile->getPosition().x, tile->getPosition().y - 20.0f);

  animation.Update(_elapsed, getSprite());

  tile->AffectEntities(this);
}

bool Tornado::Move(Direction _direction) {
  return false;
}

void Tornado::Attack(Character* _entity) {
  if (_entity->Hit(GetHitboxProperties())) {
    Audio().Play(AudioType::HURT);

    // Todo swap out with normal buster hit fx
    Artifact* hitfx = new BusterHit(GetField(), BusterHit::Type::CHARGED);
    GetField()->AddEntity(*hitfx, _entity->GetTile()->GetX(), _entity->GetTile()->GetY());
  }
}

void Tornado::OnDelete()
{
  Remove();
}
