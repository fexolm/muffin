#pragma once

#include <map>
#include <memory>
#include <vulkan/vulkan.h>
#include <vector>

enum class BufferUsage
{
	Vertex,
	Index,
	Uniform,
	Staging,
};

enum VertexElementType
{
	None,
	Float1,
	Float2,
	Float3,
	Float4,
	PackedNormal, // FPackedNormal
	UByte4,
	UByte4N,
	Color,
	Short2,
	Short4,
	Short2N, // 16 bit word normalized to (value/32767.0,value/32767.0,0,0,1)
	Half2,	 // 16 bit float using 1 bit sign, 5 bit exponent, 10 bit mantissa
	Half4,
	Short4N, // 4 X 16 bit word, normalized
	UShort2,
	UShort4,
	UShort2N,  // 16 bit word normalized to (value/65535.0,value/65535.0,0,0,1)
	UShort4N,  // 4 X 16 bit word unsigned, normalized
	URGB10A2N, // 10 bit r, g, b and 2 bit a normalized to (value/1023.0f, value/1023.0f, value/1023.0f, value/3.0f)
	UInt,
	MAX,

	NumBits = 5,
};

struct BufferInfo
{
	BufferUsage usage;
};

class RHIResource
{
public:
	virtual ~RHIResource() = default;
};

class RHIBuffer : public RHIResource
{
public:
	virtual ~RHIBuffer() override = default;

	virtual void Write(void* data, uint32_t size) = 0;
};

class RHITexture : public RHIResource
{
};

using RHITextureRef = std::shared_ptr<RHITexture>;

class RHISampler : public RHIResource
{
};

using RHISamplerRef = std::shared_ptr<RHISampler>;

class RHIGraphicsPipeline
{
public:
	virtual ~RHIGraphicsPipeline() = default;
};

using RHIGraphicsPipelineRef = std::shared_ptr<RHIGraphicsPipeline>;

enum class ShaderType
{
	Vertex,
	Fragment
};

struct RHIShader : RHIResource
{
};

using RHIShaderRef = std::shared_ptr<RHIShader>;

class RHIRenderTarget
{
};

using RHIRenderTargetRef = std::shared_ptr<RHIRenderTarget>;

struct GraphicsPipelineCreateInfo
{
	RHIShaderRef vertexShader;
	RHIShaderRef fragmentShader;
};

using RHIBufferRef = std::shared_ptr<RHIBuffer>;
using RHIResourceRef = std::shared_ptr<RHIResource>;

class RHICommandList : public RHIResource
{
public:
	virtual void Begin() = 0;

	virtual void End() = 0;

	virtual void BindPipeline(const RHIGraphicsPipelineRef& pipeline) = 0;

	virtual void BeginRenderPass(const RHIRenderTargetRef& renderTarget) = 0;

	virtual void EndRenderPass() = 0;

	virtual void BindVertexBuffer(const RHIBufferRef& buf, int binding) = 0;

	virtual void BindIndexBuffer(const RHIBufferRef& buf) = 0;

	virtual void BindUniformBuffer(const std::string& name, const RHIBufferRef& buffer, int size) = 0;

	virtual void BindTexture(const std::string& name, const RHITextureRef& texture, const RHISamplerRef& sampler) = 0;

	virtual void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset,
		uint32_t firstInstance) = 0;

    virtual void DrawImGui() = 0;

	virtual void SetViewport() = 0;

	virtual void SetScissors() = 0;
};

using RHICommandListRef = std::shared_ptr<RHICommandList>;

class RHIDriver
{
public:
	virtual ~RHIDriver() = default;

	virtual RHIShaderRef CreateShader(const std::vector<uint32_t>& code, ShaderType type) = 0;

	virtual RHIBufferRef CreateBuffer(size_t size, const BufferInfo& info) = 0;

	virtual RHIGraphicsPipelineRef CreateGraphicsPipeline(const GraphicsPipelineCreateInfo& info) = 0;

	virtual RHICommandListRef CreateCommandList() = 0;

	virtual void Submit(RHICommandListRef& commandList) = 0;

	virtual void WaitIdle() = 0;

	virtual void SubmitAndWaitIdle(RHICommandListRef& commandList) = 0;

	virtual RHIRenderTargetRef BeginFrame() = 0;

	virtual void EndFrame() = 0;

	virtual RHITextureRef CreateTexture(uint32_t width, uint32_t height) = 0;

	virtual RHISamplerRef CreateSampler() = 0;

	virtual void CopyBufferToTexture(const RHIBufferRef& buf, RHITextureRef& image, uint32_t width, uint32_t height) = 0;

    virtual void InitImGui() = 0;
};

using RHIDriverRef = std::shared_ptr<RHIDriver>;
