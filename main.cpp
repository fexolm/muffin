#include "muffin/backends/vk/VulkanRHI.h"
#include <fstream>

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

int main() {
    const std::vector<Vertex> vertices = {
            {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
            {{0.5f,  -0.5f}, {0.0f, 1.0f, 0.0f}},
            {{0.5f,  0.5f},  {0.0f, 0.0f, 1.0f}},
            {{-0.5f, 0.5f},  {1.0f, 1.0f, 1.0f}}
    };

    const std::vector<uint16_t> indices = {
            0, 1, 2, 2, 3, 0
    };

    VulkanRHI rhi;
    auto vert = rhi.createShader(readFile("vert.spv"));
    auto frag = rhi.createShader(readFile("frag.spv"));

    GraphicsPipelineCreateInfo pipelineInfo;
    pipelineInfo.fragmentShader = std::make_shared<Shader>(std::move(frag));
    pipelineInfo.vertexShader = std::make_shared<Shader>(std::move(vert));
    GraphicsPipeline pipeline = rhi.createGraphicsPipeline(pipelineInfo);

    // auto renderTarget = rhi.getNextRenderTarget();

    auto vertexBuf = rhi.createBuffer(vertices.size() * sizeof(Vertex), BufferInfo{BufferUsage::Vertex});
    vertexBuf.fill((void *) vertices.data(), vertices.size() * sizeof(Vertex));

    auto indexBuf = rhi.createBuffer(indices.size() * sizeof(uint16_t), BufferInfo{BufferUsage::Index});
    indexBuf.fill((void *) indices.data(), indices.size() * sizeof(uint16_t));

    while (true) {
        auto commandList = rhi.createCommandList();
        auto renderTarget = rhi.beginFrame();
        commandList.begin();
        commandList.beginRenderPass(renderTarget);
        commandList.bindPipeline(pipeline);
        commandList.bindVertexBuffer(vertexBuf);
        commandList.bindIndexBuffer(indexBuf);
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
