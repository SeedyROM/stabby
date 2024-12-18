#pragma once

#include <glm/glm.hpp>

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

// QuadTree implementation for spatial partitioning
template <typename T> class QuadTree {
public:
  QuadTree(const AABB &bounds, int maxDepth = 8, int maxEntries = 4)
      : bounds_(bounds), m_maxDepth(maxDepth), m_maxEntries(maxEntries) {}

  void insert(const T &entry) {
    if (!bounds_.intersects(entry.bounds)) {
      return;
    }

    if (m_entries.size() < m_maxEntries || m_depth >= m_maxDepth) {
      m_entries.push_back(entry);
      return;
    }

    if (m_children.empty()) {
      subdivide();
    }

    for (auto &child : m_children) {
      child->insert(entry);
    }
  }

  std::vector<T> query(const AABB &area) const {
    std::vector<T> result;
    if (!bounds_.intersects(area)) {
      return result;
    }

    for (const auto &entry : m_entries) {
      if (entry.bounds.intersects(area)) {
        result.push_back(entry);
      }
    }

    for (const auto &child : m_children) {
      auto childResults = child->query(area);
      result.insert(result.end(), childResults.begin(), childResults.end());
    }

    return result;
  }

  void removeEntry(std::function<bool(const T &)> predicate) {
    auto it = std::find_if(m_entries.begin(), m_entries.end(), predicate);
    if (it != m_entries.end()) {
      m_entries.erase(it);
    }

    for (auto &child : m_children) {
      child->removeEntry(predicate);
    }
  }

private:
  void subdivide() {
    float midX = (bounds_.min.x + bounds_.max.x) * 0.5f;
    float midY = (bounds_.min.y + bounds_.max.y) * 0.5f;

    m_children.push_back(std::make_unique<QuadTree>(
        AABB(bounds_.min, glm::vec2(midX, midY)), m_maxDepth, m_maxEntries));
    m_children.push_back(std::make_unique<QuadTree>(
        AABB(glm::vec2(midX, bounds_.min.y), glm::vec2(bounds_.max.x, midY)),
        m_maxDepth, m_maxEntries));
    m_children.push_back(std::make_unique<QuadTree>(
        AABB(glm::vec2(bounds_.min.x, midY), glm::vec2(midX, bounds_.max.y)),
        m_maxDepth, m_maxEntries));
    m_children.push_back(std::make_unique<QuadTree>(
        AABB(glm::vec2(midX, midY), bounds_.max), m_maxDepth, m_maxEntries));
  }

  AABB bounds_;
  int m_depth = 0;
  int m_maxDepth;
  int m_maxEntries;
  std::vector<T> m_entries;
  std::vector<std::unique_ptr<QuadTree>> m_children;
};
