#if defined(EE_VULKAN)
#include "VulkanPhysicalDevice.h"
#include "VulkanCommon.h"
#include "VulkanInstance.h"
#include "VulkanSurface.h"
#include "Base/Logging/Log.h"

#include <limits>

namespace EE::Render
{
	namespace Backend
	{
        int32_t QueueFamily::AllocateQueueFor( RHI::CommandQueueType queueType )
        {
            if ( (uint32_t) m_allocatedQueueCount >= m_props.queueCount )
            {
                // exhausted
                return -1;
            }

            if ( queueType == RHI::CommandQueueType::Graphic && IsGraphicQueue() )
            {
                return m_allocatedQueueCount++;
            }

            if ( queueType == RHI::CommandQueueType::Compute && IsComputeQueue() )
            {
                return m_allocatedQueueCount++;
            }

            if ( queueType == RHI::CommandQueueType::Transfer && IsTransferQueue() )
            {
                return m_allocatedQueueCount++;
            }
           
            return -1;
        }

        //-------------------------------------------------------------------------

		VulkanPhysicalDevice::VulkanPhysicalDevice( VkPhysicalDevice pHandle )
			: m_pHandle( pHandle )
		{}

		void VulkanPhysicalDevice::CalculatePickScore( TSharedPtr<VulkanSurface> const& pSurface )
		{
			bool haveValidPresentQueue = false;

			for ( auto const& qf : m_queueFamilies )
			{
				VkBool32 supported = false;
				EE_ASSERT( pSurface->m_loadFuncs.m_pGetPhysicalDeviceSupportFunc != nullptr );

				VK_SUCCEEDED( pSurface->m_loadFuncs.m_pGetPhysicalDeviceSupportFunc( m_pHandle, qf.m_index, pSurface->m_pHandle, &supported ) );

				// is this physical device support present?
				if ( qf.m_props.queueCount > 0 &&
				     qf.IsGraphicQueue() &&
					 supported )
				{
					haveValidPresentQueue = true;
					break;
				}
			}

			if ( haveValidPresentQueue )
			{
				if ( ( m_props.deviceType & VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU ) != 0 )
				{
					m_pickScore = 10;
				}
				else if ( ( m_props.deviceType & VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ) != 0 )
				{
					m_pickScore = 100;
				}
				else if ( ( m_props.deviceType & VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU ) != 0 )
				{
					m_pickScore = 1;
				}
				else
				{
					EE_LOG_ERROR("Render", "Vulkan Backend", "Invalid physical device type: %d", m_props.deviceType );
					m_pickScore = -1;
				}
			}
			else
			{
				m_pickScore = -1;
			}
		}

		//-------------------------------------------------------------------------

		VulkanPhysicalDevice PickMostSuitablePhysicalDevice( TVector<VulkanPhysicalDevice>& pdDevices )
		{
			bool allPhysicalDeviceInvalid = true;

			int currPickScore = std::numeric_limits<int>::min();
			size_t currPickIndex = 0;

			for ( size_t i = 0; i < pdDevices.size(); ++i )
			{
				auto const& pd = pdDevices[i];

				if ( pd.m_pickScore > 0 )
				{
					allPhysicalDeviceInvalid = false;

					if ( pd.m_pickScore > currPickScore )
					{
						currPickIndex = i;
						currPickScore = pd.m_pickScore;
					}
				}
			}

			if ( allPhysicalDeviceInvalid )
			{
				EE_LOG_ERROR( "Render", "Vulkan Backend", "All physical devices have pick score less than zero, did you forget to call VulkanPhysicalDevice::CalculatePickScore()?" );
				EE_ASSERT( false );
			}

			EE_LOG_MESSAGE("Render", "Vulkan Backend", "Pick physical device info: \n\tname: %s\n\tdriver version: %u\n\tvendor id: %u", pdDevices[currPickIndex].m_props.deviceName, pdDevices[currPickIndex].m_props.driverVersion, pdDevices[currPickIndex].m_props.vendorID );

			VulkanPhysicalDevice pd = std::move( pdDevices[currPickIndex] );
			pdDevices.erase_unsorted( pdDevices.begin() + currPickIndex );

			return pd;
		}
    }
}

#endif