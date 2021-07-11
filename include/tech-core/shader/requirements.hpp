#pragma once

#include "common.hpp"
#include <cinttypes>
#include <vector>

namespace Engine {

class PipelineRequirements {
public:
    struct IODefinition {
        uint32_t location;
        ShaderValueType valueType;
    };

    const std::vector<IODefinition> &getOutputAttachments() const { return outputAttachments; }

    const std::vector<IODefinition> &getVertexDefinition() const { return vertexDefinition; }

private:
    std::vector<IODefinition> outputAttachments;
    std::vector<IODefinition> vertexDefinition;
};

}