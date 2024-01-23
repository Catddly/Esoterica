#if defined(EE_VULKAN)
#include "VulkanTexture.h"
#include "VulkanCommon.h"
#include "VulkanDevice.h"
#include "VulkanBuffer.h"
#include "RHIToVulkanSpecification.h"
#include "Base/RHI/RHICommandBuffer.h"
#include "Base/RHI/RHIHelper.h"
#include "Base/Math/Math.h"
#include "Base/Types/Arrays.h"
#include "Base/RHI/RHIDowncastHelper.h"

namespace EE::Render
{
    namespace Backend
    {
        void* VulkanTexture::MapSlice( RHI::RHIDevice* pDevice, uint32_t layer )
        {
            EE_ASSERT( layer < m_desc.m_array );
            EE_ASSERT( pDevice );

            auto iterator = m_waitToFlushMappedMemory.find( layer );
            if ( iterator != m_waitToFlushMappedMemory.end() )
            {
                return iterator->second->Map( pDevice );
            }

            RHI::RHIBufferCreateDesc bufferDesc = RHI::RHIBufferCreateDesc::NewSize( GetLayerByteSize( layer ) );
            bufferDesc.m_memoryUsage = RHI::ERenderResourceMemoryUsage::CPUToGPU;
            bufferDesc.m_usage = RHI::EBufferUsage::TransferSrc;
            RHI::RHIBuffer* pStagingBuffer = pDevice->CreateBuffer( bufferDesc );
            EE_ASSERT( pStagingBuffer );

            m_waitToFlushMappedMemory.insert_or_assign( layer, pStagingBuffer );

            return pStagingBuffer->Map( pDevice );
        }

        void VulkanTexture::UnmapSlice( RHI::RHIDevice* pDevice, uint32_t layer )
        {
            EE_ASSERT( layer < m_desc.m_array );
            EE_ASSERT( pDevice );

            auto iterator = m_waitToFlushMappedMemory.find( layer );
            if ( iterator != m_waitToFlushMappedMemory.end() )
            {
                iterator->second->Unmap( pDevice );
                return;
            }

            EE_UNREACHABLE_CODE();
        }

        bool VulkanTexture::UploadMappedData( RHI::RHIDevice* pDevice, RHI::RenderResourceBarrierState dstBarrierState )
        {
            if ( m_waitToFlushMappedMemory.empty() )
            {
                return true;
            }

            TInlineVector<RHI::TextureSubresourceRangeUploadRef, 6> uploadRefs;
            for ( auto const& stagingBuffer : m_waitToFlushMappedMemory )
            {
                auto& uploadRef = uploadRefs.emplace_back();
                uploadRef.m_pStagingBuffer = stagingBuffer.second;
                uploadRef.m_aspectFlags.ClearAllFlags();
                // TODO: proper aspect flags
                uploadRef.m_aspectFlags.SetFlag( RHI::TextureAspectFlags::Color );
                uploadRef.m_layer = stagingBuffer.first;
                uploadRef.m_baseMipLevel = 0;
                uploadRef.m_bufferBaseOffset = 0;
                uploadRef.m_uploadAllMips = true;
            }

            DispatchImmediateCommandAndWait( pDevice, [this, &uploadRefs, dstBarrierState] ( RHI::RHICommandBuffer* pCommandBuffer ) -> bool
            {
                pCommandBuffer->CopyBufferToTexture( this, dstBarrierState, uploadRefs );

                return true;
            } );

            for ( auto stagingBuffer : m_waitToFlushMappedMemory )
            {
                pDevice->DestroyBuffer( stagingBuffer.second );
            }

            // Do NOT use clear() here, we want to free all memory immediately
            {
                TMap<uint32_t, RHI::RHIBuffer*> emptyMap;
                m_waitToFlushMappedMemory.swap( emptyMap );
            }

            return true;
        }

        //-------------------------------------------------------------------------

        RHI::RHITextureView VulkanTexture::CreateView( RHI::RHIDevice* pDevice, RHI::RHITextureViewCreateDesc const& desc ) const
        {
            auto* pVkDevice = RHI::RHIDowncast<VulkanDevice>( pDevice );
            if ( !pVkDevice )
            {
                return {};
            }

            uint32_t layerCount = 1;
            if ( m_desc.m_type == RHI::ETextureType::TCubemap || m_desc.m_type == RHI::ETextureType::TCubemapArray )
            {
                layerCount = 6;
            }

            VkImageViewCreateInfo viewCI = {};
            viewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewCI.components.r = VK_COMPONENT_SWIZZLE_R;
            viewCI.components.g = VK_COMPONENT_SWIZZLE_G;
            viewCI.components.b = VK_COMPONENT_SWIZZLE_B;
            viewCI.components.a = VK_COMPONENT_SWIZZLE_A;
            viewCI.image = m_pHandle;
            viewCI.format = desc.m_format.has_value() ? ToVulkanFormat( *desc.m_format ) : ToVulkanFormat( m_desc.m_format );
            viewCI.viewType = desc.m_viewType.has_value() ? ToVulkanImageViewType( *desc.m_viewType ) : ToVulkanImageViewType( m_desc.m_type );
            viewCI.subresourceRange.baseMipLevel = desc.m_baseMipmap;
            viewCI.subresourceRange.baseArrayLayer = 0;
            viewCI.subresourceRange.aspectMask = ToVulkanImageAspectFlags( desc.m_viewAspect );
            viewCI.subresourceRange.levelCount = desc.m_levelCount.has_value() ? *desc.m_levelCount : m_desc.m_mipmap - desc.m_baseMipmap;
            viewCI.subresourceRange.layerCount = layerCount;

            RHI::RHITextureView textureView;

            VkImageView vkView;
            VK_SUCCEEDED( vkCreateImageView( pVkDevice->m_pHandle, &viewCI, nullptr, &vkView ) );

            textureView.m_pViewHandle = reinterpret_cast<void*>( vkView );
            textureView.m_desc = desc;
            return textureView;
        }

        void VulkanTexture::DestroyView( RHI::RHIDevice* pDevice, RHI::RHITextureView& textureView )
        {
            auto* pVkDevice = RHI::RHIDowncast<VulkanDevice>( pDevice );
            if ( !pVkDevice )
            {
                EE_ASSERT( false );
                return;
            }

            if ( !textureView.IsValid() )
            {
                EE_ASSERT( false );
                return;
            }

            vkDestroyImageView( pVkDevice->m_pHandle, reinterpret_cast<VkImageView>( textureView.m_pViewHandle ), nullptr );

            textureView.m_desc = {};
            textureView.m_pViewHandle = nullptr;
        }

        //-------------------------------------------------------------------------

		void VulkanTexture::ForceDiscardAllUploadedData( RHI::RHIDevice* pDevice )
		{
            if ( !m_waitToFlushMappedMemory.empty() )
            {
                for ( auto& [layer, pStagingBuffer] : m_waitToFlushMappedMemory )
                {
                    pDevice->DestroyBuffer( pStagingBuffer );
                }

                TMap<uint32_t, RHI::RHIBuffer*> emptyMap;
                m_waitToFlushMappedMemory.swap( emptyMap );
            }
		}

        uint32_t VulkanTexture::GetLayerByteSize( uint32_t layer )
        {
            EE_ASSERT( layer < m_desc.m_array );

            uint32_t totalByteSize = 0;
            for ( uint32_t mip = 0; mip < m_desc.m_mipmap; ++mip )
            {
                uint32_t const width = Math::Max( m_desc.m_width >> mip, 1u );
                uint32_t const height = Math::Max( m_desc.m_height >> mip, 1u );
                uint32_t const depth = Math::Max( m_desc.m_depth >> mip, 1u );

                // Not support 3D texture for now
                EE_ASSERT( depth == 1 );

                uint32_t numBytes = 0;
                uint32_t numBytesPerRow = 0;
                GetPixelFormatByteSize( width, height, m_desc.m_format, numBytes, numBytesPerRow );

                if ( numBytes > 0 )
                {
                    totalByteSize += numBytes;
                }
                else
                {
                    EE_LOG_FATAL_ERROR( "Render", "Vulkan Backend", "Failed to calculate pixel format byte size!" );
                }
            }

            return totalByteSize;
        }
	}
}

#endif