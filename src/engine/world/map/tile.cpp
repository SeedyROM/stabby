#include "tile.h"

namespace ste {

// Tile implementations
Tile Tile::create(Id tileType, const glm::vec2 &position,
                  const glm::vec2 &size) {
  return Tile(tileType, position, size);
}

Tile::Id Tile::getId() const { return id_; }
Tile::Id Tile::getTileType() const { return tileType_; }
const glm::vec2 &Tile::getPosition() const { return position_; }
const glm::vec2 &Tile::getSize() const { return size_; }

AABB Tile::getBounds() const { return AABB(position_, position_ + size_); }

bool Tile::operator==(const Tile &other) const {
  return id_ == other.id_ && tileType_ == other.tileType_;
}

Tile::Tile(Id tileType, const glm::vec2 &position, const glm::vec2 &size)
    : id_(generateId()), tileType_(tileType), position_(position), size_(size) {
}

Tile::Id Tile::generateId() {
  static std::atomic<Id> nextId{0};
  return nextId++;
}

} // namespace ste