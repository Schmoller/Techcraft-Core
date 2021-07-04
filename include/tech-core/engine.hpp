#ifndef ENGINE_HPP
#define ENGINE_HPP

#include "forward.hpp"

// Forward declarations
namespace Engine {
struct QueueFamilyIndices;
struct SwapChainSupportDetails;
}

#include "common_includes.hpp"

#include "task.hpp"
#include "inputmanager.hpp"
#include "tech-core/gui/manager.hpp"
#include "tech-core/subsystem/base.hpp"
#include "post_processing.hpp"


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

    void setScene(const std::shared_ptr<Scene> &);

    const std::shared_ptr<Scene> &getScene() const { return currentScene; }

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

    const Texture *getTexture(const std::string &name);

    const Texture *getTexture(const char *name);

    void destroyTexture(const std::string &name);

    void destroyTexture(const char *name);

    ImageBuilder createImage(uint32_t width, uint32_t height);
    ImageBuilder createImageArray(uint32_t width, uint32_t height, uint32_t count);

    // ==============================================
    //  Material Methods
    // ==============================================
    MaterialBuilder createMaterial(const std::string &name);

    Material *getMaterial(const std::string &name);

    Material *getMaterial(const char *name);

    void destroyMaterial(const std::string &name);

    void destroyMaterial(const char *name);

    // ==============================================
    //  Builders
    // ==============================================
    PipelineBuilder createPipeline(Subsystem::SubsystemLayer layer = Subsystem::SubsystemLayer::Main);
    PipelineBuilder createPipeline(vk::RenderPass, uint32_t colorAttachmentCount);
    ComputeTaskBuilder createComputeTask();

    // ==============================================
    //  Managers
    // ==============================================

    TextureManager &getTextureManager() {
        return *textureManager;
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
        return *materialManager;
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
    //  Post processing effects
    // ==============================================
    EffectBuilder createEffect(const std::string &name);
    void addEffect(const std::shared_ptr<Effect> &effect);

    std::shared_ptr<Effect> getEffect(const std::string &name);

    void removeEffect(const std::string &name);

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
    std::vector<std::shared_ptr<Image>> intermediateAttachments;

    // Depth resources
    std::shared_ptr<Image> finalDepthAttachment;

    struct {
        vk::RenderPass renderPass;
        std::vector<vk::Framebuffer> framebuffers;
        vk::CommandBuffer commandBuffer;
    } layerMain;

    struct {
        vk::RenderPass renderPass;
        std::vector<vk::Framebuffer> framebuffers;
        vk::CommandBuffer commandBuffer;
    } layerOverlay;

    vk::CommandBuffer guiCommandBuffer;
    std::vector<Buffer> uniformBuffers;
    std::unique_ptr<ExecutionController> executionController;

    std::unique_ptr<TextureManager> textureManager;
    std::unique_ptr<MaterialManager> materialManager;
    std::unique_ptr<BufferManager> bufferManager;
    std::unique_ptr<TaskManager> taskManager;
    std::unique_ptr<Gui::GuiManager> guiManager;
    std::unique_ptr<FontManager> fontManager;
    std::shared_ptr<Scene> currentScene;
    std::unique_ptr<Internal::DescriptorCacheManager> descriptorManager;

    InputManager inputManager;

    std::vector<std::shared_ptr<Effect>> effects;
    std::unordered_map<std::string, std::shared_ptr<Effect>> effectsByName;

    // Camera
    Camera *camera = nullptr;

    // Resources
    std::unordered_map<std::string, std::unique_ptr<Mesh>> meshes;
    std::unordered_map<const void *, std::unique_ptr<Subsystem::Subsystem>> subsystems;
    std::vector<Subsystem::Subsystem *> orderedSubsystems;


    bool framebufferResized = false;

    // void initializeVulkan(std::vector<const char *> extensions);

    // void createInstance(std::vector<const char *> extensions);

    void initWindow(const std::string_view &title);

    static void framebufferResizeCallback(GLFWwindow *window, int width, int height);

    void initVulkan();

    void recreateSwapChain();

    void createSurface();

    void createInstance();

    void createAttachments();
    void createMainRenderPass();
    void createOverlayRenderPass();
    void updateEffectPipelines();

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
        vk::CommandBuffer, uint32_t currentImage, Subsystem::SubsystemLayer layer
    );

    void updateUniformBuffer(uint32_t currentImage);

    void cleanupSwapChain();

    void cleanup();

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