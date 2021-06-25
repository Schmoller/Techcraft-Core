#pragma once

#include "tech-core/forward.hpp"
#include <string>
#include <memory>
#include <unordered_map>
#include <vector>

namespace Engine {

class MaterialManager {
public:
    const Material *get(const std::string &name) const;
    Material *get(const std::string &name);

    Material *add(const MaterialBuilder &);
    MaterialBuilder add(const std::string &name);

    void remove(const Material *);
    void remove(const std::string &name);

    std::vector<const Material *> getMaterials() const;
private:
    std::unordered_map<std::string, std::shared_ptr<Material>> materials;
};

}
