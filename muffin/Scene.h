#pragma once

#include "RenderObject.h"

#include <string>

class Scene
{
public:
	void AddObject(RenderObjectRef object);

	RenderObjectRef GetObject(const std::string& name);

	const std::vector<RenderObjectRef>& GetObjects();

private:
	std::vector<RenderObjectRef> objects;
};