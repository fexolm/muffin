#pragma once

#include "muffin/rhi/RHI.h"

#include <memory>

struct Renderable
{
	virtual void Render(RHICommandListRef commandList) = 0;
};

using RenderableRef = std::shared_ptr<Renderable>;