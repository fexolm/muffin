#include "Material.h"

Material::Material(RHIDriverRef driver, RHIShaderRef vertexShader, RHIShaderRef fragmentShader, RHITextureRef texture)
	: driver(driver), texture(texture)
{
	GraphicsPipelineCreateInfo createInfo;
	createInfo.vertexShader = vertexShader;
	createInfo.fragmentShader = fragmentShader;

	graphicsPipeline = driver->CreateGraphicsPipeline(createInfo);
	sampler = driver->CreateSampler();

	BufferInfo uboInfo;
	uboInfo.usage = BufferUsage::Uniform;
	uboBuffer = driver->CreateBuffer(sizeof(UniformBufferObject), uboInfo);
}

void Material::Bind(RHICommandListRef commandList)
{
	commandList->BindPipeline(graphicsPipeline);
	commandList->BindTexture("texSampler", texture, sampler);
	commandList->BindUniformBuffer("ubo", uboBuffer, sizeof(UniformBufferObject));
}

MaterialRef Material::Create(RHIDriverRef driver, RHIShaderRef vertexShader, RHIShaderRef fragmentShader, RHITextureRef texture)
{
	return MaterialRef(new Material(driver, vertexShader, fragmentShader, texture));
}

void Material::UpdateUBO(const UniformBufferObject& newUBO)
{
	BufferInfo uboInfo;
	uboInfo.usage = BufferUsage::Uniform;

	uboBuffer = driver->CreateBuffer(sizeof(UniformBufferObject), uboInfo);
	uboBuffer->Write((void*)&newUBO, sizeof(UniformBufferObject));
}
