#ifndef ENGINE_HPP
#define ENGINE_HPP

#include "forward.hpp"

// Forward declarations
namespace Engine {
struct QueueFamilyIndices;
struct SwapChainSupportDetails;
}

#include "common_includes.hpp"

#include "material.hpp"
#include "task.hpp"
#include "texturemanager.hpp"
#include "inputmanager.hpp"
#include "tech-core/gui/manager.hpp"
#include "tech-core/subsystem/base.hpp"

#include <optional>
#include <unordered_map>
#include <vector>
#include <set>
#include <deque>

namespace Engine {

class RenderEngine {
public:
    RenderEngine();

    ~RenderEngine();

    void initialize(const std::string_view &title);

    bool beginFrame();

    void render();

    InputManager &getInputManager() {
        return inputManager;
    }

    Gui::Rect getScreenBounds();

    // void reinitializeSwapChain();
    // void renderFrame();
    // ==============================================
    //  Camera Methods
    // ==============================================
    void setCamera(Camera &camera);

    const Camera *getCamera() const;

    // ==============================================
    //  Mesh Methods
    // ==============================================
    template<typename VertexType>
    StaticMeshBuilder<VertexType> createStaticMesh(const std::string &name);

    template<typename VertexType>
    DynamicMeshBuilder<VertexType> createDynamicMesh(const std::string &name);

    void removeMesh(const std::string &name);

    Mesh *getMesh(const std::string &name);

    // ==============================================
    //  Texture Methods
    // ==============================================
    TextureBuilder createTexture(const std::string &name);

    Texture *getTexture(const std::string &name);

    Texture *getTexture(const char *name);

    void destroyTexture(const std::string &name);

    void destroyTexture(const char *name);

    ImageBuilder createImage(uint32_t width, uint32_t height);

    // ==============================================
    //  Material Methods
    // ==============================================
    Material *createMaterial(const MaterialCreateInfo &createInfo);

    Material *getMaterial(const std::string &name);

    Material *getMaterial(const char *name);

    void destroyMaterial(const std::string &name);

    void destroyMaterial(const char *name);

    // ==============================================
    //  Builders
    // ==============================================
    PipelineBuilder createPipeline();

    // ==============================================
    //  Managers
    // ==============================================

    TextureManager &getTextureManager() {
        return textureManager;
    }

    BufferManager &getBufferManager() {
        return *bufferManager;
    }

    TaskManager &getTaskManager() {
        return *taskManager;
    }

    Gui::GuiManager &getGuiManager() {
        return *guiManager;
    }

    FontManager &getFontManager() {
        return *fontManager;
    }

    MaterialManager &getMaterialManager() {
        return materialManager;
    }

    // ==============================================
    //  Subsystems
    // ==============================================
    /**
     * Adds a subsystem to the render engine.
     * NOTE: Subsystems need to be added before initialization
     */
    template<typename T>
    void addSubsystem(const Subsystem::SubsystemID<T> &id) {
        auto subsystem = std::make_unique<T>();
        orderedSubsystems.push_back(subsystem.get());
        subsystems[&id] = std::move(subsystem);
    }

    template<typename T>
    T *getSubsystem(const Subsystem::SubsystemID<T> &id) {
        auto it = subsystems.find(&id);
        if (it == subsystems.end()) {
            return nullptr;
        } else {
            return static_cast<T *>(it->second.get());
        }
    }

    template<typename T>
    void removeSubsystem(const Subsystem::SubsystemID<T> &id) {
        T *subsystem = getSubsystem(id);
        if (subsystem) {
            auto it = orderedSubsystems.begin();
            for (; it != orderedSubsystems.end(); ++it) {
                if (&(*it) == subsystem) {
                    orderedSubsystems.erase(it);
                    break;
                }
            }
        }
        subsystems.erase(&id);
    }

    // ==============================================
    //  Utilities
    // ==============================================

    vk::DescriptorBufferInfo getCameraDBI(uint32_t imageIndex);

protected:


private:
    GLFWwindow *window;
    vk::Instance instance;
    vk::PhysicalDevice physicalDevice;
    VkSurfaceKHR surface;
    std::unique_ptr<VulkanDevice> device;
    std::unique_ptr<SwapChain> swapChain;
    vk::RenderPass renderPass;
    std::vector<vk::Framebuffer> swapChainFramebuffers;
    std::vector<vk::CommandBuffer> commandBuffers;
    vk::CommandBuffer renderCommandBuffer;
    vk::CommandBuffer guiCommandBuffer;
    std::vector<Buffer> uniformBuffers;

    vk::DescriptorSetLayout textureDescriptorLayout;
    TextureManager textureManager;
    MaterialManager materialManager;
    std::unique_ptr<BufferManager> bufferManager;
    std::unique_ptr<TaskManager> taskManager;
    std::unique_ptr<Gui::GuiManager> guiManager;
    std::unique_ptr<FontManager> fontManager;

    InputManager inputManager;


    // Camera
    Camera *camera = nullptr;

    // Resources
    std::unordered_map<std::string, std::unique_ptr<Mesh>> meshes;
    std::unordered_map<std::string, Image> textures;
    std::unordered_map<const void *, std::unique_ptr<Subsystem::Subsystem>> subsystems;
    std::vector<Subsystem::Subsystem *> orderedSubsystems;

    // Materials and pipelines



    // Stuff to be removed from engine
    Image depthImage;

    bool framebufferResized = false;

    // void initializeVulkan(std::vector<const char *> extensions);

    // void createInstance(std::vector<const char *> extensions);

    void initWindow(const std::string_view &title);

    static void framebufferResizeCallback(GLFWwindow *window, int width, int height);

    void initVulkan();

    void recreateSwapChain();

    void createSurface();

    void createInstance();

    void createRenderPass();

    void createDescriptorSetLayout();

    vk::ShaderModule createShaderModule(const std::vector<char> &code);

    void createFramebuffers();

    void createCommandBuffers();

    void createUniformBuffers();

    vk::CommandBuffer beginSingleTimeCommands();

    void endSingleTimeCommands(vk::CommandBuffer commandBuffer);

    vk::Format findSupportedFormat(
        const std::vector<vk::Format> &candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features
    );

    vk::Format findDepthFormat();

    void createDepthResources();

    void printExtensions();

    void mainLoop();

    void drawFrame();

    void fillFrameCommands(
        vk::CommandBuffer commandBuffer, vk::CommandBufferInheritanceInfo &cbInheritance, uint32_t currentImage
    );

    void updateUniformBuffer(uint32_t currentImage);

    void cleanupSwapChain();

    void cleanup();

    void loadPlaceholders();
};

template<typename VertexType>
StaticMeshBuilder<VertexType> RenderEngine::createStaticMesh(const std::string &name) {
    auto task = taskManager->createTask();

    return StaticMeshBuilder<VertexType>(
        *bufferManager,
        *taskManager,
        [this, name](std::unique_ptr<StaticMesh> &mesh) {
            this->meshes[name] = std::move(mesh);
        }
    );
}

template<typename VertexType>
DynamicMeshBuilder<VertexType> RenderEngine::createDynamicMesh(const std::string &name) {
    return DynamicMeshBuilder<VertexType>(
        *bufferManager,
        *taskManager,
        [this, name](std::unique_ptr<DynamicMesh<VertexType>> &mesh) {
            this->meshes[name] = std::move(mesh);
        }
    );
}

}

#endif