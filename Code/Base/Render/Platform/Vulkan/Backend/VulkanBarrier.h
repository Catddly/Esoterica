#pragma once
#if defined(EE_VULKAN)

#include "Base/Render/RenderResourceBarrier.h"
#include "VulkanCommandBuffer.h"

#include <vulkan/vulkan_core.h>

namespace EE::Render
{
	namespace Backend
	{
		struct VkMemoryBarrierTransition
		{
			VkPipelineStageFlags				m_srcStage;
			VkPipelineStageFlags				m_dstStage;
			VkMemoryBarrier						m_barrier;
		};

		struct VkBufferBarrierTransition
		{
			VkPipelineStageFlags				m_srcStage;
			VkPipelineStageFlags				m_dstStage;
			VkBufferMemoryBarrier				m_barrier;
		};

		struct VkTextureBarrierTransition
		{
			VkPipelineStageFlags				m_srcStage;
			VkPipelineStageFlags				m_dstStage;
			VkImageMemoryBarrier				m_barrier;
		};

		VkMemoryBarrierTransition GetMemoryBarrierTransition( GlobalBarrier const& globalBarrier );
		VkBufferBarrierTransition GetBufferBarrierTransition( BufferBarrier const& bufferBarrier );
		VkTextureBarrierTransition GetTextureBarrierTransition( ImageBarrier const& imageBarrier );

		//-------------------------------------------------------------------------

		void VulkanCmdPipelineBarrier(
			VulkanCommandBuffer* pCmdBuf,
			GlobalBarrier const* pGlobalBarriers,
			uint32_t bufferBarrierCount, BufferBarrier const* pBufferBarriers,
			uint32_t imageBarrierCount, ImageBarrier const* pImageBarriers
		);
	}
}

#endif // EE_VULKAN