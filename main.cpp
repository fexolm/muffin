#include "muffin/backends/vk/VulkanRHI.h"
#include <fstream>

static std::vector<char> readFile(const std::string &filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("failed to open file!");
    }

    size_t fileSize = (size_t) file.tellg();
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();

    return buffer;
}

int main() {
    VulkanRHI rhi;
    auto vert = rhi.createShader(readFile("vert.spv"));
    auto frag = rhi.createShader(readFile("frag.spv"));

    GraphicsPipelineCreateInfo pipelineInfo;
    pipelineInfo.fragmentShader = std::make_shared<Shader>(std::move(frag));
    pipelineInfo.vertexShader = std::make_shared<Shader>(std::move(vert));
    GraphicsPipeline pipeline = rhi.createGraphicsPipeline(pipelineInfo);
    rhi.drawTriangle(pipeline);

    return 0;
}
