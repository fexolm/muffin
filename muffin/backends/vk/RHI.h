#pragma once
#include <memory>
#include <vulkan/vulkan_raii.hpp>

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

struct Image {
    vk::raii::Image image;
    vk::raii::DeviceMemory memory;
    vk::raii::ImageView view;
};

struct Sampler {
    vk::raii::Sampler sampler;
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

