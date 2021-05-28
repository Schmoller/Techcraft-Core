// #pragma once
//
// #include <string>
// #include <vector>
// #include <memory>
// #include <map>
// #include <vulkan/vulkan.hpp>
//
// namespace Engine {
//
// class Shader {
//
// };
//
// enum class ShaderType {
//     Vertex,
//     Fragment,
//     Compute,
//     Tessellation,
//     Geometry,
// };
//
// class ShaderBuilder {
// public:
//     ShaderBuilder &fromFile(const char *filename);
//     ShaderBuilder &fromFile(const std::string &filename);
//     ShaderBuilder &fromBytes(const char *bytes, size_t size);
//     ShaderBuilder &fromBytes(const std::vector<char> &);
//
//     ShaderBuilder &withType(ShaderType);
//
//     ShaderBuilder &withEntrypoint(const char *symbol);
//     ShaderBuilder &withEntrypoint(const std::string &symbol);
//
//     ShaderBuilder &withImage(uint32_t binding, uint32_t set = 0);
//     ShaderBuilder &withStorageImage(uint32_t binding, uint32_t set = 0);
//     ShaderBuilder &withSampler(uint32_t binding, uint32_t set = 0);
//     ShaderBuilder &withSamplerImmutable(vk::Sampler, uint32_t binding, uint32_t set = 0);
//     ShaderBuilder &withCombinedImageSampler(uint32_t binding, uint32_t set = 0);
//     ShaderBuilder &withCombinedImageSamplerImmutable(vk::Sampler, uint32_t binding, uint32_t set = 0);
//     ShaderBuilder &withUniformBuffer(uint32_t binding, uint32_t set = 0);
//     ShaderBuilder &withStorageBuffer(uint32_t binding, uint32_t set = 0);
//
//     ShaderBuilder &withImage(std::shared_ptr<Image>, uint32_t binding, uint32_t set = 0);
//     ShaderBuilder &withStorageImage(std::shared_ptr<Image>, uint32_t binding, uint32_t set = 0);
//     ShaderBuilder &withSampler(vk::Sampler, uint32_t binding, uint32_t set = 0);
//     ShaderBuilder &withCombinedImageSampler(std::shared_ptr<Image>, vk::Sampler, uint32_t binding, uint32_t set = 0);
//     ShaderBuilder &withUniformBuffer(std::shared_ptr<Buffer>, uint32_t binding, uint32_t set = 0);
//     ShaderBuilder &withStorageBuffer(std::shared_ptr<Buffer>, uint32_t binding, uint32_t set = 0);
//
//     std::shared_ptr<Shader> build();
// private:
//     std::vector<char> shaderBytes;
//     std::string entrypoint { "main" };
//     ShaderType shaderType { ShaderType::Vertex };
//
//     std::map<uint32_t, std::vector<vk::DescriptorSetLayoutBinding>> bindings;
// };
//
// }
//
//
