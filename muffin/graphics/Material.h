#pragma once

#include "muffin/graphics/rhi/RHI.h"
#include <string>

#include <glm/glm.hpp>
#include <memory>

struct UniformBufferObject
{
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;
};

class Material;
using MaterialRef = std::shared_ptr<Material>;

class Material
{
public:
	void Bind(RHICommandListRef commandList);

	static MaterialRef Create(RHIDriverRef driver, RHIShaderRef vertexShader, RHIShaderRef fragmentShader, RHITextureRef texture);

	void UpdateUBO(const UniformBufferObject& newUBO);

private:
	Material(RHIDriverRef driver, RHIShaderRef vertexShader, RHIShaderRef fragmentShader, RHITextureRef texture);

	RHIDriverRef driver;
	RHIGraphicsPipelineRef graphicsPipeline;
	RHITextureRef texture;
	RHIBufferRef uboBuffer;
	RHISamplerRef sampler;
};