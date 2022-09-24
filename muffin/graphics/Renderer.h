#pragma once

#include "Renderable.h"
#include "muffin/graphics/rhi/RHI.h"
#include <vector>

class Renderer
{
public:
	Renderer(RHIDriverRef driver);

	void Enqueue(RenderableRef obj);

	void Render();

private:
	RHIDriverRef driver;
	std::vector<RenderableRef> renderQueue;
};