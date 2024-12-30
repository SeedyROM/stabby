#pragma once

#include <string>

#include "../quadtree.h"

namespace ste {

class Tile {
public:
  using Id = size_t;

  static Tile create(Id tileType, const glm::vec2 &position,
                     const glm::vec2 &size);

  Id getId() const;
  Id getTileType() const;
  const glm::vec2 &getPosition() const;
  const glm::vec2 &getSize() const;

  AABB getBounds() const;

  // Move-only type
  Tile(Tile &&) noexcept = default;
  Tile(const Tile &) = delete;
  Tile &operator=(const Tile &) = delete;
  Tile &operator=(Tile &&) = delete;

  bool operator==(const Tile &other) const;

private:
  Tile(Id tileType, const glm::vec2 &position, const glm::vec2 &size);
  static Id generateId();

  const Id id_;
  const Id tileType_;
  const glm::vec2 position_;
  const glm::vec2 size_;
};

struct TileLocation {
  std::string layerName;
  Tile::Id tileId;
  Tile::Id tileType;
  glm::vec2 position;
  glm::vec2 size;
};

} // namespace ste