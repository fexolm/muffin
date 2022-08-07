#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include "RHI.h"

class VulkanGraphicsPipeline : public RHIGraphicsPipeline {
public:
    VulkanGraphicsPipeline(VkDevice device, VkExtent2D extent, VkFormat surfaceFormat, VkFormat depthFormat,
                           const GraphicsPipelineCreateInfo &info);

    virtual ~VulkanGraphicsPipeline();

    const std::vector<VkDescriptorSetLayout> &DescriptorLayouts() const;

    VkPipeline PipelineHandle() const;

    VkPipelineLayout LayoutHandle() const;

private:
    VkPipelineLayout layoutHandle;
    VkPipeline pipelineHandle;
    VkDevice deviceHandle;

    std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
};

