#include "VulkanGraphicsPipeline.h"
#include "VulkanRenderPass.h"

#include <map>

VkDescriptorSetLayout CreateDescriptorSetLayout(
	VkDevice device,
	const std::vector<VkDescriptorSetLayoutBinding>& bindings)
{
	VkDescriptorSetLayoutCreateInfo createInfo;
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	createInfo.bindingCount = bindings.size();
	createInfo.pBindings = bindings.data();
	createInfo.flags = 0;
	createInfo.pNext = nullptr;

	VkDescriptorSetLayout result;

	VULKAN_RHI_SAFE_CALL(
		vkCreateDescriptorSetLayout(device, &createInfo, nullptr, &result));

	return result;
}

VkPipelineLayout CreatePipelineLayout(
	const VkDevice& device,
	const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts)
{
	VkPipelineLayoutCreateInfo pipelineLayoutInfo;
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = descriptorSetLayouts.size();
	pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
	pipelineLayoutInfo.pushConstantRangeCount = 0;
	pipelineLayoutInfo.pPushConstantRanges = nullptr;
	pipelineLayoutInfo.flags = 0;
	pipelineLayoutInfo.pNext = nullptr;

	VkPipelineLayout result;

	VULKAN_RHI_SAFE_CALL(
		vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &result));
	return result;
}

// TODO: remove this
VulkanRenderPassRef CreateDummyRenderPass(VulkanDeviceRef device,
	VkFormat surfaceFormat,
	VkFormat depthFormat)
{

	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = surfaceFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	colorAttachment.flags = 0;

	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription depthAttachment{};
	depthAttachment.format = depthFormat;
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout =
		VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	depthAttachment.flags = 0;

	VkAttachmentReference depthAttachmentRef{};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;
	subpass.inputAttachmentCount = 0;
	subpass.pInputAttachments = nullptr;
	subpass.preserveAttachmentCount = 0;
	subpass.pPreserveAttachments = nullptr;

	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.srcAccessMask = VK_ACCESS_NONE;

	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	std::array<VkAttachmentDescription, 2> attachments = { colorAttachment,
		depthAttachment };
	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = attachments.size();
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;
	renderPassInfo.flags = 0;
	renderPassInfo.pNext = nullptr;

	VkRenderPass renderPass;
	VULKAN_RHI_SAFE_CALL(vkCreateRenderPass(device->Device(), &renderPassInfo,
		nullptr, &renderPass));

	return VulkanRenderPassRef(new VulkanRenderPass(device, renderPass));
}

VulkanGraphicsPipeline::VulkanGraphicsPipeline(
	VulkanDeviceRef device, VkExtent2D extent, VkFormat surfaceFormat,
	VkFormat depthFormat, const GraphicsPipelineCreateInfo& info)
	: device(device)
{
	VulkanShader* vertexShader =
		static_cast<VulkanShader*>(info.vertexShader.get());
	VulkanShader* fragmentShader =
		static_cast<VulkanShader*>(info.fragmentShader.get());

	shaders.push_back(info.vertexShader);
	shaders.push_back(info.fragmentShader);

	VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
	vertShaderStageInfo.sType =
		VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertexShader->module;
	vertShaderStageInfo.pName = "main";
	vertShaderStageInfo.flags = 0;
	vertShaderStageInfo.pSpecializationInfo = nullptr;
	vertShaderStageInfo.pNext = nullptr;

	VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
	fragShaderStageInfo.sType =
		VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragmentShader->module;
	fragShaderStageInfo.pName = "main";
	vertShaderStageInfo.flags = 0;
	fragShaderStageInfo.pSpecializationInfo = nullptr;
	fragShaderStageInfo.pNext = nullptr;

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo,
		fragShaderStageInfo };

	VkPipelineVertexInputStateCreateInfo vertexInput{};
	vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInput.vertexBindingDescriptionCount =
		vertexShader->vertexBindings.size();
	vertexInput.pVertexBindingDescriptions = vertexShader->vertexBindings.data();
	vertexInput.vertexAttributeDescriptionCount =
		vertexShader->vertexAttributes.size();
	vertexInput.pVertexAttributeDescriptions =
		vertexShader->vertexAttributes.data();
	vertexInput.flags = 0;
	vertexInput.pNext = nullptr;

	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType =
		VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = false;
	inputAssembly.flags = 0;
	inputAssembly.pNext = nullptr;

	VkViewport viewport;
	viewport.x = 0.f;
	viewport.y = 0.f;
	viewport.width = extent.width;
	viewport.height = extent.height;
	viewport.minDepth = 0.f;
	viewport.maxDepth = 1.f;

	VkRect2D scissor;
	scissor.offset = { 0, 0 };
	scissor.extent = extent;

	std::vector<VkDynamicState> dynamicStates = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
	};
	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = (uint32_t)dynamicStates.size();
	dynamicState.pDynamicStates = dynamicStates.data();
	dynamicState.flags = 0;
	dynamicState.pNext = nullptr;

	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.pScissors = &scissor;
	viewportState.flags = 0;
	viewportState.pNext = nullptr;

	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = false;
	rasterizer.rasterizerDiscardEnable = false;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = false;
	rasterizer.depthBiasConstantFactor = 0.f;
	rasterizer.depthBiasClamp = 0.f;
	rasterizer.depthBiasSlopeFactor = 0.f;
	rasterizer.flags = 0;
	rasterizer.pNext = nullptr;

	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType =
		VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = false;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.f;
	multisampling.pSampleMask = nullptr;
	multisampling.alphaToCoverageEnable = false;
	multisampling.alphaToOneEnable = false;
	multisampling.flags = 0;
	multisampling.pNext = nullptr;

	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask =
		VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = false;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType =
		VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = false;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f;
	colorBlending.blendConstants[1] = 0.0f;
	colorBlending.blendConstants[2] = 0.0f;
	colorBlending.blendConstants[3] = 0.0f;
	colorBlending.flags = 0;
	colorBlending.pNext = nullptr;

	VkPipelineDepthStencilStateCreateInfo depthStencil{};
	depthStencil.sType =
		VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = true;
	depthStencil.depthWriteEnable = true;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencil.depthBoundsTestEnable = false;
	depthStencil.minDepthBounds = 0.f;
	depthStencil.maxDepthBounds = 1.f;
	depthStencil.stencilTestEnable = false;
	depthStencil.front = VkStencilOpState{};
	depthStencil.back = VkStencilOpState{};
	depthStencil.flags = 0;
	depthStencil.pNext = nullptr;

	std::map<int, std::vector<VkDescriptorSetLayoutBinding>> bindings;

	for (auto& [set, b] : vertexShader->bindings) {
		bindings[set].insert(bindings[set].end(), b.begin(), b.end());
	}
	for (auto& [set, b] : fragmentShader->bindings) {
		bindings[set].insert(bindings[set].end(), b.begin(), b.end());
	}

	params.merge(vertexShader->params);
	params.merge(fragmentShader->params);

	for (auto& [set, b] : bindings) {
		descriptorSetLayouts.push_back(
			CreateDescriptorSetLayout(device->Device(), b));
	}

	layoutHandle = CreatePipelineLayout(device->Device(), descriptorSetLayouts);

	VulkanRenderPassRef renderPass =
		CreateDummyRenderPass(device, surfaceFormat, depthFormat);

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInput;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = &dynamicState;
	pipelineInfo.layout = layoutHandle;
	pipelineInfo.renderPass = renderPass->RenderPass();
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = nullptr;
	pipelineInfo.basePipelineIndex = -1;
	pipelineInfo.flags = 0;
	pipelineInfo.pNext = nullptr;

	VULKAN_RHI_SAFE_CALL(vkCreateGraphicsPipelines(
		device->Device(), nullptr, 1, &pipelineInfo, nullptr, &pipelineHandle));
}

VulkanGraphicsPipeline::~VulkanGraphicsPipeline()
{
	vkDestroyPipeline(device->Device(), pipelineHandle, nullptr);

	vkDestroyPipelineLayout(device->Device(), layoutHandle, nullptr);

	for (VkDescriptorSetLayout layout : descriptorSetLayouts) {
		vkDestroyDescriptorSetLayout(device->Device(), layout, nullptr);
	}
}

const std::vector<VkDescriptorSetLayout>&
VulkanGraphicsPipeline::DescriptorLayouts() const
{
	return descriptorSetLayouts;
}

VkPipeline VulkanGraphicsPipeline::PipelineHandle() const
{
	return pipelineHandle;
}

VkPipelineLayout VulkanGraphicsPipeline::LayoutHandle() const
{
	return layoutHandle;
}
