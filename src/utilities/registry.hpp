#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>

namespace Utilities {

template <typename T>
class Registry {
    public:
    const T *lookup(const std::string &id) const {
        auto it = byName.find(id);
        if (it == byName.end()) {
            return nullptr;
        }

        return it->second.get();
    }

    const T *get(uint32_t slot) const {
        return slots[slot].get();
    }

    std::vector<const T *> all() const {
        // TODO: This would be better as an iterator
        std::vector<const T *> result(slots.size());
        for (size_t i = 0; i < slots.size(); ++i) {
            result[i] = slots[i].get();
        }

        return result;
    }

    protected:
    Registry(std::unordered_map<std::string, std::shared_ptr<T>> byName, std::vector<std::shared_ptr<T>> slots)
    : byName(byName), slots(slots)
    {}

    private:
    const std::unordered_map<std::string, std::shared_ptr<T>> byName;
    const std::vector<std::shared_ptr<T>> slots;
};


template <typename T, typename R = Registry<T>>
class RegistryBuilder {
    public:
    const T *add(std::shared_ptr<T> item) {
        uint32_t slotId = static_cast<uint32_t>(slots.size());

        slots.push_back(item);

        initializeItem(item.get(), slotId);
        
        // Add to registry
        slots.push_back(item);
        byName[item->id()] = item;

        return item.get();
    }
    virtual std::unique_ptr<R> build() = 0;

    protected:
    virtual void initializeItem(T *item, size_t slotId) = 0;

    std::unordered_map<std::string, std::shared_ptr<T>> byName;
    std::vector<std::shared_ptr<T>> slots;
};

}