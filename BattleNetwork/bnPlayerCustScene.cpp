#include "bnPlayerCustScene.h"
#include "bnGameSession.h"
#include "netplay/bnBufferWriter.h"
#include "netplay/bnBufferReader.h"
#include "stx/string.h"
#include <Segues/BlackWashFade.h>

constexpr size_t BAD_GRID_POS = std::numeric_limits<size_t>::max();
constexpr size_t MAX_ALLOWED_TYPES = 4u;
constexpr size_t MAX_ITEMS_ON_SCREEN = 4u; // includes RUN button
constexpr float GRID_START_X = 4.f;
constexpr float GRID_START_Y = 23.f;
constexpr sf::Uint8 PROGRESS_MAX_ALPHA = 200;

using namespace swoosh::types;
using Blocks = PlayerCustScene::Piece::Types;

PlayerCustScene::Piece* generateRandomBlock() {
  PlayerCustScene::Piece* p = new PlayerCustScene::Piece();

  size_t x{};
  bool first = true;

  while (x == 0) {
    // generate smaller 3x3 shapes
    for (size_t i = 1; i < 4; i++) { // start one row down
      for (size_t j = 1; j < 4; j++) { // start one column over
        size_t index = (i * p->BLOCK_SIZE) + j;
        uint8_t k = rand() % 2;
        p->shape[index] = k;
        x += k;
      }
    }
  }

  p->calculateDimensions();

  p->typeIndex = rand() % static_cast<uint8_t>(Blocks::size);
  p->specialType = rand() % 2;
  p->name = stx::rand_alphanum(8);
  p->description = stx::rand_alphanum(30);

  p->description[9] = '\n';
  p->description[19] = '\n';
  return p;
}

PlayerCustScene::PlayerCustScene(swoosh::ActivityController& controller, const std::string& playerUUID, std::vector<Piece*> pieces) :
  playerUUID(playerUUID),
  pieces(pieces),
  infoText(Font::Style::thin),
  itemText(Font::Style::thick),
  hoverText(Font::Style::thick),
  textbox(sf::Vector2f(4, 200)),
  questionInterface(nullptr),
  Scene(controller)
{
  cursorLocation = ((GRID_SIZE / 2) * GRID_SIZE) + (GRID_SIZE / 2);
  gridAnim = Animation("resources/scenes/cust/grid.animation");
  trackAnim = Animation("resources/scenes/cust/track.animation") << "DEFAULT";
  clawAnim = Animation("resources/scenes/cust/claw.animation");
  blockAnim = Animation("resources/scenes/cust/block.animation");
  blockShadowHorizAnim = Animation("resources/scenes/cust/horizontal_shadow.animation");
  blockShadowVertAnim = Animation("resources/scenes/cust/vertical_shadow.animation");
  buttonAnim = Animation("resources/scenes/cust/button.animation") << "DEFAULT";
  cursorAnim = Animation("resources/scenes/cust/red_cursor.animation") << "DEFAULT" << Animator::Mode::Loop;
  menuAnim = Animation("resources/scenes/cust/menu.animation") << "MOVE";

  textbox.SetTextSpeed(1.0f);
  gotoNextScene = true;

  // Selection input delays
  maxSelectInputCooldown = 0.25; // 4th of a second
  selectInputCooldown = maxSelectInputCooldown;

  auto load_audio = [this](const std::string& path) {
    return Audio().LoadFromFile(path);
  };

  compile_start = load_audio("resources/sfx/compile_start.ogg");
  compile_complete = load_audio("resources/sfx/compile_complete.ogg");
  compile_no_item = load_audio("resources/sfx/compile_empty_item.ogg");
  compile_item = load_audio("resources/sfx/compile_item.ogg");

  auto load_texture = [this](const std::string& path) {
    return Textures().LoadFromFile(path);
  };

  cursorTexture = load_texture("resources/ui/textbox_cursor.png");
  itemArrowCursor = sf::Sprite(*cursorTexture);

  sf::FloatRect bounds = itemArrowCursor.getLocalBounds();
  itemArrowCursor.setScale(2.f, 2.f);
  itemArrowCursor.setOrigin({ bounds.width, 0. });

  bg = sf::Sprite(*load_texture("resources/scenes/cust/bg.png"));
  bg.setScale(2.f, 2.f);

  sceneLabel = sf::Sprite(*load_texture("resources/scenes/cust/scene_label.png"));
  sceneLabel.setPosition(sf::Vector2f(20.f, 8.0f));
  sceneLabel.setScale(2.f, 2.f);

  cursor = sf::Sprite(*load_texture("resources/scenes/cust/red_cursor.png"));
  cursor.setScale(2.f, 2.f);
  cursor.setTextureRect({ 0, 0, 16, 16 });

  claw = sf::Sprite(*load_texture("resources/scenes/cust/claw.png"));
  claw.setScale(2.f, 2.f);
  claw.setTextureRect({ 38, 0, 14, 14 });

  gridSprite = sf::Sprite(*load_texture("resources/scenes/cust/grid.png"));
  gridSprite.setTextureRect({ 0, 0, 127, 130 });
  gridSprite.setPosition((GRID_START_X*2.)+12, (GRID_START_Y*2.)-8);
  gridSprite.setScale(2.f, 2.f);
  
  for (uint8_t i = 0; i < static_cast<uint8_t>(Blocks::size); i++) {
    blockTypeInUseTable[i] = 0u;
    blockTextures.push_back(load_texture("resources/scenes/cust/cust_blocks_" + std::to_string(i) + ".png"));
  }

  blockShadowHorizontal = sf::Sprite(*load_texture("resources/scenes/cust/horiz_shadow.png"));
  blockShadowHorizontal.setScale(2.f, 2.f);
  blockShadowVertical = sf::Sprite(*load_texture("resources/scenes/cust/vert_shadow.png"));
  blockShadowVertical.setScale(2.f, 2.f);

  sf::IntRect buttonRect = {0, 0, 73, 19};
  blueButtonSprite = sf::Sprite(*load_texture("resources/scenes/cust/item_name_container.png"));
  blueButtonSprite.setTextureRect(buttonRect);
  blueButtonSprite.setScale(2.f, 2.f);

  greenButtonSprite = sf::Sprite(*load_texture("resources/scenes/cust/run.png"));
  greenButtonSprite.setTextureRect(buttonRect);
  greenButtonSprite.setScale(2.f, 2.f);

  infoBox = sf::Sprite(*load_texture("resources/scenes/cust/info.png"));
  infoBox.setScale(2.f, 2.f);
  infoBox.setPosition({ 280, 188 });

  previewBox = sf::Sprite(*load_texture("resources/scenes/cust/mini_block_container.png"));
  previewBox.setScale(2.f, 2.f);

  menuBox = sf::Sprite(*load_texture("resources/scenes/cust/menu.png"));
  menuBox.setScale(2.f, 2.f);
  menuBox.setPosition(GRID_START_X + 100.f, GRID_START_Y + 100.f);

  miniblocksTexture = load_texture("resources/scenes/cust/mini_blocks.png");

  sf::Vector2f pos = gridSprite.getPosition();
  pos.x += 22.f;
  pos.y += 121.f;
  track = sf::Sprite(*load_texture("resources/scenes/cust/track.png"));
  track.setScale(2.f, 2.f);
  track.setPosition(pos);

  // info text
  sf::Vector2f textPosition = infoBox.getPosition();
  textPosition.y += 20.f;
  textPosition.x += 8.f;
  infoText.setPosition(textPosition);
  infoText.setScale(2.f, 2.f);

  // item text
  itemText.setScale(2.f, 2.f);

  // hover text copies item text
  hoverText = itemText;

  // progress bar data
  auto progressBarTexture = load_texture("resources/scenes/cust/progress_bar.png");
  progressBarTexture->setRepeated(true);

  progressBarUVs = { 0, 0, 118, 14 };
  progressBar = sf::Sprite(*progressBarTexture);
  progressBar.setScale(2.f, 2.f);
  progressBar.setPosition((GRID_START_X * 2.) + 20, 168);
  progressBar.setTextureRect(progressBarUVs); // source is 69x14 pixels
  progressBar.setColor(sf::Color(255, 255, 255, PROGRESS_MAX_ALPHA));

  // ensure piece dimensions are up-to-date
  for (auto& p : pieces) {
    p->calculateDimensions();
  }

  // set screen view
  setView(sf::Vector2u(480, 320));

  // load initial layout from profile data
  loadFromSave();
}

PlayerCustScene::~PlayerCustScene()
{
  for (Piece* p : pieces) {
      delete p;
  }

  pieces.clear();
}

// cheaper check to see if piece can fit inside the region the grid cursor is located
bool PlayerCustScene::canPieceFit(Piece* piece, size_t loc)
{
  size_t x = loc % GRID_SIZE;
  size_t y = loc / GRID_SIZE;

  if (x == 0 && piece->startX <= 1)
    return false;
  else if (x == 1 && piece->startX == 0)
    return false;
  else if (x == 5 && (piece->startX + piece->maxWidth) == Piece::BLOCK_SIZE)
    return false;
  else if (x == 6 && (piece->startX + piece->maxWidth) >= Piece::BLOCK_SIZE - 1u)
    return false;

  if (y == 0 && piece->startY <= 1)
    return false;
  else if (y == 1 && piece->startY == 0)
    return false;
  else if (y == 5 && (piece->startY + piece->maxHeight) == Piece::BLOCK_SIZE)
    return false;
  else if (y == 6 && (piece->startY + piece->maxHeight) >= Piece::BLOCK_SIZE - 1u)
    return false;

  return true;
}

bool PlayerCustScene::doesPieceOverlap(Piece* piece, size_t loc)
{
  size_t start = getPieceStart(piece, loc);

  bool entirelyOutOfBounds = true; // assume

  for (size_t i = piece->startY; i < piece->maxHeight+piece->startY; i++) {
    size_t idx = 0;
    for (size_t j = piece->startX; j < piece->maxWidth+piece->startX; j++) {
      if (piece->shape[(i * Piece::BLOCK_SIZE) + j]) {
        size_t x = (start + idx) % GRID_SIZE;
        size_t y = (start + idx) / GRID_SIZE;

        if (x > 0 && x + 1u < GRID_SIZE && y > 0 && y + 1u < GRID_SIZE) {
          entirelyOutOfBounds = false;
        }

        // prevent inserting corner spots
        bool tl = (x == 0 && y == 0);
        bool tr = (x + 1u == GRID_SIZE && y == 0);
        bool bl = (x == 0 && y + 1u == GRID_SIZE);
        bool br = (x + 1u == GRID_SIZE && y + 1u == GRID_SIZE);
        bool corner = tl || tr || bl || br;
        if (grid[start + idx] || corner) return true;
      }
      idx++;
    }
    start += GRID_SIZE; // next row
  }

  if (piece->specialType && entirelyOutOfBounds) return true;

  return false;
}

bool PlayerCustScene::insertPiece(Piece* piece, size_t loc)
{
  if (!canPieceFit(piece, loc) || doesPieceOverlap(piece, loc))
    return false;

  size_t start = getPieceStart(piece, loc);

  for (size_t i = piece->startY; i < piece->maxHeight+piece->startY; i++) {
    size_t idx = 0;

    for (size_t j = piece->startX; j < piece->maxWidth+piece->startX; j++) {
      if (piece->shape[(i * Piece::BLOCK_SIZE) + j]) {
        grid[start + idx] = piece;
      }
      idx++;
    }
    start += GRID_SIZE; // next row
  }

  centerHash[piece] = loc;

  // update the block types in use table
  blockTypeInUseTable[piece->typeIndex]++;

  piece->commit();

  return true;
}

void PlayerCustScene::removePiece(Piece* piece)
{
  size_t index = getPieceCenter(piece);
  if (index == BAD_GRID_POS) return;

  size_t start = getPieceStart(piece, index);

  bool scan = true;

  for (size_t i = piece->startY; i < piece->maxHeight+piece->startY && scan; i++) {
    size_t idx = 0;

    for (size_t j = piece->startX; j < piece->maxWidth+piece->startX && scan; j++) {
      // crash prevention, shouldn't be necessary if everything is programmed correctly
      scan = (start + idx) < grid.size();

      if (scan && piece->shape[(i * Piece::BLOCK_SIZE) + j]) {
        grid[start + idx] = nullptr;
      }

      idx++;
    }
    start += GRID_SIZE;
  }

  // update the block types in use table
  blockTypeInUseTable[piece->typeIndex]--;

  centerHash[piece] = BAD_GRID_POS;
}

void PlayerCustScene::drawEdgeBlock(sf::RenderTarget& surface, Piece* piece, size_t y, size_t x)
{
  // TODO: make arrow and left block sprite
  if ((x == 0 || x + 1u == GRID_SIZE) && y == 3) return;

  sf::Vector2f pos = blockToScreen(y, x);

  // assume if false, it is a horizontal piece
  bool vert = x == 0 || x + 1u == GRID_SIZE;

  if ((state == state::compiling || state == state::finishing) && compiledHash[piece]) {
    if (vert) {
      if (blockShadowVertAnim.GetAnimationString() != "BLINK") {
        blockShadowVertAnim << "BLINK" << Animator::Mode::Loop;
      }
    }
    else {
      if (blockShadowHorizAnim.GetAnimationString() != "BLINK") {
        blockShadowHorizAnim << "BLINK" << Animator::Mode::Loop;
      }
    }
  }
  else {
    if (vert) {
      if (blockShadowVertAnim.GetAnimationString() != "DIMMED") {
        blockShadowVertAnim.SetAnimation("DIMMED");
      }
    }
    else {
      if (blockShadowHorizAnim.GetAnimationString() != "DIMMED") {
        blockShadowHorizAnim.SetAnimation("DIMMED");
      }
    }
  }

  if (x == 0) {
    pos.x += 26.f;
  }

  if (y == 0) {
    pos.y += 26.f;
  }

  if (vert) {
    blockShadowVertical.setPosition(pos);
    blockShadowVertAnim.Refresh(blockShadowVertical);
    surface.draw(blockShadowVertical);
  }
  else {
    blockShadowHorizontal.setPosition(pos);
    blockShadowHorizAnim.Refresh(blockShadowHorizontal);
    surface.draw(blockShadowHorizontal);
  }
}

bool PlayerCustScene::isGridEdge(size_t y, size_t x)
{
  return x == 0 || x == GRID_SIZE-1u || y == GRID_SIZE-1u || y == 0;
}

void PlayerCustScene::startCompile()
{
  textbox.Close();
  questionInterface = nullptr;
  infoText.SetString("RUNNING...");
  progress = 0;
  compiledHash.clear();
  state = state::compiling;
  sf::Color color = sf::Color(255, 255, 255, PROGRESS_MAX_ALPHA);
  progressBarUVs.width = 0;
  progressBar.setTextureRect(progressBarUVs);
  progressBar.setColor(color);
  Audio().Play(compile_start);
}

bool PlayerCustScene::isCompileFinished()
{
  return progress >= maxProgressTime; // in seconds
}

void PlayerCustScene::loadFromSave()
{
  std::string value = getController().Session().GetKeyValue(playerUUID + ":" + "blocks");
  if (value.empty()) return;

  Poco::Buffer<char> buffer{ value.c_str(), value.size() };
  BufferReader reader;

  size_t size = reader.Read<size_t>(buffer);
  for (size_t i = 0; i < size; i++) {
    std::string uuid = reader.ReadTerminatedString(buffer);
    bool valid = reader.Read<bool>(buffer); // unused
    size_t center = reader.Read<size_t>(buffer);
    size_t rot = reader.Read<size_t>(buffer);

    for (auto iter = pieces.begin(); iter != pieces.end(); /* skip */) {
      if ((*iter)->uuid == uuid) {
        for (size_t c = 0; c < rot; c++) {
          (*iter)->rotateLeft();
        }

        if (insertPiece(*iter, center)) {
          iter = pieces.erase(iter);
          continue;
        }
      }
      iter++;
    }
  }
}

void PlayerCustScene::completeAndSave()
{
  state = state::waiting;
  infoText.SetString("OK");
  Audio().Play(compile_complete);

  Poco::Buffer<char> buffer{ 0 };
  BufferWriter writer;

  writer.Write(buffer, centerHash.size());
  for (auto& [piece, center] : centerHash) {
    if (center == BAD_GRID_POS) continue;

    bool isValid = isBlockValid(piece);
    writer.WriteTerminatedString(buffer, piece->uuid);
    writer.WriteBytes(buffer, &isValid, sizeof(bool));
    writer.Write(buffer, center);
    writer.Write(buffer, piece->finalRot);
  }

  auto& session = getController().Session();
  
  session.SetKeyValue(playerUUID + ":" + "blocks", std::string(buffer.begin(), buffer.size()));
  session.SaveSession("profile.bin");
}

sf::Vector2f PlayerCustScene::blockToScreen(size_t y, size_t x)
{
  float offsetX = (GRID_START_X * 2) + (x * 20.f * 2.f) - 4.f;
  float offsetY = (GRID_START_Y * 2) + (y * 20.f * 2.f) - 4.f;

  return sf::Vector2f{ offsetX, offsetY };
}

bool PlayerCustScene::hasLeftInput()
{
  return Input().Has(InputEvents::pressed_ui_left) || Input().Has(InputEvents::held_ui_left);
}

bool PlayerCustScene::hasRightInput()
{
  return Input().Has(InputEvents::pressed_ui_right) || Input().Has(InputEvents::held_ui_right);
}

bool PlayerCustScene::hasUpInput()
{
  return Input().Has(InputEvents::pressed_ui_up) || Input().Has(InputEvents::held_ui_up);
}

bool PlayerCustScene::hasDownInput()
{
  return Input().Has(InputEvents::pressed_ui_down) || Input().Has(InputEvents::held_ui_down);
}

bool PlayerCustScene::isBlockValid(Piece* piece)
{
  if (!piece) return false;

  // NOTE: We can use this function to determine any bug statuses as well

  // Restriction: special pieces must be on the "compiled" line. Other pieces do not.
  return (compiledHash[piece] && piece->specialType) || !piece->specialType;
}

size_t PlayerCustScene::getPieceCenter(Piece* piece)
{
  auto iter = centerHash.find(piece);
  if (iter != centerHash.end()) {
    return iter->second;
  }

  return BAD_GRID_POS;
}

size_t PlayerCustScene::getPieceStart(Piece* piece, size_t center)
{
  size_t shape_half = ((Piece::BLOCK_SIZE / 2) * GRID_SIZE) + (GRID_SIZE - Piece::BLOCK_SIZE);
  size_t offset = ((piece->startY * GRID_SIZE) + piece->startX);

  size_t start_top_left = (center - (shape_half - offset));
  size_t start_bottom_right = (center + (offset - shape_half));
  bool afterCenter = offset > shape_half;

  return afterCenter ? start_bottom_right : start_top_left;
}

void PlayerCustScene::handleGrabAction()
{
  state = state::block_prompt;
  menuAnim.SetAnimation("MOVE");
  menuAnim.Refresh(menuBox);
  Audio().Play(AudioType::CHIP_DESC);
  updateMenuPosition();
  updateCursorHoverInfo();
}

void PlayerCustScene::handleMenuUIKeys(double elapsed)
{
  if (hasUpInput()) {
    handleInputDelay(elapsed, &PlayerCustScene::executeUpKey);
    return;
  }

  if (hasDownInput()) {
    handleInputDelay(elapsed, &PlayerCustScene::executeDownKey);
    return;
  }

  if (Input().Has(InputEvents::pressed_cancel)) {
    state = state::usermode;
    Audio().Play(AudioType::CHIP_DESC_CLOSE);
    return;
  }

  Piece* piece = grid[cursorLocation];

  if (!piece) return;

  if (Input().Has(InputEvents::pressed_confirm)) {
    if (menuAnim.GetAnimationString() == "MOVE") {
      // interpolate to the center of the block
      grabStartLocation = getPieceCenter(piece);
      cursorLocation = std::min(grabStartLocation, grid.size() - 1u);

      removePiece(piece);
      grabbingPiece = piece;
      state = state::usermode;
      Audio().Play(AudioType::CHIP_DESC);

      return;
    }

    if (menuAnim.GetAnimationString() == "REMOVE") {
      removePiece(piece);
      pieces.push_back(piece);
      state = state::usermode;
      Audio().Play(AudioType::CHIP_DESC_CLOSE);
      updateCursorHoverInfo();
    }
  }
}

sf::Vector2f PlayerCustScene::gridCursorToScreen()
{
  float cursorLocationX = (cursorLocation % GRID_SIZE) * 20.f * 2.f;
  float cursorLocationY = (cursorLocation / GRID_SIZE) * 20.f * 2.f;
  return { (GRID_START_X * 2) + cursorLocationX, (GRID_START_Y * 2) + cursorLocationY };
}

void PlayerCustScene::drawPiece(sf::RenderTarget& surface, Piece* piece, const sf::Vector2f& pos)
{
  sf::Sprite blockSprite;
  blockSprite.setScale(2.f, 2.f);

  sf::Color color = blockSprite.getColor();
  color.a = 140;
  blockSprite.setColor(color);

  for (size_t i = 0; i < Piece::BLOCK_SIZE; i++) {
    for (size_t j = 0; j < Piece::BLOCK_SIZE; j++) {
      size_t index = (i * Piece::BLOCK_SIZE) + j;
      if (piece->shape[index]) {
        
        float halfOffset = (((Piece::BLOCK_SIZE / 2)) * -20.f)*2.f;
        float offsetX = halfOffset + (j * 20.f * 2.f)-4.f;
        float offsetY = halfOffset + (i * 20.f * 2.f)-4.f;
        blockSprite.setTexture(*blockTextures[piece->typeIndex], true);
        blockSprite.setPosition({ pos.x + offsetX, pos.y + offsetY });
        refreshBlock(piece, blockSprite);
        surface.draw(blockSprite);
      }
    }
  }
}

void PlayerCustScene::drawPreview(sf::RenderTarget& surface, Piece* piece, const sf::Vector2f& pos)
{
  sf::Sprite blockSprite;
  blockSprite.setScale(2.f, 2.f);

  previewBox.setPosition(pos);
  surface.draw(previewBox);

  for (size_t i = 0; i < Piece::BLOCK_SIZE; i++) {
    for (size_t j = 0; j < Piece::BLOCK_SIZE; j++) {
      size_t index = (i * Piece::BLOCK_SIZE) + j;
      if (piece->shape[index]) {
        float offsetX = (j * 3.f * 2.f) + 4.f;
        float offsetY = (i * 3.f * 2.f) + 4.f;
        blockSprite.setTexture(*miniblocksTexture, true);
        blockSprite.setTextureRect({ static_cast<int>(piece->typeIndex*3), 0, 3, 3 });
        blockSprite.setPosition({ pos.x + offsetX, pos.y + offsetY });
        surface.draw(blockSprite);
      }
    }
  }
}

void PlayerCustScene::consolePrintGrid()
{
  for (size_t i = 0; i < GRID_SIZE; i++) {
    for (size_t j = 0; j < GRID_SIZE; j++) {
      if (grid[(i * GRID_SIZE) + j]) {
        std::cout << "X";
      }
      else {
        std::cout << ".";
      }
    }
    std::cout << std::endl;
  }
}

void PlayerCustScene::startScaffolding()
{
  scaffolding = 0.f;
}

void PlayerCustScene::animateButton(double elapsed)
{
  buttonFlashElapsed += elapsed;
  buttonAnim.SyncTime(buttonFlashElapsed);
  buttonAnim.Refresh(blueButtonSprite);
  buttonAnim.Refresh(greenButtonSprite);
}

void PlayerCustScene::animateCursor(double elapsed)
{
  if (grabbingPiece) {
    if (clawAnim.GetAnimationString() != "CLAW_CLOSED") {
      clawAnim.SetAnimation("CLAW_CLOSED");
    }
  }
  else {
    if (clawAnim.GetAnimationString() != "CLAW") {
      clawAnim << "CLAW" << Animator::Mode::Loop;
    }
  }

  cursorAnim.Update(elapsed, cursor);
  clawAnim.Update(elapsed, claw);
}

void PlayerCustScene::animateScaffolding(double elapsed)
{
  if (scaffolding < 1.) {
    scaffolding = std::min(scaffolding + (10.f*static_cast<float>(elapsed)), 1.f);
  }
}

void PlayerCustScene::animateGrid()
{
  if (itemListSelected) {
    if (gridAnim.GetAnimationString() != "DIMMED") {
      gridAnim.SetAnimation("DIMMED");
      gridAnim.Refresh(gridSprite);
    }
  }
  else {
    if (gridAnim.GetAnimationString() != "DEFAULT") {
      gridAnim.SetAnimation("DEFAULT");
      gridAnim.Refresh(gridSprite);
    }
  }
}

void PlayerCustScene::animateBlock(double elapsed, Piece* p)
{
  blockFlashElapsed += elapsed;
  blockShadowVertAnim.SyncTime(blockFlashElapsed);
  blockShadowHorizAnim.SyncTime(blockFlashElapsed);

  if (!p) return;

  if (p == insertingPiece || p == grabbingPiece) {
    if (p->specialType) {
      if (blockAnim.GetAnimationString() != "FLAT_INSERT") {
        blockAnim << "FLAT_INSERT";
      }
    }
    else {
      if (blockAnim.GetAnimationString() != "SQUARE_INSERT") {
        blockAnim << "SQUARE_INSERT";
      }
    }

    blockAnim.SyncTime(blockFlashElapsed);
    return;
  }

  if ((state == state::compiling || state == state::finishing) && compiledHash[p]) {
    if (p->specialType) {
      if (blockAnim.GetAnimationString() != "FLAT_BLINK") {
        blockAnim << "FLAT_BLINK" << Animator::Mode::Loop;
      }
    }
    else {
      if (blockAnim.GetAnimationString() != "SQUARE_BLINK") {
        blockAnim << "SQUARE_BLINK" << Animator::Mode::Loop;
      }
    }
    
    blockAnim.SyncTime(blockFlashElapsed);
    return;
  }

  if (p->specialType) {
    if (blockAnim.GetAnimationString() != "FLAT_DIMMED") {
      blockAnim << "FLAT_DIMMED" << Animator::Mode::Loop;
    }
  }
  else {
    if (blockAnim.GetAnimationString() != "SQUARE_DIMMED") {
      blockAnim << "SQUARE_DIMMED" << Animator::Mode::Loop;
    }
  }

  blockAnim.SyncTime(blockFlashElapsed);
}

void PlayerCustScene::refreshBlock(Piece* p, sf::Sprite& sprite)
{
  animateBlock(0, p);
  blockAnim.Refresh(sprite);
}

void PlayerCustScene::refreshButton(size_t idx)
{
  if (idx == listStart+currItemIndex && itemListSelected && state == state::usermode) {
    if (buttonAnim.GetAnimationString() != "BLINK") {
      buttonAnim << "BLINK" << Animator::Mode::Loop;
    }
  }
  else if (buttonAnim.GetAnimationString() != "DEFAULT") {
    buttonAnim << "DEFAULT";
  }

  blueButtonSprite.setScale(2.f, scaffolding * 2.f);
  greenButtonSprite.setScale(2.f, scaffolding * 2.f);
  animateButton(0);
}

void PlayerCustScene::refreshTrack()
{
  if (itemListSelected) {
    if (trackAnim.GetAnimationString() != "DIMMED") {
      trackAnim.SetAnimation("DIMMED");
    }
  }
  else {
    if (trackAnim.GetAnimationString() != "DEFAULT") {
      trackAnim.SetAnimation("DEFAULT");
    }
  }

  trackAnim.Refresh(track);
}

bool PlayerCustScene::handleSelectItemFromList()
{
  if (listStart >= pieces.size()) return false;

  auto iter = std::next(pieces.begin(), listStart);
  insertingPiece = *iter;
  pieces.erase(iter);

  // move cursor to the center of the grid
  cursorLocation = ((GRID_SIZE / 2) * GRID_SIZE) + (GRID_SIZE/2);
  itemListSelected = false;

  Audio().Play(AudioType::CHIP_DESC);

  return true;
}

void PlayerCustScene::executeLeftKey()
{
  if (itemListSelected) {
    selectGridUI();
    return;
  }

  if (cursorLocation % GRID_SIZE > 0) {
    cursorLocation--;
    updateCursorHoverInfo();
    Audio().Play(AudioType::CHIP_SELECT, AudioPriority::low);
  }
}

void PlayerCustScene::executeRightKey()
{
  if ((cursorLocation + 1) % GRID_SIZE > 0) {
    cursorLocation++;
    updateCursorHoverInfo();
    Audio().Play(AudioType::CHIP_SELECT, AudioPriority::low);
  }
  else if(!grabbingPiece && !insertingPiece) {
    selectItemUI(listStart);
  }
}

void PlayerCustScene::executeUpKey()
{
  if (state == state::block_prompt) {
    if (menuAnim.GetAnimationString() == "MOVE")
      return;

    menuAnim.SetAnimation("MOVE");
    menuAnim.Refresh(menuBox);
    Audio().Play(AudioType::CHIP_SELECT);
    return;
  }

  if (itemListSelected) {
    if (listStart > 0) {
      listStart--;
      updateItemListHoverInfo();
      Audio().Play(AudioType::CHIP_SELECT, AudioPriority::low);
    }
    return;
  }

  if (cursorLocation >= GRID_SIZE) {
    cursorLocation -= GRID_SIZE;
    updateCursorHoverInfo();
    Audio().Play(AudioType::CHIP_SELECT, AudioPriority::low);
  }
}

void PlayerCustScene::executeDownKey()
{
  if (state == state::block_prompt) {
    if (menuAnim.GetAnimationString() == "REMOVE")
      return;

    menuAnim.SetAnimation("REMOVE");
    menuAnim.Refresh(menuBox);
    Audio().Play(AudioType::CHIP_SELECT);

    return;
  }

  if (itemListSelected) {
    // pieces end iterator doubles as the `run` action item
    if (listStart+1u <= pieces.size()) {
      listStart++;
      updateItemListHoverInfo();
      Audio().Play(AudioType::CHIP_SELECT, AudioPriority::low);
    }
    return;
  }

  if (cursorLocation + GRID_SIZE < grid.size()) {
    cursorLocation += GRID_SIZE;
    updateCursorHoverInfo();
    Audio().Play(AudioType::CHIP_SELECT, AudioPriority::low);
  }
}

void PlayerCustScene::executeCancelInsert()
{
  pieces.push_back(insertingPiece);
  insertingPiece = nullptr;
  selectGridUI();
}

void PlayerCustScene::executeCancelGrab()
{
  insertPiece(grabbingPiece, grabStartLocation);
  cursorLocation = grabStartLocation;
  grabbingPiece = nullptr;
  selectGridUI();
}

bool PlayerCustScene::handleUIKeys(double elapsed)
{
  if (hasUpInput()) {
    handleInputDelay(elapsed, &PlayerCustScene::executeUpKey);
    return true;
  }

  if (hasDownInput()) {
    handleInputDelay(elapsed, &PlayerCustScene::executeDownKey);
    return true;
  }

  if (hasLeftInput()) {
    handleInputDelay(elapsed, &PlayerCustScene::executeLeftKey);
    return true;
  }

  if (hasRightInput()) {
    handleInputDelay(elapsed, &PlayerCustScene::executeRightKey);
    return true;
  }

  if (Input().Has(InputEvents::pressed_option) && !itemListSelected) {
    selectItemUI(pieces.size());
    startScaffolding();
    return true;
  }

  if (Input().Has(InputEvents::pressed_cancel)) {
    if (!itemListSelected) {
      selectItemUI(listStart);
      return true;
    }

    if (itemListSelected) {
      auto onYes = [this]() {
        quitScene();
      };

      auto onNo = [this]() {
        selectItemUI(listStart);
      };

      Audio().Play(AudioType::PAUSE, AudioPriority::low);
      questionInterface = new Question("Quit programming?", onYes, onNo);
      textbox.EnqueMessage(questionInterface);
      textbox.Open();
      textbox.CompleteCurrentBlock();
      return true;
    }
  }

  return false;
}

void PlayerCustScene::handleInputDelay(double elapsed, void(PlayerCustScene::*executeFunc)())
{
  selectInputCooldown -= elapsed;

  if (selectInputCooldown <= 0) {
    if (!extendedHold) {
      selectInputCooldown = maxSelectInputCooldown;
      extendedHold = true;
    }
    else {
      selectInputCooldown = maxSelectInputCooldown * 0.25;
    }

    (this->*executeFunc)();
  }
}

void PlayerCustScene::selectGridUI()
{
  if (textbox.IsOpen()) {
    textbox.Close();
    questionInterface = nullptr;
  }

  selectInputCooldown = maxSelectInputCooldown;
  state = state::usermode;
  itemListSelected = false;
  updateCursorHoverInfo();
  Audio().Play(AudioType::CHIP_DESC, AudioPriority::low);
}

void PlayerCustScene::selectItemUI(size_t idx)
{
  if (textbox.IsOpen()) {
    textbox.Close();
    questionInterface = nullptr;
  }

  state = state::usermode;
  selectInputCooldown = maxSelectInputCooldown;

  if (itemListSelected) return;

  itemListSelected = true;
  listStart = idx;
  updateItemListHoverInfo();
  Audio().Play(AudioType::CHIP_DESC_CLOSE, AudioPriority::low);
}

void PlayerCustScene::quitScene()
{
  textbox.Close();

  using effect = swoosh::types::segue<BlackWashFade, milliseconds<500>>;
  getController().pop<effect>();
}

bool PlayerCustScene::handlePieceAction(Piece*& piece, void(PlayerCustScene::* cancelFunc)())
{
  if (Input().Has(InputEvents::pressed_cancel)) {
    piece->revert();
    (this->*cancelFunc)();
    return true;
  }

  if (Input().Has(InputEvents::pressed_confirm)) {
    if (insertPiece(piece, cursorLocation)) {
      piece = nullptr;
      Audio().Play(AudioType::CHIP_DESC_CLOSE);
      updateCursorHoverInfo();
    }
    else {
      Audio().Play(AudioType::CHIP_ERROR, AudioPriority::lowest);
    }
    return true;
  }

  if (Input().Has(InputEvents::pressed_shoulder_left)) {
    piece->rotateLeft();
    Audio().Play(AudioType::CHIP_SELECT, AudioPriority::low);
    return true;
  }

  if (Input().Has(InputEvents::pressed_shoulder_right)) {
    piece->rotateRight();
    Audio().Play(AudioType::CHIP_SELECT, AudioPriority::low);
    return true;
  }

  return false;
}

void PlayerCustScene::updateCursorHoverInfo()
{
  std::string infoTextString = "";

  if (grabbingPiece) {
    infoTextString = grabbingPiece->description;
  }
  else if (Piece* p = grid[cursorLocation]; p && state != state::block_prompt) {
    infoTextString = p->description;
    hoverText.SetString(p->name);

    sf::Vector2f pos = gridCursorToScreen();
    pos.y += 20.f * 2.f; // below the block it is on
    hoverText.setPosition(pos);

    sf::FloatRect bounds = hoverText.GetLocalBounds();

    float x = (cursorLocation % GRID_SIZE)/static_cast<float>(GRID_SIZE) * bounds.width;
    float y = (cursorLocation / GRID_SIZE) + 1u == GRID_SIZE ? 20.f*2.f : 0.f;
    hoverText.setOrigin({ x, y});
  }
  else {
    hoverText.SetString("");
  }

  infoText.SetString(stx::format_to_fit(infoTextString, 11, 3));
}

void PlayerCustScene::updateMenuPosition() {
  sf::Vector2f pos = gridCursorToScreen();

  // some offsets since the cursor sprite isn't centered...
  pos.x += 10.f * 2.f;
  pos.y += 20.f * 2.f;

  float y = (cursorLocation / GRID_SIZE) + 1u >= (GRID_SIZE - 1u) ? 60.f *2.f : 0.f;
  pos.y -= y;

  menuBox.setPosition(pos);
}

void PlayerCustScene::updateItemListHoverInfo()
{
  if (listStart < pieces.size()) {
    infoText.SetString(stx::format_to_fit(pieces[listStart]->description, 11, 3));
  }
  else {
    infoText.SetString("RUN?");
  }
}

void PlayerCustScene::onLeave()
{
}

void PlayerCustScene::onExit()
{
}

void PlayerCustScene::onEnter()
{
}

void PlayerCustScene::onResume()
{
}

void PlayerCustScene::onStart()
{
  gotoNextScene = false;
}

void PlayerCustScene::onUpdate(double elapsed)
{
  textbox.Update(elapsed);
  progressBarUVs.left -= isCompileFinished()?1:2;
  progressBarUVs.width = static_cast<int>(std::min(118., (118.*(progress/maxProgressTime))));
  progressBar.setTextureRect(progressBarUVs);

  sf::Vector2f dest = gridCursorToScreen();
  sf::Vector2f cursorPos = cursor.getPosition();
  cursorPos.x = swoosh::ease::interpolate(0.5f, cursorPos.x, dest.x);
  cursorPos.y = swoosh::ease::interpolate(0.5f, cursorPos.y, dest.y);
  claw.setPosition(cursorPos);
  cursor.setPosition(cursorPos);

  animateGrid();
  animateCursor(elapsed);
  animateButton(elapsed);
  animateBlock(elapsed);
  animateScaffolding(elapsed);

  // textbox takes priority
  if (questionInterface) {
    if (!textbox.IsOpen()) return;

    if (hasLeftInput()) {
      questionInterface->SelectYes();
      return;
    }

    if (hasRightInput()) {
      questionInterface->SelectNo();
      return;
    }

    if (Input().Has(InputEvents::pressed_confirm)) {
      if (!textbox.IsEndOfBlock()) {
        textbox.CompleteCurrentBlock();
        return;
      }

      if (textbox.IsEndOfBlock() && !textbox.IsFinalBlock()) {
        textbox.ShowNextLines();
        return;
      }

      questionInterface->ConfirmSelection();
      return;
    }

    return;
  }

  // handle the compile sequence states
  if (state == state::compiling || state == state::waiting) {
    progress += elapsed;

    if (state == state::waiting) {
      if (Input().Has(InputEvents::pressed_confirm)) {
        state = state::finishing;
        return;
      }
    }
    if (isCompileFinished() && state != state::waiting) {
      completeAndSave();
      return;
    }

    if(!isCompileFinished()) {
      double percent = (progress / static_cast<double>(maxProgressTime));
      double block_time = static_cast<double>(maxProgressTime/ GRID_SIZE);

      // complete the text when we're 75% through the block
      std::string label = "None";
      double COMPLETE_AT_PROGRESS = .75;
      double block_progress = std::fmod(progress, block_time) / block_time;
      double text_progress = std::min(block_progress * 1. / COMPLETE_AT_PROGRESS, 1.);

      size_t xoffset = static_cast<size_t>(percent * GRID_SIZE);
      size_t gridLoc = (3u * GRID_SIZE) + xoffset;
      Piece* p = grid[gridLoc];

      if (p) {
        label = p->name;
      }

      if (currCompileIndex != gridLoc) {
        if (p) {
          compiledHash[p] = true;
          Audio().Play(compile_item);
        }
        else {
          Audio().Play(compile_no_item);
        }

        currCompileIndex = gridLoc;
      }

      size_t len = static_cast<size_t>(std::ceil(label.size() * text_progress));
      infoText.SetString("Running...\n" + stx::format_to_fit(label.substr(0, len), 11, 2));
    }

    return;
  }
  
  // if finishing the compilation sequence
  if(state == state::finishing) {
    sf::Color color = progressBar.getColor();

    if (color.a > 0) {
      color.a -= 4;
      progressBarUVs.left -= 1; // another speedup at this step
      progressBar.setColor(color);
    }
    else {
      // the animation is over, stop blinking
      compiledHash.clear();

      auto onYes = [this]() {
        quitScene();
      };

      auto onNo = [this]() {
        selectItemUI(pieces.size());
      };

      std::string msg = "OK!\nRUN complete!\n\n";
      msg += "Good job, " + getController().Session().GetNick() + "!\n\n\n";
      msg += "Quit the Navi Customizer?";
      questionInterface = new Question(msg, onYes, onNo);
      textbox.EnqueMessage(questionInterface);
      textbox.Open();
    }

    return;
  }

  // handle input
  if (itemListSelected) {
    if (handleUIKeys(elapsed)) {
      return;
    }

    //else arrow keys are not held this state
    selectInputCooldown = 0;
    extendedHold = false;

    if (!Input().Has(InputEvents::pressed_confirm)) return;
    
    if (handleSelectItemFromList()) return;

    startCompile();

    return;
  }

  if (state == state::block_prompt) {
    handleMenuUIKeys(elapsed);
    return;
  }

  if (!grabbingPiece && !insertingPiece) {
    // DEBUG: generate random pieces
    //if (Input().GetAnyKey() == sf::Keyboard::Space) {
    //  insertingPiece = generateRandomBlock();
    //  return;
    //}

    if (Input().Has(InputEvents::pressed_confirm)) {
      if (Piece* piece = grid[cursorLocation]) {
        handleGrabAction();
      }

      return;
    }
  }
  else if (insertingPiece) {
    if(handlePieceAction(insertingPiece, &PlayerCustScene::executeCancelInsert))
      return;
  }
  else if (grabbingPiece) {
    if(handlePieceAction(grabbingPiece, &PlayerCustScene::executeCancelGrab))
      return;
  }

  if (handleUIKeys(elapsed)) {
    return;
  }

  //else
  selectInputCooldown = 0;
  extendedHold = false;
}

void PlayerCustScene::onDraw(sf::RenderTexture& surface)
{
  surface.draw(bg);
  surface.draw(sceneLabel);
  surface.draw(gridSprite);

  sf::Sprite blockSprite;
  blockSprite.setScale(2.f, 2.f);

  size_t count{};
  for (auto& [key, value] : blockTypeInUseTable) {
    if (value == 0) continue;
    float x = (gridSprite.getPosition().x + (8.f*2.f)) + (count*(14.f+1.f)*2.f);
    float y = gridSprite.getPosition().y + 2.f*2.f;
    blockSprite.setTexture(*blockTextures[key], true);
    blockSprite.setTextureRect({ 60, 0, 14, 9 });
    blockSprite.setPosition({ x, y });
    surface.draw(blockSprite);
    count++;
  }

  for (size_t i = 0; i < GRID_SIZE; i++) {
    for (size_t j = 0; j < GRID_SIZE; j++) {
      size_t index = (i * GRID_SIZE) + j;
      if (Piece* p = grid[index]) {
        if (isGridEdge(i, j)) {
          drawEdgeBlock(surface, p, i, j);
        }
        else {
          sf::Vector2f blockPos = blockToScreen(i, j);

          blockSprite.setTexture(*blockTextures[grid[index]->typeIndex], true);
          blockSprite.setPosition({ blockPos.x, blockPos.y });
          refreshBlock(p, blockSprite);
          surface.draw(blockSprite);
        }
      }
    }
  }

  // draw track
  refreshTrack();
  surface.draw(track);

  // draw items
  float yoffset = 0.;
  
  if (listStart+1u > MAX_ITEMS_ON_SCREEN) {
    yoffset = -blueButtonSprite.getLocalBounds().height * ((listStart+1u) - MAX_ITEMS_ON_SCREEN);
  }

  sf::Vector2f previewPos{};
  sf::Vector2f bottom;
  bottom.x = 140.f;

  for (size_t i = 0; i < pieces.size(); i++) {
    refreshButton(i);
    blueButtonSprite.setPosition(bottom.x * 2.f, (bottom.y + yoffset) * 2.f);
    surface.draw(blueButtonSprite);

    if (listStart == i) {
      sf::Vector2f dest = blueButtonSprite.getPosition();
      sf::Vector2f pos = itemArrowCursor.getPosition();
      pos.x = swoosh::ease::interpolate(0.5f, pos.x, dest.x);
      pos.y = swoosh::ease::interpolate(0.5f, pos.y, dest.y);
      itemArrowCursor.setPosition(pos);
      previewPos = { dest.x + (blueButtonSprite.getGlobalBounds().width+10.f), dest.y };
    }

    if (scaffolding == 1.f) {
      sf::Vector2f textOffset = { 4.f, 8.f };
      itemText.SetString(pieces[i]->name);
      itemText.SetColor(sf::Color(33, 115, 140));
      itemText.setPosition((bottom.x + textOffset.x) * itemText.getScale().x, ((bottom.y + yoffset) * itemText.getScale().y) + textOffset.y + 2.f);
      surface.draw(itemText);
      itemText.SetColor(sf::Color::White);
      itemText.setPosition((bottom.x + textOffset.x) * itemText.getScale().x, ((bottom.y + yoffset) * itemText.getScale().y) + textOffset.y);
      surface.draw(itemText);
    }

    bottom.y += 19; // pixel height of button
  }

  if (bottom.y == 0) {
    bottom.y = 19; // pixel height of button
  }

  // draw run button
  refreshButton(pieces.size());
  greenButtonSprite.setPosition(bottom.x*2.f, (bottom.y+yoffset)*2.f);
  surface.draw(greenButtonSprite);

  if (itemListSelected) {
    if (state == state::usermode) {
      surface.draw(itemArrowCursor);
    }

    if (listStart == pieces.size()) {
      sf::Vector2f dest = greenButtonSprite.getPosition();
      sf::Vector2f pos = itemArrowCursor.getPosition();
      pos.x = swoosh::ease::interpolate(0.5f, pos.x, dest.x);
      pos.y = swoosh::ease::interpolate(0.5f, pos.y, dest.y);
      itemArrowCursor.setPosition(pos);
    }
    else {
      // check if this value was set
      if (previewPos.x > 0.) {
        drawPreview(surface, pieces[listStart], previewPos);
      }
    }
  }

  // draw info box
  surface.draw(infoBox);

  sf::Vector2f pos = infoText.getPosition();
  infoText.SetColor(sf::Color(33, 115, 140));
  infoText.setPosition(pos.x+2.f, pos.y+2.f);
  surface.draw(infoText);
  infoText.SetColor(sf::Color::White);
  infoText.setPosition(pos);
  surface.draw(infoText);

  // progress bar shows when compiling
  surface.draw(progressBar);
  
  if(!itemListSelected) {
    // claw/cursor always draws on top
    Piece* p = insertingPiece ? insertingPiece : grabbingPiece ? grabbingPiece : nullptr;
    sf::Vector2f piecePos = insertingPiece ? cursor.getPosition() : gridCursorToScreen();
    if (p) {
      drawPiece(surface, p, piecePos);

      if (grabbingPiece) {
        surface.draw(claw);
      }
    }
    else {
      // display any hovering text
      sf::Vector2f pos = hoverText.getPosition();
      hoverText.SetColor(sf::Color::Black);
      hoverText.setPosition(pos.x + 2.f, pos.y + 2.f);
      surface.draw(hoverText);
      hoverText.SetColor(sf::Color::White);
      hoverText.setPosition(pos);
      surface.draw(hoverText);

      if (grid[cursorLocation]) {
        surface.draw(claw);
      }
      else {
        surface.draw(cursor);
      }
    }
  }

  // grab prompt
  if (state == state::block_prompt) {
    surface.draw(menuBox);
  }

  // textbox is top over everything
  surface.draw(textbox);
}

std::vector<std::string> PlayerCustScene::getInstalledBlocks(const std::string& playerID, const GameSession& session)
{
  std::vector<std::string> res;

  std::string value = session.GetKeyValue(playerID + ":" + "blocks");
  if (value.empty()) return res;

  Poco::Buffer<char> buffer{ value.c_str(), value.size() };
  BufferReader reader;

  size_t size = reader.Read<size_t>(buffer);
  for (size_t i = 0; i < size; i++) {
    std::string uuid = reader.ReadTerminatedString(buffer);

    bool isValid = reader.Read<bool>(buffer);
    reader.Read<size_t>(buffer); // skip center
    reader.Read<size_t>(buffer); // skip rot

    if (isValid) {
      res.push_back(uuid);
    }
  }

  return res;
}

void PlayerCustScene::onEnd()
{
}