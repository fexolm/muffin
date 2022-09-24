#pragma once

#include "muffin/graphics/rhi/RHI.h"

#include <glm/glm.hpp>
#include <memory>
#include <vector>

class Mesh;
using MeshRef = std::shared_ptr<Mesh>;

class Mesh
{
public:
	void Draw(RHICommandListRef commandList);

	static MeshRef Create(
		RHIDriverRef driver,
		const std::vector<glm::vec3>& triangles,
		const std::vector<uint16_t>& indices,
		const std::vector<glm::vec3>& colors,
		const std::vector<glm::vec2>& texCoords);

private:
	Mesh(
		RHIDriverRef driver,
		const std::vector<glm::vec3>& triangles,
		const std::vector<uint16_t>& indices,
		const std::vector<glm::vec3>& colors,
		const std::vector<glm::vec2>& texCoords);

	RHIBufferRef trianglesBuf;
	RHIBufferRef indicesBuf;
	RHIBufferRef colorsBuf;
	RHIBufferRef texCoordsBuf;

	int indicesSize;
};