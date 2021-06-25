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
class Material;
class MaterialBuilder;

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

class EffectBuilder;
class Effect;

// Scene
class Scene;
class Entity;
class Component;

// Gui
namespace Gui {
class Drawer;
}

namespace Internal {
class TextureArray;
class RenderPlanner;
}

}