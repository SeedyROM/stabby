#pragma once

#include <algorithm>
#include <bitset>
#include <concepts>
#include <functional>
#include <iostream>
#include <memory>
#include <ranges>
#include <span>
#include <stdexcept>
#include <type_traits>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace ste {

// Maximum number of components that can exist in the system
constexpr size_t MAX_COMPONENTS = 64;

// Component concept - ensures components are simple POD types
template <typename T>
concept Component =
    std::is_standard_layout_v<T> && std::is_trivially_copyable_v<T>;

// Forward declarations
template <typename... Components>
requires(Component<Components> &&...) class Query;

class World;
class Entity;

// Time resource
class Time {
public:
  float deltaSeconds = 0.0f;
  virtual ~Time() = default; // Make Time non-trivially-copyable
};

// Resource concept - ensures resources are heap-allocated objects
template <typename T>
concept Resource = std::is_base_of_v<Time, T> ||
    (!std::is_trivially_copyable_v<T> && std::is_class_v<T>);

// Component ID generator
class ComponentId {
  static inline size_t nextId = 0;

public:
  template <Component T> static size_t get() {
    static size_t id = nextId++;
    return id;
  }
};

// System structure
struct SystemInfo {
  std::function<void(World &)> func;
  int priority;
  std::string label;
};

// Custom comparison for SystemInfo
inline bool operator<(const SystemInfo &a, const SystemInfo &b) {
  return a.priority < b.priority;
}

// Entity class
class Entity {
  size_t id;
  World *world;
  std::bitset<MAX_COMPONENTS> mask;

  template <typename... Ts>
  requires(Component<Ts> &&...) friend class Query;
  friend class World;

  Entity(World *world, size_t id) : world(world), id(id) {}

public:
  size_t getId() const { return id; }

  template <Component T> Entity &with(T component);

  template <Component T> T &get();

  template <Component T> bool has() const;
};

// ComponentArray - type-erased storage for components
class IComponentArray {
public:
  virtual ~IComponentArray() = default;
  virtual void remove(size_t entityId) = 0;
  virtual size_t size() const = 0;
};

template <Component T> class ComponentArray : public IComponentArray {
  std::vector<T> components;
  std::vector<bool> occupied;
  size_t count = 0;

public:
  void insert(size_t entityId, T component) {
    if (entityId >= components.size()) {
      components.resize(entityId + 1);
      occupied.resize(entityId + 1);
    }
    components[entityId] = component;
    if (!occupied[entityId]) {
      occupied[entityId] = true;
      count++;
    }
  }

  T &get(size_t entityId) {
    if (!occupied[entityId]) {
      throw std::runtime_error("Component not found");
    }
    return components[entityId];
  }

  bool has(size_t entityId) const {
    return entityId < occupied.size() && occupied[entityId];
  }

  void remove(size_t entityId) override {
    if (entityId < occupied.size() && occupied[entityId]) {
      occupied[entityId] = false;
      count--;
    }
  }

  size_t size() const override { return count; }
};

// World class
class World {
private:
  std::vector<bool> activeEntities;
  std::unordered_map<size_t, std::unique_ptr<IComponentArray>> components;
  std::unordered_map<std::type_index, std::shared_ptr<void>> resources;
  std::vector<SystemInfo> updateSystems;
  std::vector<SystemInfo> renderSystems;
  size_t nextEntityId = 0;

  template <typename... Ts>
  requires(Component<Ts> &&...) friend class Query;

public:
  World() { addResource(std::make_shared<Time>()); }

  Entity spawn() {
    if (nextEntityId >= activeEntities.size()) {
      activeEntities.resize(nextEntityId + 1);
    }
    activeEntities[nextEntityId] = true;
    return Entity(this, nextEntityId++);
  }

  void destroy(Entity entity) {
    if (entity.getId() < activeEntities.size()) {
      activeEntities[entity.getId()] = false;
      for (auto &[_, array] : components) {
        array->remove(entity.getId());
      }
    }
  }

  template <Component T> void setComponent(size_t entityId, T component) {
    size_t componentId = ComponentId::get<T>();
    if (!components.contains(componentId)) {
      components[componentId] = std::make_unique<ComponentArray<T>>();
    }
    auto *array =
        static_cast<ComponentArray<T> *>(components[componentId].get());
    array->insert(entityId, component);
  }

  template <Component T> T &getComponent(size_t entityId) {
    size_t componentId = ComponentId::get<T>();
    auto it = components.find(componentId);
    if (it == components.end()) {
      throw std::runtime_error("Component type not found");
    }
    return static_cast<ComponentArray<T> *>(it->second.get())->get(entityId);
  }

  template <Component T> bool hasComponent(size_t entityId) const {
    size_t componentId = ComponentId::get<T>();
    auto it = components.find(componentId);
    if (it == components.end())
      return false;
    return static_cast<const ComponentArray<T> *>(it->second.get())
        ->has(entityId);
  }

  template <Resource T> void addResource(std::shared_ptr<T> resource) {
    resources[std::type_index(typeid(T))] = resource;
  }

  template <Resource T> std::shared_ptr<T> getResource() {
    auto it = resources.find(std::type_index(typeid(T)));
    if (it == resources.end()) {
      throw std::runtime_error("Resource not found");
    }
    return std::static_pointer_cast<T>(it->second);
  }

  void addSystem(const std::string &label, std::function<void(World &)> func,
                 int priority = 0, bool isRender = false) {
    auto &systems = isRender ? renderSystems : updateSystems;
    systems.push_back(
        {.func = std::move(func), .priority = priority, .label = label});
  }

  void update(float deltaTime) {
    if (auto time = getResource<Time>()) {
      time->deltaSeconds = deltaTime;
    }

    // Execute all systems
    for (auto &system : updateSystems) {
      if (system.func) { // Check if function exists
        system.func(*this);
      }
    }
  }

  void render() {
    for (auto &system : renderSystems) {
      if (system.func) { // Check if function exists
        system.func(*this);
      }
    }
  }
};

// Implementation of Entity methods
template <Component T> Entity &Entity::with(T component) {
  world->setComponent(id, component);
  mask.set(ComponentId::get<T>());
  return *this;
}

template <Component T> T &Entity::get() { return world->getComponent<T>(id); }

template <Component T> bool Entity::has() const {
  return world->hasComponent<T>(id);
}

// Query class
template <typename... Components>
requires(Component<Components> &&...) class Query {
  World *world;
  std::vector<Entity> matches;

public:
  Query(World *world) : world(world) {
    // Find entities that have all required components
    std::bitset<MAX_COMPONENTS> requiredComponents;
    (requiredComponents.set(ComponentId::get<Components>()), ...);

    // Scan all active entities
    for (size_t i = 0; i < world->activeEntities.size(); i++) {
      if (!world->activeEntities[i])
        continue;

      // Check if entity has all required components
      bool hasAll = true;
      ((hasAll = hasAll && world->hasComponent<Components>(i)), ...);

      if (hasAll) {
        matches.push_back(Entity(world, i));
      }
    }
  }

  class Iterator {
  public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = std::tuple<Entity &, Components &...>;
    using difference_type = std::ptrdiff_t;
    using pointer = value_type *;
    using reference = value_type;

    Iterator(Entity *ptr, World *world) : ptr(ptr), world(world) {}

    value_type operator*() {
      Entity &e = *ptr;
      return std::tuple_cat(
          std::tuple<Entity &>(e),
          std::make_tuple(std::ref(e.template get<Components>())...));
    }

    Iterator &operator++() {
      ++ptr;
      return *this;
    }
    Iterator operator++(int) {
      Iterator tmp = *this;
      ++ptr;
      return tmp;
    }
    bool operator==(const Iterator &other) const { return ptr == other.ptr; }
    bool operator!=(const Iterator &other) const { return ptr != other.ptr; }

  private:
    Entity *ptr;
    World *world;
  };

  auto begin() { return Iterator(matches.data(), world); }
  auto end() { return Iterator(matches.data() + matches.size(), world); }
};

} // namespace ste