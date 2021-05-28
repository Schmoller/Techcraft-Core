// #include "tech-core/shader.hpp"
// #include "vulkanutils.hpp"
// 
// namespace Engine {
// 
// ShaderBuilder &ShaderBuilder::fromFile(const char *filename) {
//     shaderBytes = readFile(filename);
//     return *this;
// }
// 
// ShaderBuilder &ShaderBuilder::fromFile(const std::string &filename) {
//     shaderBytes = readFile(filename);
//     return *this;
// }
// 
// ShaderBuilder &ShaderBuilder::fromBytes(const char *bytes, size_t size) {
//     shaderBytes.resize(size);
//     std::memcpy(shaderBytes.data(), bytes, size);
//     return *this;
// }
// 
// ShaderBuilder &ShaderBuilder::fromBytes(const std::vector<char> &bytes) {
//     shaderBytes = bytes;
//     return *this;
// }
// 
// ShaderBuilder &ShaderBuilder::withType(ShaderType type) {
//     shaderType = type;
//     return *this;
// }
// 
// ShaderBuilder &ShaderBuilder::withEntrypoint(const char *symbol) {
//     entrypoint = symbol;
//     return *this;
// }
// 
// ShaderBuilder &ShaderBuilder::withEntrypoint(const std::string &symbol) {
//     entrypoint = symbol;
//     return *this;
// }
// 
// ShaderBuilder &ShaderBuilder::withImage(uint32_t binding, uint32_t set) {
//     auto it = bindings.find(set);
//     if (it == bindings.end()) {
//         bindings.emplace(set, std::vector<vk::DescriptorSetLayoutBinding>());
//     }
// 
// 
//     bindings.try_emplace()
//     bindings.emplace_back()
//     return *this;
// }
// 
// ShaderBuilder &ShaderBuilder::withStorageImage(uint32_t binding, uint32_t set) {
//     return *this;
// }
// 
// ShaderBuilder &ShaderBuilder::withSampler(uint32_t binding, uint32_t set) {
//     return *this;
// }
// 
// ShaderBuilder &ShaderBuilder::withSampler(vk::Sampler, uint32_t binding, uint32_t set) {
//     return *this;
// }
// 
// ShaderBuilder &ShaderBuilder::withCombinedImageSampler(uint32_t binding, uint32_t set) {
//     return *this;
// }
// 
// ShaderBuilder &ShaderBuilder::withCombinedImageSampler(vk::Sampler, uint32_t binding, uint32_t set) {
//     return *this;
// }
// 
// ShaderBuilder &ShaderBuilder::withUniformBuffer(uint32_t binding, uint32_t set) {
//     return *this;
// }
// 
// ShaderBuilder &ShaderBuilder::withStorageBuffer(uint32_t binding, uint32_t set) {
//     return *this;
// }
// 
// std::shared_ptr<Shader> ShaderBuilder::build() {
//     return std::shared_ptr<Shader>();
// }
// }
