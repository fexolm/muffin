#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include "../RHI.h"
#include <unordered_map>
#include <string>
#include "VulkanRHI.h"

const int FRAMES_IN_FLIGHT = 3;

class VulkanGraphicsPipeline : public RHIGraphicsPipeline {
public:
    VulkanGraphicsPipeline(VkDevice device, VkExtent2D extent, VkFormat surfaceFormat, VkFormat depthFormat,
                           const GraphicsPipelineCreateInfo &info);

    virtual ~VulkanGraphicsPipeline();

    const std::vector<VkDescriptorSetLayout> &DescriptorLayouts() const;

    VkPipeline PipelineHandle() const;

    VkPipelineLayout LayoutHandle() const;

    std::unordered_map<std::string, DescriptorSetBindingPoint> params;

private:
    VkPipelineLayout layoutHandle;
    VkPipeline pipelineHandle;
    VkDevice deviceHandle;

    std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
};

