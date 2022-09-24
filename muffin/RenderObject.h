#pragma once

#include "Material.h"
#include "Mesh.h"
#include "Renderable.h"
#include "muffin/rhi/RHI.h"

#include <memory>
#include <string>

class RenderObject;

using RenderObjectRef = std::shared_ptr<RenderObject>;

class RenderObject : public Renderable
{
public:
	static RenderObjectRef Create(const std::string& name, MeshRef mesh, MaterialRef material);

	virtual void Render(RHICommandListRef commandList) override;

	void SetTransform(const glm::mat4& newTransform);

	const glm::mat4& GetTransform();

	const std::string& Name();

private:
	RenderObject(const std::string& name, MeshRef mesh, MaterialRef material);

	std::string name;
	MeshRef mesh;
	MaterialRef material;
	glm::mat4 transform;
};