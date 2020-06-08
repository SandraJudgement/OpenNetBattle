#include "bnMegaman.h"
#include "bnShaderResourceManager.h"
#include "bnBusterCardAction.h"
#include "bnCrackShotCardAction.h"
#include "bnFireBurnCardAction.h"
#include "bnTornadoCardAction.h"
#include "bnCardAction.h"
#include "bnSpriteProxyNode.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"

#include "bnWind.h"
#include "bnPaletteSwap.h"

Megaman::Megaman() : Player() {
  auto basePallete = Textures().LoadTextureFromFile("resources/navis/megaman/forms/base.palette.png");
  PaletteSwap* pswap = CreateComponent<PaletteSwap>(this, basePallete);

  SetHealth(900);
  SetName("Megaman");
  setTexture(Textures().GetTexture(TextureType::NAVI_MEGAMAN_ATLAS));

  AddForm<TenguCross>()->SetUIPath("resources/navis/megaman/forms/tengu_entry.png");
  AddForm<HeatCross>()->SetUIPath("resources/navis/megaman/forms/heat_entry.png");
  AddForm<TomahawkCross>()->SetUIPath("resources/navis/megaman/forms/hawk_entry.png");
}

Megaman::~Megaman()
{
}

void Megaman::OnUpdate(float elapsed)
{
  Player::OnUpdate(elapsed);
}

CardAction* Megaman::ExecuteBusterAction()
{
  return new BusterCardAction(*this, false, 1);
}

CardAction* Megaman::ExecuteChargedBusterAction()
{
  if (activeForm) {
    return activeForm->OnChargedBusterAction(*this);
  }
  else {
    return new BusterCardAction(*this, true, 10);
  }
}

CardAction* Megaman::ExecuteSpecialAction() {
  if (activeForm) {
    return activeForm->OnSpecialAction(*this);
  }

  return nullptr;
}

// CROSSES / FORMS

TenguCross::TenguCross()
{
  parentAnim = nullptr;
  loaded = false;
}

TenguCross::~TenguCross()
{
  delete overlay;
}

void TenguCross::OnActivate(Player& player)
{
  ResourceHandle handle;

  overlayAnimation = Animation("resources/navis/megaman/forms/tengu_cross.animation");
  overlayAnimation.Load();
  auto cross = handle.Textures().LoadTextureFromFile("resources/navis/megaman/forms/tengu_cross.png");
  overlay = new SpriteProxyNode();
  overlay->setTexture(cross);
  overlay->SetLayer(-1);
  overlay->EnableParentShader(false);

  parentAnim = player.GetFirstComponent<AnimationComponent>();
  auto pswap = player.GetFirstComponent<PaletteSwap>();

  pswap->LoadPaletteTexture("resources/navis/megaman/forms/tengu.palette.png");

  loaded = true;

  OnUpdate(0.01f, player);

  player.AddNode(overlay);

  parentAnim->AddToOverrideList(&overlayAnimation);
}

void TenguCross::OnDeactivate(Player & player)
{
  player.RemoveNode(overlay);
  auto pswap = player.GetFirstComponent<PaletteSwap>();
  pswap->Revert();

  parentAnim->RemoveFromOverrideList(&overlayAnimation);

}

void TenguCross::OnUpdate(float elapsed, Player& player)
{
  parentAnim->SyncAnimation(overlayAnimation);
  overlayAnimation.Refresh(overlay->getSprite());

  overlay->setColor(player.getColor());

  // update node position in the animation
  auto baseOffset = parentAnim->GetPoint("Head");
  auto origin = player.getOrigin();
  baseOffset = baseOffset - origin;

  overlay->setPosition(baseOffset);
}

CardAction* TenguCross::OnChargedBusterAction(Player& player)
{
  return new BusterCardAction(player, true, 10);
}

CardAction* TenguCross::OnSpecialAction(Player& player)
{
  return new TenguCross::SpecialAction(player);
}

// HEAT CROSS

HeatCross::HeatCross()
{
  parentAnim = nullptr;
  loaded = false;
}

HeatCross::~HeatCross()
{
  delete overlay;
}

void HeatCross::OnActivate(Player& player)
{
  ResourceHandle handle;

  overlayAnimation = Animation("resources/navis/megaman/forms/heat_cross.animation");
  overlayAnimation.Load();
  auto cross = handle.Textures().LoadTextureFromFile("resources/navis/megaman/forms/heat_cross.png");
  overlay = new SpriteProxyNode();
  overlay->setTexture(cross);
  overlay->SetLayer(-1);
  overlay->EnableParentShader(false);

  parentAnim = player.GetFirstComponent<AnimationComponent>();
  auto pswap = player.GetFirstComponent<PaletteSwap>();

  pswap->LoadPaletteTexture("resources/navis/megaman/forms/heat.palette.png");

  player.SetElement(Element::fire);

  loaded = true;

  OnUpdate(0.01f, player);
  player.AddNode(overlay);

  parentAnim->AddToOverrideList(&overlayAnimation);

}

void HeatCross::OnDeactivate(Player & player)
{
  player.RemoveNode(overlay);
  auto pswap = player.GetFirstComponent<PaletteSwap>();
  pswap->Revert();

  player.SetElement(Element::none);

  parentAnim->RemoveFromOverrideList(&overlayAnimation);

}

void HeatCross::OnUpdate(float elapsed, Player& player)
{
  overlay->setColor(player.getColor());

  parentAnim->SyncAnimation(overlayAnimation);
  overlayAnimation.Refresh(overlay->getSprite());

  // update node position in the animation
  auto baseOffset = parentAnim->GetPoint("Head");
  auto origin = player.getSprite().getOrigin();
  baseOffset = baseOffset - origin;

  overlay->setPosition(baseOffset);
}

CardAction* HeatCross::OnChargedBusterAction(Player& player)
{
  return new FireBurnCardAction(player, FireBurn::Type::_2, 60);
}

CardAction* HeatCross::OnSpecialAction(Player& player)
{
  return nullptr;
}

// TOMAHAWK CROSS


TomahawkCross::TomahawkCross()
{
  parentAnim = nullptr;
  loaded = false;
}

TomahawkCross::~TomahawkCross()
{
  delete overlay;
}

void TomahawkCross::OnActivate(Player& player)
{
  ResourceHandle handle;

  overlayAnimation = Animation("resources/navis/megaman/forms/hawk_cross.animation");
  overlayAnimation.Load();
  auto cross = handle.Textures().LoadTextureFromFile("resources/navis/megaman/forms/hawk_cross.png");
  overlay = new SpriteProxyNode();
  overlay->setTexture(cross);
  overlay->SetLayer(-1);
  overlay->EnableParentShader(false);

  parentAnim = player.GetFirstComponent<AnimationComponent>();
  auto pswap = player.GetFirstComponent<PaletteSwap>();

  pswap->LoadPaletteTexture("resources/navis/megaman/forms/hawk.palette.png");

  loaded = true;

  OnUpdate(0.01f, player);
  player.AddNode(overlay);

  parentAnim->AddToOverrideList(&overlayAnimation);
  player.SetAirShoe(true);

}

void TomahawkCross::OnDeactivate(Player & player)
{
  player.RemoveNode(overlay);
  auto pswap = player.GetFirstComponent<PaletteSwap>();
  pswap->Revert();

  parentAnim->RemoveFromOverrideList(&overlayAnimation);
  player.SetAirShoe(false);
}

void TomahawkCross::OnUpdate(float elapsed, Player& player)
{
  overlay->setColor(player.getColor());

  parentAnim->SyncAnimation(overlayAnimation);
  overlayAnimation.Refresh(overlay->getSprite());

  // update node position in the animation
  auto baseOffset = parentAnim->GetPoint("Head");
  auto origin = player.getSprite().getOrigin();
  baseOffset = baseOffset - origin;

  overlay->setPosition(baseOffset);
}

CardAction* TomahawkCross::OnChargedBusterAction(Player& player)
{
  return new CrackShotCardAction(player, 30);
}

CardAction* TomahawkCross::OnSpecialAction(Player& player)
{
  return nullptr;
}

// SPECIAL ABILITY IMPLEMENTATIONS
#define FRAME1 { 1, 0.05 }
#define FRAME2 { 2, 0.05 }
#define FRAME3 { 3, 0.3 }

#define FRAMES FRAME1, FRAME2, FRAME3

TenguCross::SpecialAction::SpecialAction(Character& user) : CardAction(user, "PLAYER_SWORD") {
  attachment = new SpriteProxyNode();
  attachment->setTexture(user.getTexture());
  attachment->SetLayer(-1);
  attachment->EnableParentShader(true);

  OverrideAnimationFrames({ FRAMES });

  attachmentAnim = Animation(anim->GetFilePath());
  attachmentAnim.SetAnimation("HAND");

  AddAttachment(anim->GetAnimationObject(), "hilt", *attachment).PrepareAnimation(attachmentAnim);
}

TenguCross::SpecialAction::~SpecialAction()
{
}

void TenguCross::SpecialAction::OnExecute()
{
  auto& user = GetUser();
  auto team = user.GetTeam();
  auto field = user.GetField();

  // On throw frame, spawn projectile
  auto onThrow = [this, &user, team, field]() -> void {
    auto wind = new Wind(field, team);
    field->AddEntity(*wind, 6, 1);

    wind = new Wind(field, team);
    field->AddEntity(*wind, 6, 2);

    wind = new Wind(field, team);
    field->AddEntity(*wind, 6, 3);
  };

  AddAction(3, onThrow);
}

void TenguCross::SpecialAction::OnEndAction()
{
  GetUser().RemoveNode(attachment);
}
