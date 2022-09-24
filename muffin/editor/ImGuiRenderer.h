#pragma once

#include "muffin/rhi/RHI.h"
#include "muffin/Renderable.h"

#include <fstream>
#include <glm/glm.hpp>
#include <imgui.h>
#include <stdexcept>
#include <vector>

class ImGuiRenderer : public Renderable
{
public:
	ImGuiRenderer(RHIDriverRef driver);

	virtual void Render(RHICommandListRef commandList) override;

private:
	RHIDriverRef driver;
	RHIGraphicsPipelineRef pipeline;
	RHISamplerRef fontSampler;
	RHITextureRef fontTexture;
};