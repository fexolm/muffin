#pragma once

#include "Material.h"
#include "Mesh.h"
#include "muffin/rhi/RHI.h"

#include <memory>

class RenderObject;

using RenderObjectRef = std::shared_ptr<RenderObject>;

class RenderObject
{
public:
	static RenderObjectRef Create(MeshRef mesh, MaterialRef material);

	void Draw(RHICommandListRef commandList);

	void UpdateUBO(const UniformBufferObject& newUBO);

private:
	RenderObject(MeshRef mesh, MaterialRef material);

	MeshRef mesh;
	MaterialRef material;
};