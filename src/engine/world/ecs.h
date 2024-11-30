#pragma once

#include <algorithm>
#include <bitset>
#include <concepts>
#include <functional>
#include <memory>
#include <string>
#include <type_traits>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace ste {

constexpr size_t MAX_COMPONENTS = 64;

// Component concept
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
  virtual ~Time() = default;
};

// Resource concept
template <typename T>
concept Resource = (std::is_class_v<T>);

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

bool operator<(const SystemInfo &a, const SystemInfo &b);

// Entity declaration (implementations below)
class Entity {
  size_t id;
  World *world;
  std::bitset<MAX_COMPONENTS> mask;

  template <typename... Ts>
  requires(Component<Ts> &&...) friend class Query;
  friend class World;

  Entity(World *world, size_t id);

public:
  size_t getId() const;
  template <Component T> Entity &with(T component);
  template <Component T> T &get();
  template <Component T> bool has() const;
};

// Component storage base class
class IComponentArray {
public:
  virtual ~IComponentArray() = default;
  virtual void remove(size_t entityId) = 0;
  virtual size_t size() const = 0;
};

// Component storage template
template <Component T> class ComponentArray : public IComponentArray {
public:
  // Use a sparse set implementation for better cache locality
  void insert(size_t entityId, T component) {
    if (entityId >= sparse.size()) {
      sparse.resize(entityId + 1, -1);
    }

    // Check if entity already has component
    if (sparse[entityId] != -1) {
      dense_components[sparse[entityId]] = component;
      return;
    }

    // Add new component
    sparse[entityId] = dense_components.size();
    dense_components.push_back(component);
    dense_entities.push_back(entityId);
  }

  T &get(size_t entityId) {
    if (entityId >= sparse.size() || sparse[entityId] == -1) {
      throw std::runtime_error("Component not found");
    }
    return dense_components[sparse[entityId]];
  }

  bool has(size_t entityId) const {
    return entityId < sparse.size() && sparse[entityId] != -1;
  }

  void remove(size_t entityId) override {
    if (entityId >= sparse.size() || sparse[entityId] == -1) {
      return;
    }

    // Swap and pop for O(1) removal
    size_t dense_idx = sparse[entityId];
    size_t last_idx = dense_components.size() - 1;

    if (dense_idx != last_idx) {
      dense_components[dense_idx] = dense_components[last_idx];
      dense_entities[dense_idx] = dense_entities[last_idx];
      sparse[dense_entities[dense_idx]] = dense_idx;
    }

    sparse[entityId] = -1;
    dense_components.pop_back();
    dense_entities.pop_back();
  }

  size_t size() const override { return dense_components.size(); }

  // Iterator access for systems
  auto begin() { return dense_components.begin(); }
  auto end() { return dense_components.end(); }
  const auto &entities() const { return dense_entities; }

private:
  std::vector<T> dense_components;    // Packed array of components
  std::vector<size_t> dense_entities; // Corresponding entity IDs
  std::vector<int> sparse;            // Entity ID to dense index mapping
};

// World class
class World {
public:
  World();
  Entity spawn();
  void destroy(Entity entity);
  void update(float deltaTime);
  void render();

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
                 int priority = 0, bool isRender = false);

private:
  std::vector<bool> activeEntities;
  std::unordered_map<size_t, std::unique_ptr<IComponentArray>> components;
  std::unordered_map<std::type_index, std::shared_ptr<void>> resources;
  std::vector<SystemInfo> updateSystems;
  std::vector<SystemInfo> renderSystems;
  size_t nextEntityId = 0;

  template <typename... Ts>
  requires(Component<Ts> &&...) friend class Query;
};

// Entity template implementations (after World is defined)
template <Component T> Entity &Entity::with(T component) {
  world->setComponent(id, component);
  mask.set(ComponentId::get<T>());
  return *this;
}

template <Component T> T &Entity::get() { return world->getComponent<T>(id); }

template <Component T> bool Entity::has() const {
  return world->hasComponent<T>(id);
}

// Query class (needs full World definition)
template <typename... Components>
requires(Component<Components> &&...) class Query {
  friend class World; // Add friendship to access Entity constructor
public:
  Query(World *world) : world(world) {
    // Find the component array with the smallest size for optimal iteration
    size_t minSize = std::numeric_limits<size_t>::max();
    size_t smallestComponentId = 0;

    // Get all component IDs and find the smallest array
    std::array<size_t, sizeof...(Components)> componentIds = {
        ComponentId::get<Components>()...};

    for (size_t componentId : componentIds) {
      auto it = world->components.find(componentId);
      if (it != world->components.end()) {
        size_t currentSize = it->second->size();
        if (currentSize < minSize) {
          minSize = currentSize;
          smallestComponentId = componentId;
        }
      } else {
        // If any required component type doesn't exist, we can return early
        matches.clear();
        return;
      }
    }

    // Get the smallest component array for base iteration
    auto &baseArray = world->components[smallestComponentId];
    auto *typedBaseArray = static_cast<
        ComponentArray<std::tuple_element_t<0, std::tuple<Components...>>> *>(
        baseArray.get());

    // Reserve space for efficiency
    matches.reserve(minSize);

    // Iterate through the entities that have the smallest component array
    const auto &baseEntities = typedBaseArray->entities();
    for (size_t entityId : baseEntities) {
      if (!world->activeEntities[entityId]) {
        continue;
      }

      // Check if entity has all other required components
      bool hasAll = true;

      // Using fold expression to check all components
      ([&] {
        auto componentId = ComponentId::get<Components>();
        if (componentId != smallestComponentId) {
          hasAll = hasAll && world->hasComponent<Components>(entityId);
        }
        return hasAll;
      }() &&
       ...);

      if (hasAll) {
        // Use push_back instead of emplace_back since Entity constructor is
        // private
        matches.push_back(Entity(world, entityId));
      }
    }

    // Sort matches by entity ID for consistent iteration order
    std::sort(
        matches.begin(), matches.end(),
        [](const Entity &a, const Entity &b) { return a.getId() < b.getId(); });
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

private:
  World *world;
  std::vector<Entity> matches;
};

} // namespace ste