#pragma once

#include "RenderObject.h"
#include "muffin/rhi/RHI.h"
#include "muffin/editor/ImGuiRenderer.h"
#include <vector>

class Renderer
{
public:
	Renderer(RHIDriverRef driver);

	void Enqueue(RenderObjectRef obj);

	void Render();

private:
	RHIDriverRef driver;
	std::vector<RenderObjectRef> renderQueue;
    std::shared_ptr<ImGuiRenderer> gui;
};