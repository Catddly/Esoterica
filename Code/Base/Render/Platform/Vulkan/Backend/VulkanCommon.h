#pragma once
#if defined(EE_VULKAN)

#include "Base/Types/Arrays.h"

#define VK_SUCCEEDED(func) do { if ( ((VkResult) func) != VK_SUCCESS ) { EE_LOG_ERROR("Vulkan", EE_FILE_LINE, "Vulkan Failure."); } } while(0)

namespace EE::Render
{
	namespace Backend
	{
		TVector<char const*> GetEngineVulkanInstanceRequiredLayers( bool enableDebug );
		TVector<char const*> GetEngineVulkanInstanceRequiredExtensions();

		TVector<char const*> GetEngineVulkanDeviceRequiredLayers( bool enableDebug );
		TVector<char const*> GetEngineVulkanDeviceRequiredExtensions();
	}
}

// TODO: be configurable
#define VULKAN_USE_VMA_ALLOCATION 1

#endif