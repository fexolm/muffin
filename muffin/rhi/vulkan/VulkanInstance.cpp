#include "VulkanInstance.h"

VkInstance createInstance(const std::vector<const char*>& enabledExtensions)
{
	VkApplicationInfo applicationInfo{};
	applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	applicationInfo.pApplicationName = "Muffin";
	applicationInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	applicationInfo.pEngineName = "Muffin";
	applicationInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	applicationInfo.apiVersion = VK_API_VERSION_1_0;
	applicationInfo.pNext = nullptr;

	// TODO: check extensions support
	const std::vector<const char*> validationLayers = {
		"VK_LAYER_KHRONOS_validation"
	};

	VkInstanceCreateInfo instanceCreateInfo{};
	instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceCreateInfo.pApplicationInfo = &applicationInfo;
	instanceCreateInfo.enabledExtensionCount = (uint32_t)enabledExtensions.size();
	instanceCreateInfo.ppEnabledExtensionNames = enabledExtensions.data();
	instanceCreateInfo.enabledLayerCount = (uint32_t)validationLayers.size();
	instanceCreateInfo.ppEnabledLayerNames = validationLayers.data();
	instanceCreateInfo.flags = 0;
	instanceCreateInfo.pNext = nullptr;

	VkInstance result;
	vkCreateInstance(&instanceCreateInfo, nullptr, &result);

	return result;
}

VulkanInstance::VulkanInstance(
	const std::vector<const char*>& enabledExtensions)
{
	instance = createInstance(enabledExtensions);
}

VulkanInstance::~VulkanInstance()
{
	vkDestroyInstance(instance, nullptr);
}

const VkInstance& VulkanInstance::Instance()
{
	return instance;
}