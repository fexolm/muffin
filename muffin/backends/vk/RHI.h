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

enum VertexElementType {
    None,
    Float1,
    Float2,
    Float3,
    Float4,
    PackedNormal,    // FPackedNormal
    UByte4,
    UByte4N,
    Color,
    Short2,
    Short4,
    Short2N,        // 16 bit word normalized to (value/32767.0,value/32767.0,0,0,1)
    Half2,            // 16 bit float using 1 bit sign, 5 bit exponent, 10 bit mantissa
    Half4,
    Short4N,        // 4 X 16 bit word, normalized
    UShort2,
    UShort4,
    UShort2N,        // 16 bit word normalized to (value/65535.0,value/65535.0,0,0,1)
    UShort4N,        // 4 X 16 bit word unsigned, normalized
    URGB10A2N,        // 10 bit r, g, b and 2 bit a normalized to (value/1023.0f, value/1023.0f, value/1023.0f, value/3.0f)
    UInt,
    MAX,

    NumBits = 5,
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

struct DescriptorSetBindingPoint {
    int set;
    int binding;
};


struct Shader {
    explicit Shader(vk::raii::ShaderModule &&module);

    vk::raii::ShaderModule module;

    std::map<int, std::vector<VkDescriptorSetLayoutBinding>> bindings;
    std::vector<VkVertexInputAttributeDescription> vertexAttributes;
    std::vector<VkVertexInputBindingDescription> vertexBindings;

    std::unordered_map<std::string, DescriptorSetBindingPoint> params;
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


