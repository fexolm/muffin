#pragma once

#include "RenderObject.h"
#include "muffin/rhi/RHI.h"

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
};