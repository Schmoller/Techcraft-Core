#pragma once
namespace Engine {

class RenderEngine;
class InputManager;

class BufferManager;

class Buffer;
class DivisibleBuffer;
class ImageBuilder;

class Image;
class Camera;

class FPSCamera;
class FontManager;

class Font;
class TextureManager;

class TextureBuilder;
struct Texture;

class MaterialManager;
struct Material;
struct MaterialCreateInfo;

class TaskManager;
class Task;

// Meshes
class Mesh;
template<typename VertexType>
class StaticMeshBuilder;
class StaticMesh;
template<typename VertexType>
class DynamicMeshBuilder;
template<typename>
class DynamicMesh;
class Model;

// Shapes
class Frustum;
class BoundingSphere;
class BoundingBox;
class Plane;

// Internal
class VulkanDevice;
class Pipeline;
class PipelineBuilder;
class SwapChain;
class ExecutionController;

class ComputeTaskBuilder;
class ComputeTask;
class ComputeManager;

// Gui
namespace Gui {
class Drawer;
}
}