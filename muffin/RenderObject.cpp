#include "RenderObject.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

RenderObjectRef RenderObject::Create(const std::string &name, MeshRef mesh, MaterialRef material)
{
	return RenderObjectRef(new RenderObject(name, mesh, material));
}

RenderObject::RenderObject(const std::string &name, MeshRef mesh, MaterialRef material)
	: name(name), mesh(mesh), material(material)
{
}

void RenderObject::Draw(RHICommandListRef commandList)
{
	material->Bind(commandList);
	commandList->SetViewport();
	commandList->SetScissors();
	mesh->Draw(commandList);
}

void RenderObject::SetTransform(const glm::mat4 &newTransform)
{
    transform = newTransform;

    UniformBufferObject ubo;
    ubo.view = glm::lookAt(glm::vec3(0.0f, 5.0f, 5.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.proj = glm::perspective(glm::radians(45.0f), 800.f / 600.f, 0.1f, 10.0f);
    ubo.model = transform;
    ubo.proj[1][1] *= -1;
	material->UpdateUBO(ubo);
}

const glm::mat4 &RenderObject::GetTransform() {
    return transform;
}

const std::string &RenderObject::Name() {
    return name;
}