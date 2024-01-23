#if defined(EE_VULKAN)
#include "VulkanCommon.h"

#include <vulkan/vulkan_core.h>

namespace EE::Render
{
	namespace Backend
	{
		constexpr static char const* const gEngineRequiredInstanceLayers[] = {
			"VK_LAYER_KHRONOS_validation",
		};

		#ifdef _WIN32
		#include <windows.h>
		#include <vulkan/vulkan_win32.h>
		#ifdef EE_DEBUG
		constexpr static char const* const gEngineRequiredInstanceExtensions[] = {
			VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
			VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
			VK_KHR_SURFACE_EXTENSION_NAME,
			VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
		};
		#else
		constexpr static char const* const gEngineRequiredInstanceExtensions[] = {
			VK_KHR_SURFACE_EXTENSION_NAME,
			VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
		};
		#endif // EE_DEBUG
		#else // _WIN32
		#error Unsupported vulkan platform!
		#endif

		constexpr static char const* const gEngineRequiredDeviceLayers[] = {
			"VK_LAYER_KHRONOS_validation",
		};

		constexpr static char const* const gEngineRequiredDeviceExtensions[] = {
			// common
			//-------------------------------------------------------------------------
			VK_KHR_MAINTENANCE1_EXTENSION_NAME,
			VK_KHR_MAINTENANCE2_EXTENSION_NAME,
			VK_KHR_MAINTENANCE3_EXTENSION_NAME,
            VK_KHR_MAINTENANCE_4_EXTENSION_NAME, // allow descriptor set has NOT consumed slot
			VK_KHR_SWAPCHAIN_EXTENSION_NAME, // swapchain

			VK_KHR_SAMPLER_MIRROR_CLAMP_TO_EDGE_EXTENSION_NAME,

			VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME, // buffer address

			// ray tracing
			//-------------------------------------------------------------------------
			//VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
			//VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
			//VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
			//VK_KHR_RAY_QUERY_EXTENSION_NAME,
		};

		TVector<char const*> GetEngineVulkanInstanceRequiredLayers( bool enableDebug )
		{
			if ( enableDebug )
			{
				TVector<char const*> validationLayers = {};
				size_t const layerCount = sizeof( gEngineRequiredInstanceLayers ) / sizeof( gEngineRequiredInstanceLayers[0] );

				for ( int i = 0; i < layerCount; ++i )
				{
					validationLayers.push_back( gEngineRequiredInstanceLayers[i] );
				}

				return validationLayers;
			}
			else
			{
				return TVector<char const*> {};
			}
		}

		TVector<char const*> GetEngineVulkanInstanceRequiredExtensions()
		{
			TVector<char const*> validationExtensions = {};
			size_t const extCount = sizeof( gEngineRequiredInstanceExtensions ) / sizeof( gEngineRequiredInstanceExtensions[0] );

			for ( int i = 0; i < extCount; ++i )
			{
				validationExtensions.push_back( gEngineRequiredInstanceExtensions[i] );
			}

			return validationExtensions;
		}

		//-------------------------------------------------------------------------

		TVector<char const*> GetEngineVulkanDeviceRequiredLayers( bool enableDebug )
		{
			if ( enableDebug )
			{
				TVector<char const*> validationLayers = {};
				size_t const layerCount = sizeof( gEngineRequiredDeviceLayers ) / sizeof( gEngineRequiredDeviceLayers[0] );

				for ( int i = 0; i < layerCount; ++i )
				{
					validationLayers.push_back( gEngineRequiredDeviceLayers[i] );
				}

				return validationLayers;
			}
			else
			{
				return TVector<char const*> {};
			}
		}

		TVector<char const*> GetEngineVulkanDeviceRequiredExtensions()
		{
			TVector<char const*> validationExtensions = {};
			size_t const extCount = sizeof( gEngineRequiredDeviceExtensions ) / sizeof( gEngineRequiredDeviceExtensions[0] );

			for ( int i = 0; i < extCount; ++i )
			{
				validationExtensions.push_back( gEngineRequiredDeviceExtensions[i] );
			}

			return validationExtensions;
		}
	}
}

#endif // EE_VULKAN