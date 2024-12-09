#include "ecs.h"

namespace ste {

bool operator<(const SystemInfo &a, const SystemInfo &b) {
  return a.priority < b.priority;
}

Entity::Entity(World *world, size_t id) : world(world), id(id) {}

size_t Entity::getId() const { return id; }

World::World() { addResource(std::make_shared<Time>()); }

Entity World::spawn() {
  if (nextEntityId >= activeEntities.size()) {
    activeEntities.resize(nextEntityId + 1);
  }
  activeEntities[nextEntityId] = true;
  Entity entity(this, nextEntityId++);
  emit(EntityCreated{entity});
  return entity;
}

void World::destroy(Entity entity) {
  if (entity.getId() < activeEntities.size()) {
    emit(EntityDestroyed{entity});

    activeEntities[entity.getId()] = false;
    for (auto &[typeId, array] : components) {
      if (array->has(entity.getId())) {
        emit(ComponentRemoved{entity, array->getTypeInfo()});
      }
      array->remove(entity.getId());
    }
  }
}

void World::update(float deltaTime) {
  if (auto time = getResource<Time>()) {
    time->deltaSeconds = deltaTime;
  }

  for (auto &system : updateSystems) {
    if (system.func) {
      system.func(*this);
    }
  }
}

void World::render() {
  for (auto &system : renderSystems) {
    if (system.func) {
      system.func(*this);
    }
  }
}

void World::addSystem(const std::string &label,
                      std::function<void(World &)> func, int priority,
                      bool isRender) {
  auto &systems = isRender ? renderSystems : updateSystems;
  systems.push_back(SystemInfo{
      .func = std::move(func), .priority = priority, .label = label});
}

} // namespace ste