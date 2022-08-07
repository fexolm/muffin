#pragma once
#include <memory>
#include <vulkan/vulkan_raii.hpp>
#include <map>

enum class BufferUsage {
    Vertex,
    Index,
    Uniform,
    Staging,
};

struct BufferInfo {
    BufferUsage usage;
};

class RHIResource {
public:
    virtual ~RHIResource() = default;
};

class RHIBuffer : public RHIResource {
public:
    virtual ~RHIBuffer() override = default;

    virtual void Write(void *data, uint32_t size) = 0;
};

class RHIGraphicsPipeline {
public:
    virtual ~RHIGraphicsPipeline() = default;
};

using RHIGraphicsPipelineRef = std::shared_ptr<RHIGraphicsPipeline>;

struct Image {
    vk::raii::Image image;
    vk::raii::DeviceMemory memory;
    vk::raii::ImageView view;
};

struct Sampler {
    vk::raii::Sampler sampler;
};

enum class ShaderType {
    Vertex,
    Fragment
};

struct Shader {
    explicit Shader(vk::raii::ShaderModule &&module);

    vk::raii::ShaderModule module;

    std::map<int, std::vector<VkDescriptorSetLayoutBinding>> bindings;
    std::vector<VkVertexInputAttributeDescription> vertexAttributes;
    std::vector<VkVertexInputBindingDescription> vertexBindings;
};

struct GraphicsPipelineCreateInfo {
    std::shared_ptr<Shader> vertexShader;
    std::shared_ptr<Shader> fragmentShader;
};

using RHIBufferRef = std::shared_ptr<RHIBuffer>;
using RHIResourceRef = std::shared_ptr<RHIResource>;


class RHIDescriptorSet : public RHIResource {
public:
    virtual ~RHIDescriptorSet() override= default;

    virtual void Update(int binding, const RHIBufferRef &buf, int size) = 0;

    virtual void Update(int binding, Image &image, Sampler &sampler) = 0;
};

using RHIDescriptorSetRef = std::shared_ptr<RHIDescriptorSet>;

#define VULKAN_RHI_SAFE_CALL(Result) do {if((Result) != VK_SUCCESS) {}} while(0)


