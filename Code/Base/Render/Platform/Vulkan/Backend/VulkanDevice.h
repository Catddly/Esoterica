#pragma once
#if defined(EE_VULKAN)

#include "VulkanPhysicalDevice.h"
#include "VulkanMemoryAllocator.h"
#include "VulkanSampler.h"
#include "VulkanDescriptorSet.h"
#include "VulkanSynchronization.h"
#include "Base/Types/Map.h"
#include "Base/Types/String.h"
#include "Base/Types/Queue.h"
#include "Base/Types/HashMap.h"
#include "Base/Types/Event.h"
#include "Base/Types/Function.h"
#include "Base/Memory/Pointers.h"
#include "Base/Resource/ResourcePtr.h"
#include "Base/RHI/RHIDevice.h"
#include "Base/RHI/Resource/RHIPipelineState.h"
#include "Base/RHI/Resource/RHIResourceCreationCommons.h"

#include <vulkan/vulkan_core.h>

namespace EE::RHI
{
    class RHIShader;
    class RHIBuffer;
    class RHITexture;
    class RHISemaphore;

    class RHICommandBuffer;
    class RHICommandQueue;
}

//-------------------------------------------------------------------------

namespace EE::Render
{
	namespace Backend
	{
        namespace Vulkan
        {
            static VulkanDescriptorSetReleaseImpl gDescriptorSetReleaseImpl;
            static VulkanCPUGPUSyncImpl gCPUGPUSyncImpl;
        }

		class VulkanInstance;
		class VulkanSurface;
        class VulkanCommandBuffer;
        class VulkanCommandBufferPool;
        class VulkanCommandQueue;

        class VulkanRasterPipelineState;
        class VulkanComputePipelineState;

		class VulkanDevice final : public RHI::RHIDevice
		{
            friend class VulkanMemoryAllocator;
            friend class VulkanCommandBuffer;
            friend class VulkanCommandBufferPool;
            friend class VulkanCommandQueue;
            friend class VulkanSwapchain;
            friend class VulkanBuffer;
            friend class VulkanTexture;
            friend class VulkanFramebufferCache;

            friend struct VulkanDescriptorSetReleaseImpl;
            friend struct VulkanCPUGPUSyncImpl;

        public:

			struct InitConfig
			{
				static InitConfig GetDefault( bool enableDebug );

                void*                               m_activeWindowHandle = nullptr;

				TVector<char const*>				m_requiredLayers;
				TVector<char const*>				m_requiredExtensions;
			};

			struct CollectedInfo
			{
				TVector<VkLayerProperties>			m_deviceLayerProps;
				TVector<VkExtensionProperties>		m_deviceExtensionProps;
			};

        public:

            EE_RHI_STATIC_TAGGED_TYPE( RHI::ERHIType::Vulkan )

			// Only support single physical device for now.
			VulkanDevice();
			VulkanDevice( InitConfig config );

			~VulkanDevice();

        public:

            inline TEventHandle<RHI::RHITexture*> OnSwapchainImageDestroyed() { return m_onSwapchainImageDestroyedEvent; }

		public:

            virtual void BeginFrame() override;
            virtual void EndFrame() override;

            inline virtual void WaitUntilIdle() override;

            virtual RHI::RHICommandBuffer* AllocateCommandBuffer() override;
            inline virtual RHI::RHICommandQueue* GetMainGraphicCommandQueue() override;

            virtual RHI::RHICommandBuffer* GetImmediateCommandBuffer() override;

            virtual bool BeginCommandBuffer( RHI::RHICommandBuffer* pCommandBuffer ) override;
            virtual void EndCommandBuffer( RHI::RHICommandBuffer* pCommandBuffer ) override;

            virtual void SubmitCommandBuffer(
                RHI::RHICommandBuffer* pCommandBuffer,
                TSpan<RHI::RHISemaphore*> pWaitSemaphores,
                TSpan<RHI::RHISemaphore*> pSignalSemaphores,
                TSpan<Render::PipelineStage> waitStages
            ) override;

            //-------------------------------------------------------------------------

            virtual RHI::RHITexture* CreateTexture( RHI::RHITextureCreateDesc const& createDesc ) override;
            virtual void             DestroyTexture( RHI::RHITexture* pTexture ) override;

            virtual RHI::RHIBuffer* CreateBuffer( RHI::RHIBufferCreateDesc const& createDesc ) override;
            virtual void            DestroyBuffer( RHI::RHIBuffer* pBuffer ) override;

            virtual RHI::RHIShader* CreateShader( RHI::RHIShaderCreateDesc const& createDesc ) override;
            virtual void            DestroyShader( RHI::RHIShader* pShader ) override;

            virtual RHI::RHISemaphore* CreateSyncSemaphore( RHI::RHISemaphoreCreateDesc const& createDesc ) override;
            virtual void               DestroySyncSemaphore( RHI::RHISemaphore* pSemaphore ) override;

            //-------------------------------------------------------------------------

            virtual RHI::RHIRenderPass* CreateRenderPass( RHI::RHIRenderPassCreateDesc const& createDesc) override;
            virtual void                DestroyRenderPass( RHI::RHIRenderPass* pRenderPass ) override;

            //-------------------------------------------------------------------------

            virtual RHI::RHIPipelineState* CreateRasterPipelineState( RHI::RHIRasterPipelineStateCreateDesc const& createDesc, CompiledShaderArray const& compiledShaders ) override;
            virtual void                   DestroyRasterPipelineState( RHI::RHIPipelineState* pPipelineState ) override;

            virtual RHI::RHIPipelineState* CreateComputePipelineState( RHI::RHIComputePipelineStateCreateDesc const& createDesc, Render::ComputeShader const* pCompiledShader ) override;
            virtual void                   DestroyComputePipelineState( RHI::RHIPipelineState* pPipelineState ) override;

		private:

            using CombinedShaderBindingLayout = TMap<uint32_t, Render::Shader::ResourceBinding>;
            using CombinedShaderSetLayout = TFixedMap<uint32_t, CombinedShaderBindingLayout, RHI::NumMaxResourceBindingSet>;

            struct VulkanDescriptorSetLayoutInfos
            {
                TFixedVector<VkDescriptorSetLayout, RHI::NumMaxResourceBindingSet>                  m_vkDescriptorSetLayouts;
                TFixedVector<TMap<uint32_t, VkDescriptorType>, RHI::NumMaxResourceBindingSet>       m_SetLayoutsVkDescriptorTypes;
            };

            constexpr static uint32_t BindlessDescriptorSetDesiredSampledImageCount = 1024;
            constexpr static uint32_t DescriptorSetReservedSampledImageCount = 32;

            void PickPhysicalDeviceAndCreate( InitConfig const& config );
			bool CheckAndCollectDeviceLayers( InitConfig const& config );
			bool CheckAndCollectDeviceExtensions( InitConfig const& config );
			bool CreateDevice( InitConfig const& config );

            // Resource
            //-------------------------------------------------------------------------

            void ImmediateUploadBufferData( VulkanBuffer* pBuffer, RHI::RHIBufferUploadData const& uploadData );
            void ImmediateUploadTextureData( VulkanTexture* pTexture, RHI::RHITextureBufferData const& uploadData );

            // Pipeline State
            //-------------------------------------------------------------------------

            bool CreateRasterPipelineStateLayout( RHI::RHIRasterPipelineStateCreateDesc const& createDesc, CompiledShaderArray const& compiledShaders, VulkanRasterPipelineState* pPipelineState );
            // TODO: extract common destroy pattern
            void DestroyRasterPipelineStateLayout( VulkanRasterPipelineState* pPipelineState );

            bool CreateComputePipelineStateLayout( RHI::RHIComputePipelineStateCreateDesc const& createDesc, Render::ComputeShader const* pCompiledShader, VulkanComputePipelineState* pPipelineState );
            void DestroyComputePipelineStateLayout( VulkanComputePipelineState* pPipelineState );
            
            CombinedShaderSetLayout CombinedAllShaderSetLayouts( CompiledShaderArray const& compiledShaders );
            TPair<VkDescriptorSetLayout, TMap<uint32_t, RHI::EBindingResourceType>> CreateDescriptorSetLayout( uint32_t set, CombinedShaderBindingLayout const& combinedSetBindingLayout, VkShaderStageFlags stage );

            // Static Resources
            //-------------------------------------------------------------------------
            
            void CreateStaticSamplers();
            VkSampler FindImmutableSampler( String const& indicateString );
            void DestroyStaticSamplers();

            // Utility Functions
            //-------------------------------------------------------------------------
		
            bool VulkanDevice::GetMemoryType( uint32_t typeBits, VkMemoryPropertyFlags properties, uint32_t& memTypeFound ) const;
            uint32_t GetMaxBindlessDescriptorSampledImageCount() const;

            inline VulkanCommandBufferPool& GetCurrentFrameCommandBufferPool();

		private:

			TSharedPtr<VulkanInstance>			            m_pInstance = nullptr;
            TSharedPtr<VulkanSurface>                       m_pSurface = nullptr;

			VkDevice							            m_pHandle = nullptr;
			CollectedInfo						            m_collectInfos;
			VulkanPhysicalDevice				            m_physicalDevice;
            bool                                            m_frameExecuting = false;

            TEvent<RHI::RHITexture*>                        m_onSwapchainImageDestroyedEvent;

			VulkanCommandQueue*							    m_pGlobalGraphicQueue = nullptr;
            VulkanCommandBufferPool*                        m_commandBufferPool[NumDeviceFramebufferCount];

            VulkanCommandBufferPool*                        m_immediateCommandBufferPool;

            VulkanMemoryAllocator                           m_globalMemoryAllcator;

            THashMap<VulkanStaticSamplerDesc, VkSampler>    m_immutableSamplers;
		};
	}
}

#endif