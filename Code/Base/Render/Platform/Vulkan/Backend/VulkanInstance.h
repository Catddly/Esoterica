#pragma once
#if defined(EE_VULKAN)

#include "Base/Types/Arrays.h"
#include "Base/Memory/Pointers.h"
#include "VulkanPhysicalDevice.h"

#include <vulkan/vulkan_core.h>

namespace EE::Render
{
	namespace Backend
	{
		class VulkanInstance
		{
            friend class VulkanSurface;
            friend class VulkanMemoryAllocator;

		public:

			struct InitConfig
			{
				static InitConfig GetDefault();

				TVector<char const*>				m_requiredLayers;
				TVector<char const*>				m_requiredExtensions;
				bool								m_enableDebug;
			};

			struct CollectedInfo
			{
				TVector<VkLayerProperties>			m_instanceLayerProps;
				TVector<VkExtensionProperties>		m_instanceExtensionProps;
			};

		public:

			VulkanInstance();
			VulkanInstance(InitConfig config);

			VulkanInstance( VulkanInstance const& ) = delete;
			VulkanInstance& operator=( VulkanInstance const& ) = delete;

			VulkanInstance( VulkanInstance&& ) = default;
			VulkanInstance& operator=( VulkanInstance&& ) = default;

			~VulkanInstance();

		public:

			// Get process address of a function pointer.
			// If the function pointer doesn't exist, return nullptr.
			void* GetProcAddress( char const* pFuncName ) const;

			// Enumerate all usable physical devices.
			TVector<VulkanPhysicalDevice> EnumeratePhysicalDevice() const;

			inline bool IsEnableDebug() const { return m_enableDebug; }

			//-------------------------------------------------------------------------

			inline TVector<VkLayerProperties> const& GetLayerProps() const { return m_collectInfos.m_instanceLayerProps; }
			inline TVector<VkExtensionProperties> const& GetExtensionProps() const { return m_collectInfos.m_instanceExtensionProps; }

		private:

			bool CheckAndCollectInstanceLayerProps( InitConfig const& config );
			bool CheckAndCollectInstanceExtensionProps( InitConfig const& config );
			bool CreateInstance( InitConfig const& config );
			bool CreateDebugMessageer();

		private:

			bool									m_enableDebug;
			VkDebugUtilsMessengerEXT				m_pDebugUtilsMessager = nullptr;

			VkInstance								m_pHandle;
			CollectedInfo							m_collectInfos;
		};
	}
}

#endif // EE_VULKAN