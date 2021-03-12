#pragma once

#include <SFML/Graphics.hpp>
#include <vector>
#include <memory>

#include "bnOverworldShapes.h"
#include "../bnAnimation.h"

namespace Overworld
{
  class Map;

  enum class Projection { Isometric, Orthographic };

  struct Tileset {
    const std::string name;
    const unsigned int firstGid;
    const unsigned int tileCount;
    const sf::Vector2f drawingOffset;
    const sf::Vector2f alignmentOffset;
    const Projection orientation; // used for collisions
    std::shared_ptr<sf::Texture> texture;
    Animation animation;
  };

  struct TileMeta {
    const unsigned int id;
    const unsigned int gid;
    const sf::Vector2f drawingOffset;
    const sf::Vector2f alignmentOffset;
    Animation animation;
    sf::Sprite sprite;
    std::vector<std::unique_ptr<Shape>> collisionShapes;


    TileMeta(unsigned int id, unsigned int gid, sf::Vector2f drawingOffset, sf::Vector2f alignmentOffset)
      : id(id), gid(gid), drawingOffset(drawingOffset), alignmentOffset(alignmentOffset) {}
  };

  struct Tile {
    unsigned int gid;
    bool flippedHorizontal;
    bool flippedVertical;
    bool rotated;

    Tile(unsigned int gid) {
      // https://doc.mapeditor.org/en/stable/reference/tmx-map-format/#tile-flipping
      bool flippedHorizontal = (gid >> 31 & 1) == 1;
      bool flippedVertical = (gid >> 30 & 1) == 1;
      bool flippedAntiDiagonal = (gid >> 29 & 1) == 1;

      this->gid = gid << 3 >> 3;
      this->rotated = flippedAntiDiagonal;

      if (rotated) {
        this->flippedHorizontal = flippedVertical;
        this->flippedVertical = !flippedHorizontal;
      }
      else {
        this->flippedHorizontal = flippedHorizontal;
        this->flippedVertical = flippedVertical;
      }
    }

    bool Intersects(Map& map, float x, float y) const;
  };
}
