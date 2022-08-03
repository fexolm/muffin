#include "muffin/backends/vk/VulkanRHI.h"
#include <fstream>
#include <chrono>
#include <glm/matrix.hpp>
#include <glm/geometric.hpp>
#include <glm/ext.hpp>
#include "stb_image.h"

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

    const std::vector<Vertex> vertices = {
            {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
            {{0.5f,  -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
            {{0.5f,  0.5f},  {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
            {{-0.5f, 0.5f},  {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}
    };

    const std::vector<uint16_t> indices = {
            0,
            1,
            2,
            2,
            3,
            0
    };

    VulkanRHI rhi;
    auto vert = rhi.createShader(readFile("vert.spv"), ShaderType::Vertex);
    auto frag = rhi.createShader(readFile("frag.spv"), ShaderType::Fragment);

    GraphicsPipelineCreateInfo pipelineInfo;
    pipelineInfo.fragmentShader = std::make_shared<Shader>(std::move(frag));
    pipelineInfo.vertexShader = std::make_shared<Shader>(std::move(vert));
    GraphicsPipeline pipeline = rhi.createGraphicsPipeline(pipelineInfo);

    // auto renderTarget = rhi.getNextRenderTarget();

    auto vertexBuf = rhi.createBuffer(vertices.size() * sizeof(Vertex), BufferInfo{BufferUsage::Vertex});
    vertexBuf.fill((void *) vertices.data(), vertices.size() * sizeof(Vertex));

    auto indexBuf = rhi.createBuffer(indices.size() * sizeof(uint16_t), BufferInfo{BufferUsage::Index});
    indexBuf.fill((void *) indices.data(), indices.size() * sizeof(uint16_t));

    int texWidth, texHeight, texChannels;
    stbi_uc *pixels = stbi_load("texture.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

    auto imgBuffer = rhi.createBuffer(texWidth * texHeight * 4, BufferInfo{BufferUsage::Staging});
    imgBuffer.fill(pixels, texWidth * texHeight * 4);
    stbi_image_free(pixels);

    auto texture = rhi.createTexture(texWidth, texHeight);

    rhi.copyBufferToTexture(imgBuffer, texture, texWidth, texHeight);

    Sampler sampler = rhi.createSampler();

    while (true) {
        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

        auto uniformBuffer = rhi.createBuffer(sizeof(UniformBufferObject), BufferInfo{BufferUsage::Uniform});
        auto descriptorSet = rhi.createDescriptorSet(pipeline);
        rhi.updateDescriptorSet(descriptorSet, uniformBuffer, texture, sampler, sizeof(UniformBufferObject));

        ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.proj = glm::perspective(glm::radians(45.0f), 800.f / 600.f, 0.1f, 10.0f);
        ubo.proj[1][1] *= -1;

        uniformBuffer.fill((void *) &ubo, sizeof(UniformBufferObject));

        auto commandList = rhi.createCommandList();
        auto renderTarget = rhi.beginFrame();
        commandList.begin();
        commandList.beginRenderPass(renderTarget);
        commandList.bindPipeline(pipeline);
        commandList.bindVertexBuffer(vertexBuf);
        commandList.bindIndexBuffer(indexBuf);
        commandList.bindDescriptorSet(pipeline, descriptorSet);
        commandList.setViewport();
        commandList.setScissors();
        commandList.drawIndexed(indices.size(), 1, 0, 0, 0);
        commandList.endRenderPass();
        commandList.end();
        rhi.submit(commandList);
        rhi.endFrame();
    }
    return 0;
}
