#pragma once
#if defined(EE_VULKAN)

#include "Base/Memory/Pointers.h"

#include <vulkan/vulkan_core.h>

#ifdef _WIN32
#include <windows.h>
#include <vulkan/vulkan_win32.h>
#endif

namespace EE::Render
{
	namespace Backend
	{
		class VulkanInstance;

		class VulkanSurface
		{
		public:

			struct LoadFuncs
			{
				#ifdef _WIN32
				PFN_vkCreateWin32SurfaceKHR					m_pCreateFunc = nullptr;
				#endif

				PFN_vkDestroySurfaceKHR						m_pDestroyFunc = nullptr;
				PFN_vkGetPhysicalDeviceSurfaceSupportKHR	m_pGetPhysicalDeviceSupportFunc = nullptr;
			};

		public:

			VulkanSurface( TSharedPtr<VulkanInstance> pInstance, void* pActiveWindowHandle );

			VulkanSurface( VulkanSurface const& ) = delete;
			VulkanSurface& operator=( VulkanSurface const& ) = delete;

			VulkanSurface( VulkanSurface&& ) = default;
			VulkanSurface& operator=( VulkanSurface&& ) = default;

			~VulkanSurface();

		private:

			void LoadVulkanFuncPointer();

			#ifdef _WIN32
			bool CreateWin32Surface( HWND activeWindow );
			#else
			#error Unsupport platform surface create function!
			#endif

		private:

			friend class VulkanPhysicalDevice;
			friend class VulkanSwapchain;

			TSharedPtr<VulkanInstance>				m_pInstance = nullptr;
			LoadFuncs								m_loadFuncs;

			VkSurfaceKHR							m_pHandle = nullptr;
		};
	}
}

#endif // EE_VULKAN