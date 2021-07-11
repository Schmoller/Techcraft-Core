#include "tech-core/shader/requirements.hpp"

namespace Engine {

void PipelineRequirements::addOutputAttachment(const IODefinition &def) {
    outputAttachments.emplace_back(def);
}

void PipelineRequirements::addVertexDefinition(const IODefinition &def) {
    vertexDefinition.emplace_back(def);
}
}