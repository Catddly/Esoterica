#if defined(EE_VULKAN)

#include "VulkanInstance.h"
#include "VulkanCommon.h"
#include "Base/Memory/Memory.h"
#include "Base/Logging/Log.h"

namespace EE::Render
{
	namespace Backend
	{
		static VkBool32 DebugUtilsUserCallback( VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
												VkDebugUtilsMessageTypeFlagsEXT             messageTypes,
												const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
												void*										pUserData )
		{
			char const* messageTypeStr = "";
			switch ( messageTypes )
			{
				case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT: messageTypeStr = "[General]"; break;
				case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT: messageTypeStr = "[Validation]"; break;
				case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT: messageTypeStr = "[Performance]"; break;
				case VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT: messageTypeStr = "[DeviceAddessBinding]"; break;
				default: messageTypeStr = "[Unknown]"; break;
			}

			switch ( messageSeverity )
			{
				case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
					EE_LOG_INFO( "Render", "Vulkan Backend", "%s %s", messageTypeStr, pCallbackData->pMessage );
				break;

				case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
					EE_LOG_INFO( "Render", "Vulkan Backend", "%s %s", messageTypeStr, pCallbackData->pMessage );
				break;

				case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
					EE_LOG_WARNING( "Render", "Vulkan Backend", "%s %s", messageTypeStr, pCallbackData->pMessage );
				break;

				case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
					EE_LOG_ERROR( "Render", "Vulkan Backend", "%s %s", messageTypeStr, pCallbackData->pMessage );
				break;

				default:
					EE_UNREACHABLE_CODE();
				break;
			}

			return false;
		}

		static VkDebugUtilsMessengerCreateInfoEXT PopulateDebugMessageerCreateInfo()
		{
			VkDebugUtilsMessengerCreateInfoEXT info = {};
			info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
			info.pNext = nullptr;
			info.flags = VkFlags( 0 );
			info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
			info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT;
			info.pfnUserCallback = DebugUtilsUserCallback;
			info.pUserData = nullptr;
			return info;
		}

		//-------------------------------------------------------------------------

		VulkanInstance::InitConfig VulkanInstance::InitConfig::GetDefault()
		{
			InitConfig config;
			#ifdef EE_DEBUG
			config.m_enableDebug = true;
			#else
			config.m_enableDebug = false;
			#endif
			config.m_requiredLayers = GetEngineVulkanInstanceRequiredLayers( config.m_enableDebug );
			config.m_requiredExtensions = GetEngineVulkanInstanceRequiredExtensions();
			return config;
		}

		//-------------------------------------------------------------------------

		VulkanInstance::VulkanInstance()
			: VulkanInstance( InitConfig::GetDefault() )
		{}

		VulkanInstance::VulkanInstance( InitConfig config )
			: m_enableDebug( config.m_enableDebug )
		{
			// TODO: use volk
			EE_ASSERT( CheckAndCollectInstanceLayerProps(config) );
			EE_ASSERT( CheckAndCollectInstanceExtensionProps(config) );

			EE_ASSERT( CreateInstance( config ) );
			EE_ASSERT( CreateDebugMessageer() );
		}

		VulkanInstance::~VulkanInstance()
		{
			if ( m_enableDebug && m_pDebugUtilsMessager )
			{
				auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) GetProcAddress( "vkDestroyDebugUtilsMessengerEXT" );
				EE_ASSERT( func != nullptr );

				func( m_pHandle, m_pDebugUtilsMessager, nullptr );
				m_pDebugUtilsMessager = nullptr;
			}

			EE_ASSERT( m_pHandle != nullptr );

			vkDestroyInstance( m_pHandle, nullptr );
			m_pHandle = nullptr;
		}

		//-------------------------------------------------------------------------

		void* VulkanInstance::GetProcAddress( char const* pFuncName ) const
		{
			void* pAddr = vkGetInstanceProcAddr( m_pHandle, pFuncName );
			if ( pAddr != nullptr )
			{
				return pAddr;
			}
			else
			{
				EE_LOG_ERROR("Render", "Vulkan Backend", "Failed to load vulkan func: %s", pFuncName);
				return nullptr;
			}
		}

		TVector<VulkanPhysicalDevice> VulkanInstance::EnumeratePhysicalDevice() const
		{
			uint32_t pdCount = 0;
			VK_SUCCEEDED( vkEnumeratePhysicalDevices( m_pHandle, &pdCount, nullptr ) );

			if ( pdCount == 0 )
			{
				EE_LOG_ERROR( "Render", "Vulkan Backend", "No suitable physical device found to render!" );
				return TVector<VulkanPhysicalDevice> {};
			}

			auto pdDevices = TVector<VkPhysicalDevice>( pdCount );
			VK_SUCCEEDED( vkEnumeratePhysicalDevices( m_pHandle, &pdCount, pdDevices.data() ) );

			TVector<VulkanPhysicalDevice> out;

			for ( uint32_t i = 0; i < pdCount; ++i )
			{
				auto const& pdDevice = pdDevices[i];

				VkPhysicalDeviceFeatures pdFeatures = {};
				vkGetPhysicalDeviceFeatures( pdDevice, &pdFeatures );

				VkPhysicalDeviceProperties pdProps = {};
				vkGetPhysicalDeviceProperties( pdDevice, &pdProps );

				VkPhysicalDeviceMemoryProperties pdMemoryProps = {};
				vkGetPhysicalDeviceMemoryProperties( pdDevice, &pdMemoryProps );

				uint32_t queueCount = 0;
				vkGetPhysicalDeviceQueueFamilyProperties( pdDevice, &queueCount, nullptr );
				TVector<VkQueueFamilyProperties> queueProps( queueCount );
				vkGetPhysicalDeviceQueueFamilyProperties( pdDevice, &queueCount, queueProps.data() );

				auto& phyDevice = out.emplace_back( pdDevice );
				phyDevice.m_features = pdFeatures;
				phyDevice.m_props = pdProps;
				phyDevice.m_memoryProps = pdMemoryProps;

				for ( uint32_t qIndex = 0; qIndex < queueCount; ++qIndex )
				{
					phyDevice.m_queueFamilies.emplace_back( qIndex, queueProps[qIndex] );
				}
			}

			return out;
		}

		//-------------------------------------------------------------------------

		bool VulkanInstance::CheckAndCollectInstanceLayerProps( InitConfig const& config )
		{
			uint32_t layerCount = 0;
			VK_SUCCEEDED( vkEnumerateInstanceLayerProperties( &layerCount, nullptr ) );

			EE_ASSERT( layerCount > 0 );

			TVector<VkLayerProperties> layerProps( layerCount );
			VK_SUCCEEDED( vkEnumerateInstanceLayerProperties( &layerCount, layerProps.data() ) );
			m_collectInfos.m_instanceLayerProps = layerProps;
			
			for ( auto const& required : config.m_requiredLayers )
			{
				bool foundLayer = false;

				for ( auto const& layer : layerProps )
				{
					if ( strcmp( required, layer.layerName ) == 0 )
					{
						foundLayer = true;
						break;
					}
				}

				if ( !foundLayer )
				{
					EE_LOG_ERROR( "Render", "Vulkan Backend", "Instance layer not found: %s", required );
					return false;
				}
			}

			return true;
		}

		bool VulkanInstance::CheckAndCollectInstanceExtensionProps( InitConfig const& config )
		{
			uint32_t extCount = 0;
			VK_SUCCEEDED( vkEnumerateInstanceExtensionProperties( nullptr, &extCount, nullptr ) );

			EE_ASSERT( extCount > 0 );

			TVector<VkExtensionProperties> extProps( extCount );
			VK_SUCCEEDED( vkEnumerateInstanceExtensionProperties( nullptr, &extCount, extProps.data() ) );
			m_collectInfos.m_instanceExtensionProps = extProps;

			for ( auto const& required : config.m_requiredExtensions )
			{
				bool foundExt = false;

				for ( auto const& ext : extProps )
				{
					if ( strcmp( required, ext.extensionName ) == 0 )
					{
						foundExt = true;
						break;
					}
				}

				if ( !foundExt )
				{
					EE_LOG_ERROR( "Render", "Vulkan Backend", "Instance extension not found: %s", required );
					return false;
				}
			}

			return true;
		}

		bool VulkanInstance::CreateInstance( InitConfig const& config )
		{
			VkApplicationInfo appInfo = {};
			appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
			appInfo.pNext = nullptr;
			appInfo.pEngineName = "Esoterica Engine";
			appInfo.pApplicationName = "Unknown";
			appInfo.apiVersion = VK_API_VERSION_1_3;
			appInfo.applicationVersion = VK_MAKE_API_VERSION( 0, 0, 1, 0 );
			appInfo.engineVersion = VK_MAKE_API_VERSION( 0, 0, 1, 0 );

			VkInstanceCreateInfo instanceCI = {};
			instanceCI.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
			instanceCI.pNext = nullptr;
			instanceCI.flags = VkFlags( 0 );
			instanceCI.pApplicationInfo = &appInfo;

			auto debugMessagerCreateInfo = PopulateDebugMessageerCreateInfo();

			if ( config.m_enableDebug )
			{
				instanceCI.pNext = (void*) &debugMessagerCreateInfo;
			}

			instanceCI.enabledLayerCount = static_cast<uint32_t>( config.m_requiredLayers.size() );
			instanceCI.ppEnabledLayerNames = config.m_requiredLayers.data();
			instanceCI.enabledExtensionCount = static_cast<uint32_t>( config.m_requiredExtensions.size() );;
			instanceCI.ppEnabledExtensionNames = config.m_requiredExtensions.data();

			VK_SUCCEEDED( vkCreateInstance( &instanceCI, nullptr, &m_pHandle ) );
			return true;
		}

		bool VulkanInstance::CreateDebugMessageer()
		{
			if ( m_enableDebug )
			{
				auto debugMessagerCI = PopulateDebugMessageerCreateInfo();

				auto func = (PFN_vkCreateDebugUtilsMessengerEXT)GetProcAddress( "vkCreateDebugUtilsMessengerEXT" );
				EE_ASSERT( func != nullptr );

				VK_SUCCEEDED( func( m_pHandle, &debugMessagerCI, nullptr, &m_pDebugUtilsMessager ) );
			}
			return true;
		}
	}
}

#endif // EE_VULKAN