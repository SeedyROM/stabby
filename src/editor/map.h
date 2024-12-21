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

#include "engine/engine.h"

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

struct MapEntry {
  std::string layerName;
  Tile::Id tileId;
  AABB bounds;

  MapEntry(const std::string &layerName, Tile::Id tileId, const AABB &bounds);

  MapEntry(MapEntry &&) noexcept = default;
  MapEntry &operator=(MapEntry &&) noexcept = default;
  MapEntry(const MapEntry &) = default;
  MapEntry &operator=(const MapEntry &) = default;
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

  const std::string &getName() const;
  int getDepth() const;
  const std::vector<Tile> &getTiles() const;

  void addTile(Tile &&tile);
  bool removeTile(Tile::Id tileId);
  void setDepth(int depth);

  const_iterator begin() const;
  const_iterator end() const;

private:
  std::string name_;
  int depth_;
  std::vector<Tile> tiles_;

  friend class Map;
  Layer(const std::string &name, int depth = 0);
};

class Map {
public:
  using LayerName = std::string;

  explicit Map(const AABB &bounds = AABB(glm::vec2(-10000), glm::vec2(10000)));

  class LayerIterator {
  public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = const Layer;
    using difference_type = std::ptrdiff_t;
    using pointer = const Layer *;
    using reference = const Layer &;

    explicit LayerIterator(
        std::unordered_map<LayerName, Layer>::const_iterator it);

    LayerIterator &operator++();
    LayerIterator operator++(int);
    bool operator==(const LayerIterator &rhs) const;
    bool operator!=(const LayerIterator &rhs) const;
    reference operator*() const;

  private:
    std::unordered_map<LayerName, Layer>::const_iterator current_;
  };

  class LayerContainer {
  public:
    explicit LayerContainer(const Map &map);
    LayerIterator begin() const;
    LayerIterator end() const;

  private:
    const Map &map_;
  };

  bool createLayer(const LayerName &name, int depth = 0);
  bool removeLayer(const LayerName &name);
  LayerContainer layers() const;

  std::optional<Tile::Id> addTile(const LayerName &layerName, Tile::Id tileType,
                                  const glm::vec2 &position,
                                  const glm::vec2 &size);
  bool removeTile(const LayerName &layerName, Tile::Id tileId);

  std::vector<TileLocation> getTilesAt(const glm::vec2 &position) const;
  std::optional<TileLocation> getTileAt(const glm::vec2 &position) const;

private:
  mutable std::shared_mutex mutex_;
  std::unordered_map<LayerName, Layer> layers_;
  std::unique_ptr<QuadTree<MapEntry>> spatialIndex_;
};