// ecs.h
#pragma once

#include <algorithm>
#include <any>
#include <bitset>
#include <concepts>
#include <functional>
#include <limits>
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

// Event concept
template <typename T>
concept Event = std::is_standard_layout_v<T> && std::is_trivially_copyable_v<T>;

// Forward declarations
template <typename... Components>
requires(Component<Components> &&...) class Query;

class World;
class Entity;

// Event listener type
template <Event T> using EventListener = std::function<void(const T &)>;

// Entity declaration
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

// Built-in events
struct EntityCreated {
  Entity entity;
};
struct EntityDestroyed {
  Entity entity;
};
struct ComponentAdded {
  Entity entity;
  std::type_index componentType;
};
struct ComponentRemoved {
  Entity entity;
  std::type_index componentType;
};

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

// Component storage base class
class IComponentArray {
public:
  virtual ~IComponentArray() = default;
  virtual void remove(size_t entityId) = 0;
  virtual size_t size() const = 0;
  virtual bool has(size_t entityId) const = 0;
  virtual const std::type_info &getTypeInfo() const = 0;
};

// Component storage template
template <Component T> class ComponentArray : public IComponentArray {
public:
  void insert(size_t entityId, T component) {
    if (entityId >= sparse.size()) {
      sparse.resize(entityId + 1, -1);
    }

    if (sparse[entityId] != -1) {
      dense_components[sparse[entityId]] = component;
      return;
    }

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

  bool has(size_t entityId) const override {
    return entityId < sparse.size() && sparse[entityId] != -1;
  }

  const std::type_info &getTypeInfo() const override { return typeid(T); }

  void remove(size_t entityId) override {
    if (entityId >= sparse.size() || sparse[entityId] == -1) {
      return;
    }

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

  auto begin() { return dense_components.begin(); }
  auto end() { return dense_components.end(); }
  const auto &entities() const { return dense_entities; }

private:
  std::vector<T> dense_components;
  std::vector<size_t> dense_entities;
  std::vector<int> sparse;
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
    bool isNewComponent = !components.contains(componentId);

    if (isNewComponent) {
      components[componentId] = std::make_unique<ComponentArray<T>>();
    }

    auto *array =
        static_cast<ComponentArray<T> *>(components[componentId].get());
    bool hadComponent = array->has(entityId);
    array->insert(entityId, component);

    if (!hadComponent) {
      emit(ComponentAdded{Entity(this, entityId), std::type_index(typeid(T))});
    }
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

  // Event system
  template <Event T> void subscribe(EventListener<T> listener) {
    auto &listeners = eventListeners[std::type_index(typeid(T))];
    listeners.push_back(std::move(listener));
  }

  template <Event T> void emit(const T &event) {
    auto it = eventListeners.find(std::type_index(typeid(T)));
    if (it != eventListeners.end()) {
      for (const auto &listener : it->second) {
        std::any_cast<EventListener<T>>(listener)(event);
      }
    }
  }

private:
  std::vector<bool> activeEntities;
  std::unordered_map<size_t, std::unique_ptr<IComponentArray>> components;
  std::unordered_map<std::type_index, std::shared_ptr<void>> resources;
  std::vector<SystemInfo> updateSystems;
  std::vector<SystemInfo> renderSystems;
  std::unordered_map<std::type_index, std::vector<std::any>> eventListeners;
  size_t nextEntityId = 0;

  template <typename... Ts>
  requires(Component<Ts> &&...) friend class Query;
};

// Entity template implementations
template <Component T> Entity &Entity::with(T component) {
  world->setComponent(id, component);
  mask.set(ComponentId::get<T>());
  return *this;
}

template <Component T> T &Entity::get() { return world->getComponent<T>(id); }

template <Component T> bool Entity::has() const {
  return world->hasComponent<T>(id);
}

// Query class implementation
template <typename... Components>
requires(Component<Components> &&...) class Query {
  friend class World;

public:
  Query(World *world) : world(world) {
    size_t minSize = std::numeric_limits<size_t>::max();
    size_t smallestComponentId = 0;

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
        matches.clear();
        return;
      }
    }

    auto &baseArray = world->components[smallestComponentId];
    auto *typedBaseArray = static_cast<
        ComponentArray<std::tuple_element_t<0, std::tuple<Components...>>> *>(
        baseArray.get());

    matches.reserve(minSize);

    const auto &baseEntities = typedBaseArray->entities();
    for (size_t entityId : baseEntities) {
      if (!world->activeEntities[entityId]) {
        continue;
      }

      bool hasAll = true;
      ([&] {
        auto componentId = ComponentId::get<Components>();
        if (componentId != smallestComponentId) {
          hasAll = hasAll && world->hasComponent<Components>(entityId);
        }
        return hasAll;
      }() &&
       ...);

      if (hasAll) {
        matches.push_back(Entity(world, entityId));
      }
    }

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