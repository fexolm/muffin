#include "muffin/editor/ImGuiRenderer.h"
#include "muffin/graphics/Material.h"
#include "muffin/graphics/Mesh.h"
#include "muffin/graphics/RenderObject.h"
#include "muffin/graphics/Renderer.h"
#include "muffin/graphics/Scene.h"
#include "muffin/graphics/rhi/RHI.h"
#include "muffin/graphics/rhi/vulkan/RHI.h"

#include <chrono>
#include <fstream>

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include "stb_image.h"
#include <glm/ext.hpp>
#include <glm/geometric.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>
#include <imgui_impl_sdl.h>
#include <imgui_impl_vulkan.h>

#define SDL_MAIN_HANDLED
#include <SDL.h>

static std::vector<uint32_t> readFile(const std::string& filename)
{
	std::ifstream file(filename, std::ios::ate | std::ios::binary);
	if (!file.is_open()) {
		throw std::runtime_error("failed to open file!");
	}

	size_t fileSize = (size_t)file.tellg();
	std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));
	file.seekg(0);
	file.read((char*)buffer.data(), fileSize);
	file.close();

	return buffer;
}

void DrawGUI(Scene& scene)
{
	static float f = 0.0f;
	static int counter = 0;

	ImGui::Begin("Hello, world!"); // Create a window called "Hello, world!" and append into it.

	for (RenderObjectRef obj : scene.GetObjects()) {
		if (ImGui::Button(obj->Name().c_str())) {
			glm::mat4 transform = obj->GetTransform();
			obj->SetTransform(glm::rotate(transform, glm::radians(15.f), glm::vec3(0, 0, 1)));
		}
	}

	ImGui::End();
}

int main()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	(void)io;
	ImGui::StyleColorsDark();

	auto startTime = std::chrono::high_resolution_clock::now();

	UniformBufferObject ubo{};

	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, "viking_room.obj")) {
		throw std::runtime_error(warn + err);
	}

	std::vector<glm::vec3> positions;
	std::vector<glm::vec3> colors;
	std::vector<glm::vec2> texCoords;
	std::vector<uint16_t> indices;

	for (const auto& shape : shapes) {
		for (const auto& index : shape.mesh.indices) {
			positions.emplace_back(attrib.vertices[3 * index.vertex_index + 0],
				attrib.vertices[3 * index.vertex_index + 1],
				attrib.vertices[3 * index.vertex_index + 2]);
			colors.emplace_back(1.0f, 1.0f, 1.0f);
			texCoords.emplace_back(attrib.texcoords[2 * index.texcoord_index + 0],
				1.0f - attrib.texcoords[2 * index.texcoord_index + 1]);
			indices.push_back(indices.size());
		}
	}

	RHIDriverRef rhi = CreateVulkanRhi();

	Renderer renderer(rhi);

	MeshRef mesh = Mesh::Create(rhi, positions, indices, colors, texCoords);

	auto vertFile = readFile("vert.spv");
	auto fragFile = readFile("frag.spv");
	auto vert = rhi->CreateShader(vertFile, ShaderType::Vertex);
	auto frag = rhi->CreateShader(fragFile, ShaderType::Fragment);

	int texWidth, texHeight, texChannels;
	stbi_uc* pixels = stbi_load("viking_room.png", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

	auto imgBuffer = rhi->CreateBuffer(texWidth * texHeight * 4, BufferInfo{ BufferUsage::Staging });
	imgBuffer->Write(pixels, texWidth * texHeight * 4);
	stbi_image_free(pixels);

	auto texture = rhi->CreateTexture(texWidth, texHeight);

	rhi->CopyBufferToTexture(imgBuffer, texture, texWidth, texHeight);

	MaterialRef material = Material::Create(rhi, vert, frag, texture);

	MaterialRef material2 = Material::Create(rhi, vert, frag, texture);

	RenderObjectRef obj1 = RenderObject::Create("Object1", mesh, material);
	RenderObjectRef obj2 = RenderObject::Create("Object2", mesh, material2);

	glm::mat4 obj1Transform = glm::translate(glm::mat4(1.0f), glm::vec3(2, 0, 0));
	glm::mat4 obj2Transform = glm::translate(glm::mat4(1.0f), glm::vec3(-2, 0, 0));

	bool exit = false;

	bool show_demo_window = true;
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	ubo.view = glm::lookAt(glm::vec3(0.0f, 5.0f, 5.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.proj = glm::perspective(glm::radians(45.0f), 800.f / 600.f, 0.1f, 10.0f);
	ubo.proj[1][1] *= -1;

	obj1->SetTransform(obj1Transform);
	obj2->SetTransform(obj2Transform);

	Scene scene;

	scene.AddObject(obj1);
	scene.AddObject(obj2);

	auto gui = std::make_shared<ImGuiRenderer>(rhi);

	while (!exit) {
		SDL_Event e;
		SDL_PollEvent(&e);

		ImGui_ImplSDL2_ProcessEvent(&e);

		if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
			exit = true;
		}

		auto currentTime = std::chrono::high_resolution_clock::now();
		float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

		renderer.Enqueue(obj1);
		renderer.Enqueue(obj2);
		renderer.Enqueue(gui);

		ImGui_ImplSDL2_NewFrame();
		ImGui::NewFrame();

		DrawGUI(scene);

		ImGui::Render();

		renderer.Render();
	}
	rhi->WaitIdle();
	return 0;
}
