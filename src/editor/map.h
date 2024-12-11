#pragma once

#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>

// Forward declarations
class Layer;
class Map;
class QuadTree;

// Axis-aligned bounding box
struct AABB {
  glm::vec2 min;
  glm::vec2 max;

  AABB() : min(0), max(0) {}
  AABB(const glm::vec2 &min, const glm::vec2 &max) : min(min), max(max) {}

  bool contains(const glm::vec2 &point) const {
    return point.x >= min.x && point.x <= max.x && point.y >= min.y &&
           point.y <= max.y;
  }

  bool intersects(const AABB &other) const {
    return min.x <= other.max.x && max.x >= other.min.x &&
           min.y <= other.max.y && max.y >= other.min.y;
  }
};

class Tile {
public:
  using Id = size_t;

  static Tile create(Id tileType, const glm::vec2 &position,
                     const glm::vec2 &size) {
    return Tile(tileType, position, size);
  }

  Id getId() const { return id_; }
  Id getTileType() const { return tileType_; }
  const glm::vec2 &getPosition() const { return position_; }
  const glm::vec2 &getSize() const { return size_; }

  AABB getBounds() const { return AABB(position_, position_ + size_); }

  // Move-only type
  Tile(Tile &&) noexcept = default;
  Tile(const Tile &) = delete;
  Tile &operator=(const Tile &) = delete;
  Tile &operator=(Tile &&) = delete;

  bool operator==(const Tile &other) const {
    return id_ == other.id_ && tileType_ == other.tileType_;
  }

private:
  Tile(Id tileType, const glm::vec2 &position, const glm::vec2 &size)
      : id_(generateId()), tileType_(tileType), position_(position),
        size_(size) {}

  static Id generateId() {
    static std::atomic<Id> nextId{0};
    return nextId++;
  }

  const Id id_;
  const Id tileType_;
  const glm::vec2 position_;
  const glm::vec2 size_;
};

// QuadTree implementation for spatial partitioning
class QuadTree {
public:
  struct Entry {
    std::string layerName;
    Tile::Id tileId;
    AABB bounds;

    Entry(const std::string &layerName, Tile::Id tileId, const AABB &bounds)
        : layerName(layerName), tileId(tileId), bounds(bounds) {}

    Entry(Entry &&) noexcept = default;
    Entry &operator=(Entry &&) noexcept = default;
    Entry(const Entry &) = default;
    Entry &operator=(const Entry &) = default;
  };

  QuadTree(const AABB &bounds, int maxDepth = 8, int maxEntries = 4)
      : bounds_(bounds), maxDepth_(maxDepth), maxEntries_(maxEntries) {}

  void insert(const Entry &entry) {
    if (!bounds_.intersects(entry.bounds)) {
      return;
    }

    if (entries_.size() < maxEntries_ || depth_ >= maxDepth_) {
      entries_.push_back(entry);
      return;
    }

    if (children_.empty()) {
      subdivide();
    }

    for (auto &child : children_) {
      child->insert(entry);
    }
  }

  void remove(const std::string &layerName, Tile::Id tileId) {
    entries_.erase(std::remove_if(entries_.begin(), entries_.end(),
                                  [&](const Entry &e) {
                                    return e.layerName == layerName &&
                                           e.tileId == tileId;
                                  }),
                   entries_.end());

    for (auto &child : children_) {
      child->remove(layerName, tileId);
    }
  }

  std::vector<Entry> query(const AABB &area) const {
    std::vector<Entry> result;
    if (!bounds_.intersects(area)) {
      return result;
    }

    for (const auto &entry : entries_) {
      if (entry.bounds.intersects(area)) {
        result.push_back(entry);
      }
    }

    for (const auto &child : children_) {
      auto childResults = child->query(area);
      result.insert(result.end(), childResults.begin(), childResults.end());
    }

    return result;
  }

private:
  void subdivide() {
    float midX = (bounds_.min.x + bounds_.max.x) * 0.5f;
    float midY = (bounds_.min.y + bounds_.max.y) * 0.5f;

    children_.push_back(std::make_unique<QuadTree>(
        AABB(bounds_.min, glm::vec2(midX, midY)), maxDepth_, maxEntries_));
    children_.push_back(std::make_unique<QuadTree>(
        AABB(glm::vec2(midX, bounds_.min.y), glm::vec2(bounds_.max.x, midY)),
        maxDepth_, maxEntries_));
    children_.push_back(std::make_unique<QuadTree>(
        AABB(glm::vec2(bounds_.min.x, midY), glm::vec2(midX, bounds_.max.y)),
        maxDepth_, maxEntries_));
    children_.push_back(std::make_unique<QuadTree>(
        AABB(glm::vec2(midX, midY), bounds_.max), maxDepth_, maxEntries_));
  }

  AABB bounds_;
  int depth_ = 0;
  int maxDepth_;
  int maxEntries_;
  std::vector<Entry> entries_;
  std::vector<std::unique_ptr<QuadTree>> children_;
};

struct TileLocation {
  std::string layerName;
  Tile::Id tileId;
  Tile::Id tileType;
  glm::vec2 position;
  glm::vec2 size;
};

class Layer {
public:
  using const_iterator = std::vector<Tile>::const_iterator;

  const std::string &getName() const { return name_; }
  int getDepth() const { return depth_; }
  const std::vector<Tile> &getTiles() const { return tiles_; }

  void addTile(Tile &&tile) { tiles_.push_back(std::move(tile)); }

  bool removeTile(Tile::Id tileId) {
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

  void setDepth(int depth) { depth_ = depth; }

  const_iterator begin() const { return tiles_.begin(); }
  const_iterator end() const { return tiles_.end(); }

private:
  std::string name_;
  int depth_;
  std::vector<Tile> tiles_;

  friend class Map;

  Layer(const std::string &name, int depth = 0) : name_(name), depth_(depth) {}
};

class Map {
public:
  using LayerName = std::string;

  Map(const AABB &bounds = AABB(glm::vec2(-10000), glm::vec2(10000)))
      : spatialIndex_(std::make_unique<QuadTree>(bounds)) {}

  class LayerIterator {
  public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = const Layer;
    using difference_type = std::ptrdiff_t;
    using pointer = const Layer *;
    using reference = const Layer &;

    LayerIterator(std::unordered_map<LayerName, Layer>::const_iterator it)
        : current_(it) {}

    LayerIterator &operator++() {
      ++current_;
      return *this;
    }

    LayerIterator operator++(int) {
      LayerIterator tmp = *this;
      ++(*this);
      return tmp;
    }

    bool operator==(const LayerIterator &rhs) const {
      return current_ == rhs.current_;
    }

    bool operator!=(const LayerIterator &rhs) const { return !(*this == rhs); }

    reference operator*() const { return current_->second; }

  private:
    std::unordered_map<LayerName, Layer>::const_iterator current_;
  };

  class LayerContainer {
  public:
    explicit LayerContainer(const Map &map) : map_(map) {}

    LayerIterator begin() const {
      std::shared_lock lock(map_.mutex_);
      return LayerIterator(map_.layers_.begin());
    }

    LayerIterator end() const {
      std::shared_lock lock(map_.mutex_);
      return LayerIterator(map_.layers_.end());
    }

  private:
    const Map &map_;
  };

  bool createLayer(const LayerName &name, int depth = 0) {
    std::unique_lock lock(mutex_);
    return layers_.try_emplace(name, Layer(name, depth)).second;
  }

  bool removeLayer(const LayerName &name) {
    std::unique_lock lock(mutex_);
    auto it = layers_.find(name);
    if (it != layers_.end()) {
      for (const auto &tile : it->second.getTiles()) {
        spatialIndex_->remove(name, tile.getId());
      }
      layers_.erase(it);
      return true;
    }
    return false;
  }

  LayerContainer layers() const { return LayerContainer(*this); }

  std::optional<Tile::Id> addTile(const LayerName &layerName, Tile::Id tileType,
                                  const glm::vec2 &position,
                                  const glm::vec2 &size) {
    std::unique_lock lock(mutex_);
    auto it = layers_.find(layerName);
    if (it != layers_.end()) {
      auto tile = Tile::create(tileType, position, size);
      Tile::Id id = tile.getId();

      // Add to spatial index
      spatialIndex_->insert(QuadTree::Entry{layerName, id, tile.getBounds()});

      // Add to layer
      it->second.addTile(std::move(tile));
      return id;
    }
    return std::nullopt;
  }

  bool removeTile(const LayerName &layerName, Tile::Id tileId) {
    std::unique_lock lock(mutex_);
    auto it = layers_.find(layerName);
    if (it != layers_.end()) {
      if (it->second.removeTile(tileId)) {
        spatialIndex_->remove(layerName, tileId);
        return true;
      }
    }
    return false;
  }

  // Find tiles at a given position using the quadtree
  std::vector<TileLocation> getTilesAt(const glm::vec2 &position) const {
    std::shared_lock lock(mutex_);

    // Create a small AABB around the point for the query
    AABB queryArea(position - glm::vec2(0.1f), position + glm::vec2(0.1f));

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

private:
  mutable std::shared_mutex mutex_;
  std::unordered_map<LayerName, Layer> layers_;
  std::unique_ptr<QuadTree> spatialIndex_;
};