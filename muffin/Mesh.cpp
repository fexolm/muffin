#include "Mesh.h"

Mesh::Mesh(
	RHIDriverRef driver,
	const std::vector<glm::vec3>& triangles,
	const std::vector<uint16_t>& indices,
	const std::vector<glm::vec3>& colors,
	const std::vector<glm::vec2>& texCoords)
{

	BufferInfo vertexInfo;
	vertexInfo.usage = BufferUsage::Vertex;

	trianglesBuf = driver->CreateBuffer(triangles.size() * sizeof(glm::vec3), vertexInfo);
	trianglesBuf->Write((void*)triangles.data(), triangles.size() * sizeof(glm::vec3));

	colorsBuf = driver->CreateBuffer(colors.size() * sizeof(glm::vec3), vertexInfo);
	colorsBuf->Write((void*)colors.data(), colors.size() * sizeof(glm::vec3));

	texCoordsBuf = driver->CreateBuffer(texCoords.size() * sizeof(glm::vec2), vertexInfo);
	texCoordsBuf->Write((void*)texCoords.data(), texCoords.size() * sizeof(glm::vec2));

	BufferInfo indexInfo;
	indexInfo.usage = BufferUsage::Index;

	indicesBuf = driver->CreateBuffer(indices.size() * sizeof(uint16_t), indexInfo);
	indicesBuf->Write((void*)indices.data(), indices.size() * sizeof(uint16_t));
	indicesSize = indices.size();
}

void Mesh::Draw(RHICommandListRef commandList)
{
	commandList->BindVertexBuffer(trianglesBuf, 0);
	commandList->BindVertexBuffer(colorsBuf, 1);
	commandList->BindVertexBuffer(texCoordsBuf, 2);
	commandList->BindIndexBuffer(indicesBuf);

	commandList->DrawIndexed(indicesSize, 1, 0, 0, 0);
}

MeshRef Mesh::Create(
	RHIDriverRef driver,
	const std::vector<glm::vec3>& triangles,
	const std::vector<uint16_t>& indices,
	const std::vector<glm::vec3>& colors,
	const std::vector<glm::vec2>& texCoords)
{
	return MeshRef(new Mesh(driver, triangles, indices, colors, texCoords));
}
