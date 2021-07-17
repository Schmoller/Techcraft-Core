#include "internal.hpp"
#include <cassert>

namespace Engine::Internal {

ShaderBindingUpdateFrequency getFrequency(ShaderBindingUsage usage) {
    switch (usage) {
        case ShaderBindingUsage::Material:
            return ShaderBindingUpdateFrequency::PerMaterial;
        case ShaderBindingUsage::Entity:
            return ShaderBindingUpdateFrequency::PerEntity;
        default:
            assert(false);
    }
}


ShaderBindingUpdateFrequency getFrequency(ShaderSystemInput input) {
    switch (input) {
        case ShaderSystemInput::Camera:
            return ShaderBindingUpdateFrequency::PerFrame;
        case ShaderSystemInput::Entity:
        case ShaderSystemInput::Light:
            return ShaderBindingUpdateFrequency::PerEntity;
        default:
            assert(false);
    }
}

std::unordered_map<uint32_t, ShaderBindingGroup> allocateBindingSets(
    std::unordered_map<uint32_t, ShaderVariable> &variables,
    std::unordered_map<uint32_t, ShaderSystemInput> &systemInputs
) {
    std::unordered_map<uint32_t, ShaderBindingGroup> sets;
    std::unordered_map<ShaderBindingUpdateFrequency, uint32_t> frequencySets;

    uint32_t nextSet = 0;

    for (auto &pair : systemInputs) {
        auto frequency = getFrequency(pair.second);

        uint32_t set;
        if (frequency == ShaderBindingUpdateFrequency::Custom) {
            set = nextSet++;
        } else {
            auto freqIt = frequencySets.find(frequency);
            if (freqIt == frequencySets.end()) {
                set = nextSet++;
                frequencySets.emplace(frequency, set);
            } else {
                set = freqIt->second;
            }
        }

        auto setIt = sets.find(set);
        if (setIt == sets.end()) {
            sets.emplace(
                set, ShaderBindingGroup {
                    frequency,
                    { pair.first }
                }
            );
        } else {
            setIt->second.bindings.emplace(pair.first);
        }
    }

    for (auto &variable : variables) {
        auto frequency = getFrequency(variable.second.usage);

        uint32_t set;
        if (frequency == ShaderBindingUpdateFrequency::Custom) {
            set = nextSet++;
        } else {
            auto freqIt = frequencySets.find(frequency);
            if (freqIt == frequencySets.end()) {
                set = nextSet++;
                frequencySets.emplace(frequency, set);
            } else {
                set = freqIt->second;
            }
        }

        auto setIt = sets.find(set);
        if (setIt == sets.end()) {
            sets.emplace(
                set, ShaderBindingGroup {
                    frequency,
                    { variable.second.bindingId }
                }
            );
        } else {
            setIt->second.bindings.emplace(variable.second.bindingId);
        }
    }

    return sets;
}

}
