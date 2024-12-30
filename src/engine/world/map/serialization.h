#pragma once

#include <fstream>
#include <istream>
#include <sstream>
#include <string>

#include <glm/glm.hpp>
#include <nlohmann/json.hpp>

#include "map.h"

using json = nlohmann::json;

namespace ste {

class MapSerializer {

public:
  static std::string serialize(const Map &mapInstance) {
    json map;
    json layers = json::array();

    for (const auto &layer : mapInstance.layers()) {
      json layerObj;
      layerObj["name"] = layer.getName();
      layerObj["depth"] = layer.getDepth();

      json tiles = json::array();
      for (const auto &tile : layer.getTiles()) {
        json tileObj;
        tileObj["id"] = tile.getId();
        tileObj["type"] = tile.getTileType();
        tileObj["position"] = {tile.getPosition().x, tile.getPosition().y};
        tileObj["size"] = {tile.getSize().x, tile.getSize().y};
        tiles.push_back(tileObj);
      }

      layerObj["tiles"] = tiles;
      layers.push_back(layerObj);
    }

    map["layers"] = layers;

    return map.dump(-1);
  }

  static Map deserialize(const std::string &jsonStr) {
    Map mapInstance;

    try {
      json map = json::parse(jsonStr);

      std::vector<Layer> layers;
      for (const auto &layerJson : map["layers"]) {
        mapInstance.addLayer(layerJson["name"], layerJson["depth"]);

        for (const auto &tileJson : layerJson["tiles"]) {
          glm::vec2 position{tileJson["position"][0], tileJson["position"][1]};
          glm::vec2 size{tileJson["size"][0], tileJson["size"][1]};

          mapInstance.addTile(layerJson["name"], tileJson["type"], position,
                              size);
        }
      }

      return mapInstance;

    } catch (const json::exception &e) {
      throw std::runtime_error("Failed to parse map JSON: " +
                               std::string(e.what()));
    }
  }

  static Map deserializeFromFile(const std::string &filename) {
    std::ifstream file(filename);
    if (!file) {
      throw std::runtime_error("Could not open file: " + filename);
    }
    std::string jsonStr((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    return deserialize(jsonStr);
  }

  static void serializeToFile(const Map &mapInstance,
                              const std::string &filename) {
    std::ofstream file(filename);
    if (!file) {
      throw std::runtime_error("Could not open file for writing: " + filename);
    }
    file << serialize(mapInstance);
  }

private:
  MapSerializer() = delete;
  ~MapSerializer() = delete;
  MapSerializer(const MapSerializer &) = delete;
  MapSerializer &operator=(const MapSerializer &) = delete;
};

} // namespace ste