#include "RHI.h"
#include "VulkanRHI.h"

RHIDriverRef CreateVulkanRhi()
{
	return RHIDriverRef(new VulkanRHI());
}