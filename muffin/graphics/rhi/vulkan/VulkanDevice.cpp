#include "VulkanDevice.h"

VkPhysicalDevice choosePhysicalDevice(VkInstance instance)
{
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

	std::vector<VkPhysicalDevice> physicalDevices(deviceCount);

	vkEnumeratePhysicalDevices(instance, &deviceCount, physicalDevices.data());
	for (auto& device : physicalDevices) {
		VkPhysicalDeviceProperties deviceProperties;
		vkGetPhysicalDeviceProperties(device, &deviceProperties);
		if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
			return device;
		}
	}
	// TODO: throw
	return nullptr;
}

std::vector<VkQueueFamilyProperties>
getQueueFamilyProperties(VkPhysicalDevice device)
{
	uint32_t propertiesCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &propertiesCount, nullptr);

	std::vector<VkQueueFamilyProperties> families(propertiesCount);

	vkGetPhysicalDeviceQueueFamilyProperties(device, &propertiesCount,
		families.data());

	return families;
}

uint32_t findGraphicsFamilyIdx(VkPhysicalDevice device)
{
	auto families = getQueueFamilyProperties(device);
	for (uint32_t i = 0; i < families.size(); ++i) {
		if (families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			return i;
		}
	}
	return uint32_t(-1);
}

uint32_t findPresentFamilyIdx(VkPhysicalDevice device, VkSurfaceKHR surface)
{
	auto families = getQueueFamilyProperties(device);
	for (uint32_t i = 0; i < families.size(); ++i) {
		if (families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			VkBool32 supportsSurface = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface,
				&supportsSurface);
			if (supportsSurface) {
				return i;
			}
		}
	}
	return uint32_t(-1);
}

VkDevice createDevice(VkPhysicalDevice physicalDevice,
	uint32_t graphicsFamilyIdx, uint32_t presentFamilyIdx,
	const std::vector<const char*>& deviceExtensions)
{
	float queuePriority = 1.0f;

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	queueCreateInfos.emplace_back();
	queueCreateInfos.back().sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfos.back().queueFamilyIndex = graphicsFamilyIdx;
	queueCreateInfos.back().queueCount = 1;
	queueCreateInfos.back().pQueuePriorities = &queuePriority;
	queueCreateInfos.back().flags = 0;
	queueCreateInfos.back().pNext = nullptr;

	if (graphicsFamilyIdx != presentFamilyIdx) {
		queueCreateInfos.emplace_back();

		queueCreateInfos.back().sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfos.back().queueFamilyIndex = presentFamilyIdx;
		queueCreateInfos.back().queueCount = 1;
		queueCreateInfos.back().pQueuePriorities = &queuePriority;
		queueCreateInfos.back().flags = 0;
		queueCreateInfos.back().pNext = nullptr;
	}

	VkPhysicalDeviceFeatures deviceFeatures{};
	deviceFeatures.samplerAnisotropy = true;

	VkDeviceCreateInfo deviceCreateInfo{};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.queueCreateInfoCount = (uint32_t)queueCreateInfos.size();
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
	deviceCreateInfo.enabledExtensionCount = (uint32_t)deviceExtensions.size();
	deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
	deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
	deviceCreateInfo.flags = 0;
	deviceCreateInfo.pNext = nullptr;

	VkDevice device;
	vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device);

	return device;
}

VulkanDevice::VulkanDevice(VulkanInstanceRef instance,
	const std::vector<const char*>& deviceExtensions,
	VkSurfaceKHR surface)
	: instance(instance)
{
	physicalDevice = choosePhysicalDevice(instance->Instance());
	graphicsFamilyIdx = findGraphicsFamilyIdx(physicalDevice);
	presentFamilyIdx = findPresentFamilyIdx(physicalDevice, surface);
	device = createDevice(physicalDevice, graphicsFamilyIdx, presentFamilyIdx,
		deviceExtensions);

	vkGetDeviceQueue(device, graphicsFamilyIdx, 0, &graphicsQueue);
	vkGetDeviceQueue(device, presentFamilyIdx, 0, &presentQueue);
}

VulkanDevice::~VulkanDevice()
{
	vkDestroyDevice(device, nullptr);
}

const VkDevice& VulkanDevice::Device()
{
	return device;
}

const VkPhysicalDevice& VulkanDevice::PhysicalDevice()
{
	return physicalDevice;
}

uint32_t VulkanDevice::GraphicsFamily()
{
	return graphicsFamilyIdx;
}

uint32_t VulkanDevice::PresentFamily()
{
	return presentFamilyIdx;
}

const VkQueue& VulkanDevice::GraphicsQueue()
{
	return graphicsQueue;
}

const VkQueue& VulkanDevice::PresentQueue()
{
	return presentQueue;
}