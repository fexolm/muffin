#include "RenderObject.h"

RenderObjectRef RenderObject::Create(MeshRef mesh, MaterialRef material)
{
	return RenderObjectRef(new RenderObject(mesh, material));
}

RenderObject::RenderObject(MeshRef mesh, MaterialRef material)
	: mesh(mesh), material(material)
{
}

void RenderObject::Draw(RHICommandListRef commandList)
{
	material->Bind(commandList);
	commandList->SetViewport();
	commandList->SetScissors();
	mesh->Draw(commandList);
}

void RenderObject::UpdateUBO(const UniformBufferObject& newUBO)
{
	material->UpdateUBO(newUBO);
}
