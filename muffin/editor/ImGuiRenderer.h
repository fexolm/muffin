#pragma once

#include "muffin/rhi/RHI.h"

#include <fstream>
#include <glm/glm.hpp>
#include <imgui.h>
#include <stdexcept>
#include <vector>

class ImGuiRenderer
{
public:
	ImGuiRenderer(RHIDriverRef driver);

	void Render(RHICommandListRef commandList);

private:
	RHIDriverRef driver;
	RHIGraphicsPipelineRef pipeline;
	RHISamplerRef fontSampler;
	RHITextureRef fontTexture;
};