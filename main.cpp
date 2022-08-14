#include "muffin/rhi/RHI.h"
#include "muffin/rhi/vulkan/RHI.h"

#include <fstream>
#include <chrono>

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/geometric.hpp>
#include <glm/ext.hpp>
#include "stb_image.h"

#define SDL_MAIN_HANDLED
#include <SDL.h>

static std::vector<uint32_t> readFile(const std::string &filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("failed to open file!");
    }

    size_t fileSize = (size_t) file.tellg();
    std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));
    file.seekg(0);
    file.read((char *) buffer.data(), fileSize);
    file.close();

    return buffer;
}

struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

int main() {
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

    for (const auto &shape: shapes) {
        for (const auto &index: shape.mesh.indices) {
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
    auto vert = rhi->CreateShader(readFile("vert.spv"), ShaderType::Vertex);
    auto frag = rhi->CreateShader(readFile("frag.spv"), ShaderType::Fragment);

    GraphicsPipelineCreateInfo pipelineInfo;
    pipelineInfo.fragmentShader = frag;
    pipelineInfo.vertexShader = vert;

    RHIGraphicsPipelineRef pipeline = rhi->CreateGraphicsPipeline(pipelineInfo);

    auto posBuf = rhi->CreateBuffer(positions.size() * sizeof(glm::vec3), BufferInfo{BufferUsage::Vertex});
    posBuf->Write((void *) positions.data(), positions.size() * sizeof(glm::vec3));

    auto colorsBuf = rhi->CreateBuffer(colors.size() * sizeof(glm::vec3), BufferInfo{BufferUsage::Vertex});
    colorsBuf->Write((void *) colors.data(), colors.size() * sizeof(glm::vec3));

    auto texCoordsBuf = rhi->CreateBuffer(texCoords.size() * sizeof(glm::vec2), BufferInfo{BufferUsage::Vertex});
    texCoordsBuf->Write((void *) texCoords.data(), texCoords.size() * sizeof(glm::vec2));

    auto indexBuf = rhi->CreateBuffer(indices.size() * sizeof(uint16_t), BufferInfo{BufferUsage::Index});
    indexBuf->Write((void *) indices.data(), indices.size() * sizeof(uint16_t));

    int texWidth, texHeight, texChannels;
    stbi_uc *pixels = stbi_load("viking_room.png", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

    auto imgBuffer = rhi->CreateBuffer(texWidth * texHeight * 4, BufferInfo{BufferUsage::Staging});
    imgBuffer->Write(pixels, texWidth * texHeight * 4);
    stbi_image_free(pixels);

    auto texture = rhi->CreateTexture(texWidth, texHeight);

    rhi->CopyBufferToTexture(imgBuffer, texture, texWidth, texHeight);

    auto sampler = rhi->CreateSampler();

    while (true) {

        SDL_Event e;
        SDL_PollEvent(&e);

        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

        ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.proj = glm::perspective(glm::radians(45.0f), 800.f / 600.f, 0.1f, 10.0f);
        ubo.proj[1][1] *= -1;

        auto uniformBuffer = rhi->CreateBuffer(sizeof(UniformBufferObject), BufferInfo{BufferUsage::Uniform});
        uniformBuffer->Write((void *) &ubo, sizeof(UniformBufferObject));

        auto commandList = rhi->CreateCommandList();
        auto renderTarget = rhi->BeginFrame();
        commandList->Begin();
        commandList->BeginRenderPass(renderTarget);
        commandList->BindPipeline(pipeline);

        commandList->BindVertexBuffer(posBuf, 0);
        commandList->BindVertexBuffer(colorsBuf, 1);
        commandList->BindVertexBuffer(texCoordsBuf, 2);

        commandList->BindIndexBuffer(indexBuf);

        commandList->BindUniformBuffer("ubo", uniformBuffer, sizeof(UniformBufferObject));
        commandList->BindTexture("texSampler", texture, sampler);

        commandList->SetViewport();
        commandList->SetScissors();
        commandList->DrawIndexed(indices.size(), 1, 0, 0, 0);
        commandList->EndRenderPass();
        commandList->End();
        rhi->Submit(commandList);
        rhi->EndFrame();
    }
    return 0;
}
