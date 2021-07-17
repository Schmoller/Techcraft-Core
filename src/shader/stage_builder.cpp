#include "tech-core/shader/stage_builder.hpp"
#include "tech-core/shader/stage.hpp"
#include "vulkanutils.hpp"
#include <spirv_reflect.h>
#include <fstream>

namespace Engine {

ShaderStageBuilder &ShaderStageBuilder::fromBytes(const std::vector<uint8_t> &data, const char *symbol) {
    shaderData = data;
    entrypoint = std::string { symbol };
    return *this;
}

ShaderStageBuilder &ShaderStageBuilder::fromBytes(const std::vector<int8_t> &data, const char *symbol) {
    shaderData = { data.begin(), data.end() };
    entrypoint = std::string { symbol };
    return *this;
}

ShaderStageBuilder &ShaderStageBuilder::fromBytes(const uint8_t *buffer, size_t size, const char *symbol) {
    shaderData.resize(size);
    std::memcpy(shaderData.data(), buffer, size);
    entrypoint = std::string { symbol };
    return *this;
}

ShaderStageBuilder &ShaderStageBuilder::fromBytes(const int8_t *buffer, size_t size, const char *symbol) {
    shaderData.resize(size);
    std::memcpy(shaderData.data(), buffer, size);
    entrypoint = std::string { symbol };
    return *this;
}

ShaderStageBuilder &ShaderStageBuilder::fromFile(const char *path, const char *symbol) {
    std::ifstream file(path, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file");
    }

    auto fileSize = file.tellg();
    shaderData.resize(fileSize);
    file.seekg(0);
    file.read(reinterpret_cast<char *>(shaderData.data()), fileSize);
    file.close();

    entrypoint = std::string { symbol };

    return *this;
}

ShaderStageBuilder &ShaderStageBuilder::fromFile(const std::string_view &path, const char *symbol) {
    std::ifstream file(path.data(), std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file");
    }

    auto fileSize = file.tellg();
    shaderData.resize(fileSize);
    file.seekg(0);
    file.read(reinterpret_cast<char *>(shaderData.data()), fileSize);
    file.close();

    entrypoint = std::string { symbol };

    return *this;
}

ShaderStageBuilder &ShaderStageBuilder::withType(ShaderStageType shaderType) {
    type = shaderType;
    return *this;
}

ShaderStageBuilder &ShaderStageBuilder::withConstant(uint32_t constantId, bool value) {
    uint32_t offset = sizeof(uint32_t) * specializationData.size();
    specializationData.push_back(static_cast<VkBool32>(value));
    specializationEntries.emplace_back(constantId, offset, sizeof(VkBool32));
    return *this;
}

ShaderStageBuilder &ShaderStageBuilder::withVariable(
    std::string variableName, uint32_t bindingId, ShaderBindingType bindingType, ShaderBindingUsage usage
) {
    assert(!systemInputs.contains(bindingId));
    variables.emplace(bindingId, ShaderVariable { std::move(variableName), bindingId, bindingType, usage });
    return *this;
}

ShaderStageBuilder &ShaderStageBuilder::withVariable(
    const std::string_view &variableName, uint32_t bindingId, ShaderBindingType bindingType, ShaderBindingUsage usage
) {
    assert(!systemInputs.contains(bindingId));
    variables.emplace(bindingId, ShaderVariable { std::string(variableName), bindingId, bindingType, usage });
    return *this;
}

ShaderStageBuilder &ShaderStageBuilder::withSystemInput(ShaderSystemInput input, uint32_t bindingId) {
    assert(!variables.contains(bindingId));
    systemInputs.emplace(bindingId, input);
    return *this;
}

std::shared_ptr<ShaderStage> ShaderStageBuilder::build() const {
    return std::make_shared<ShaderStage>(*this);
}

}