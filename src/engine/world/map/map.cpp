#include "map.h"

namespace ste {

// MapEntry implementation
MapEntry::MapEntry(const std::string &layerName, Tile::Id tileId,
                   const AABB &bounds)
    : layerName(layerName), tileId(tileId), bounds(bounds) {}

// Layer implementations
Layer::Layer(const std::string &name, int depth) : name_(name), depth_(depth) {}

const std::string &Layer::getName() const { return name_; }
int Layer::getDepth() const { return depth_; }
const std::vector<Tile> &Layer::getTiles() const { return tiles_; }

void Layer::addTile(Tile &&tile) { tiles_.push_back(std::move(tile)); }

bool Layer::removeTile(Tile::Id tileId) {
  auto it =
      std::find_if(tiles_.begin(), tiles_.end(), [tileId](const Tile &tile) {
        return tile.getId() == tileId;
      });

  if (it != tiles_.end()) {
    std::vector<Tile> newTiles;
    newTiles.reserve(tiles_.size() - 1);
    std::move(tiles_.begin(), it, std::back_inserter(newTiles));
    std::move(it + 1, tiles_.end(), std::back_inserter(newTiles));
    tiles_ = std::move(newTiles);
    return true;
  }
  return false;
}

void Layer::setDepth(int depth) { depth_ = depth; }

Layer::const_iterator Layer::begin() const { return tiles_.begin(); }
Layer::const_iterator Layer::end() const { return tiles_.end(); }

// Map implementations
Map::Map(const AABB &bounds)
    : spatialIndex_(std::make_unique<QuadTree<MapEntry>>(bounds)) {}

Map::LayerIterator::LayerIterator(
    std::unordered_map<LayerName, Layer>::const_iterator it)
    : current_(it) {}

Map::LayerIterator &Map::LayerIterator::operator++() {
  ++current_;
  return *this;
}

Map::LayerIterator Map::LayerIterator::operator++(int) {
  LayerIterator tmp = *this;
  ++(*this);
  return tmp;
}

bool Map::LayerIterator::operator==(const LayerIterator &rhs) const {
  return current_ == rhs.current_;
}

bool Map::LayerIterator::operator!=(const LayerIterator &rhs) const {
  return !(*this == rhs);
}

Map::LayerIterator::reference Map::LayerIterator::operator*() const {
  return current_->second;
}

Map::LayerContainer::LayerContainer(const Map &map) : map_(map) {}

Map::LayerIterator Map::LayerContainer::begin() const {
  return LayerIterator(map_.layers_.begin());
}

Map::LayerIterator Map::LayerContainer::end() const {
  return LayerIterator(map_.layers_.end());
}

bool Map::addLayer(const LayerName &name, int depth) {
  return layers_.try_emplace(name, Layer(name, depth)).second;
}

bool Map::removeLayer(const LayerName &name) {
  auto it = layers_.find(name);
  if (it != layers_.end()) {
    for (const auto &tile : it->second.getTiles()) {
      spatialIndex_->removeEntry([&](const MapEntry &entry) {
        return entry.layerName == name && entry.tileId == tile.getId();
      });
    }
    layers_.erase(it);
    return true;
  }
  return false;
}

Map::LayerContainer Map::layers() const { return LayerContainer(*this); }

std::optional<Tile::Id> Map::addTile(const LayerName &layerName,
                                     Tile::Id tileType,
                                     const glm::vec2 &position,
                                     const glm::vec2 &size) {
  auto it = layers_.find(layerName);
  if (it != layers_.end()) {
    auto tile = Tile::create(tileType, position, size);
    Tile::Id id = tile.getId();

    // Add to spatial index
    spatialIndex_->insert(MapEntry{layerName, id, tile.getBounds()});

    // Add to layer
    it->second.addTile(std::move(tile));
    return id;
  }
  return std::nullopt;
}

bool Map::removeTile(const LayerName &layerName, Tile::Id tileId) {
  auto it = layers_.find(layerName);
  if (it != layers_.end()) {
    if (it->second.removeTile(tileId)) {
      spatialIndex_->removeEntry([&](const MapEntry &entry) {
        return entry.layerName == layerName && entry.tileId == tileId;
      });
      return true;
    }
  }
  return false;
}

std::vector<TileLocation> Map::getTilesAt(const glm::vec2 &position) const {
  // Create a small AABB around the point for the query, use the f32 epsilon
  AABB queryArea(position - glm::vec2(std::numeric_limits<float>::epsilon()),
                 position + glm::vec2(std::numeric_limits<float>::epsilon()));

  auto entries = spatialIndex_->query(queryArea);
  std::vector<TileLocation> result;

  for (const auto &entry : entries) {
    auto it = layers_.find(entry.layerName);
    if (it != layers_.end()) {
      auto tileIt = std::find_if(
          it->second.getTiles().begin(), it->second.getTiles().end(),
          [&](const Tile &tile) { return tile.getId() == entry.tileId; });

      if (tileIt != it->second.getTiles().end() &&
          tileIt->getBounds().contains(position)) {
        result.push_back({entry.layerName, tileIt->getId(),
                          tileIt->getTileType(), tileIt->getPosition(),
                          tileIt->getSize()});
      }
    }
  }

  return result;
}

std::optional<TileLocation> Map::getTileAt(const glm::vec2 &position) const {
  auto tiles = getTilesAt(position);
  if (!tiles.empty()) {
    return tiles.front();
  }
  return std::nullopt;
}

} // namespace ste