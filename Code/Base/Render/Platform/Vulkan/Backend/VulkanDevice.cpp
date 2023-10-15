#if defined(EE_VULKAN)
#include "VulkanDevice.h"
#include "VulkanCommon.h"
#include "VulkanInstance.h"
#include "VulkanSurface.h"
#include "VulkanShader.h"
#include "VulkanSemaphore.h"
#include "VulkanUtils.h"
#include "VulkanTexture.h"
#include "VulkanBuffer.h"
#include "VulkanRenderPass.h"
#include "VulkanPipelineState.h"
#include "VulkanCommandBuffer.h"
#include "VulkanCommandQueue.h"
#include "RHIToVulkanSpecification.h"
#include "Base/RHI/RHIDowncastHelper.h"
#include "Base/Logging/Log.h"
#include "Base/Types/List.h"
#include "Base/Types/HashMap.h"
#include "Base/Resource/ResourcePtr.h"

namespace EE::Render
{
	namespace Backend
	{
		VulkanDevice::InitConfig VulkanDevice::InitConfig::GetDefault( bool enableDebug )
		{
			InitConfig config;
			config.m_requiredLayers = GetEngineVulkanDeviceRequiredLayers( enableDebug );
			config.m_requiredExtensions = GetEngineVulkanDeviceRequiredExtensions();
			return config;
		}

		//-------------------------------------------------------------------------

		VulkanDevice::VulkanDevice()
            : RHIDevice( RHI::ERHIType::Vulkan ), m_deviceFrameCount( 0 )
		{
            m_pInstance = MakeShared<VulkanInstance>();
            EE_ASSERT( m_pInstance != nullptr );

            m_pSurface = MakeShared<VulkanSurface>( m_pInstance );
            EE_ASSERT( m_pSurface != nullptr );

            InitConfig const config = InitConfig::GetDefault( m_pInstance->IsEnableDebug() );

            PickPhysicalDeviceAndCreate( config );

            m_globalMemoryAllcator.Initialize( this );

            CreateStaticSamplers();
        }

        VulkanDevice::VulkanDevice( InitConfig config )
            : RHIDevice( RHI::ERHIType::Vulkan ), m_deviceFrameCount( 0 )
		{
            m_pInstance = MakeShared<VulkanInstance>();
            EE_ASSERT( m_pInstance != nullptr );

            m_pSurface = MakeShared<VulkanSurface>( m_pInstance );
            EE_ASSERT( m_pSurface != nullptr );

            PickPhysicalDeviceAndCreate( config );

            m_globalMemoryAllcator.Initialize( this );

            CreateStaticSamplers();
		}

		VulkanDevice::~VulkanDevice()
		{
			EE_ASSERT( m_pHandle != nullptr );

            for ( auto& commandPool : m_commandBufferPool )
            {
                vkDestroyCommandPool( m_pHandle, commandPool.m_pHandle, nullptr );
            }

            if ( m_pGlobalGraphicQueue )
            {
                EE::Delete( m_pGlobalGraphicQueue );
            }

            DestroyStaticSamplers();
            m_globalMemoryAllcator.Shutdown();

			vkDestroyDevice( m_pHandle, nullptr );
			m_pHandle = nullptr;
		}

        //-------------------------------------------------------------------------

        size_t VulkanDevice::BeginFrame()
        {
            EE_ASSERT( !m_frameExecuting );

            // TODO: make sure all command buffers which allocated from
            //       this command pool had finished execution in GPU side.

            auto& commandPool = GetCurrentFrameCommandBufferPool();
            vkResetCommandPool( m_pHandle, commandPool.m_pHandle, 0 );

            m_frameExecuting = true;

            return m_deviceFrameCount;
        }

        void VulkanDevice::EndFrame()
        {
            EE_ASSERT( m_frameExecuting );

            ++m_deviceFrameCount;

            m_frameExecuting = false;
        }

        RHI::RHICommandBuffer* VulkanDevice::AllocateCommandBuffer()
        {
            EE_ASSERT( m_frameExecuting );

            auto& commandPool = GetCurrentFrameCommandBufferPool();

            VkCommandBufferAllocateInfo allocInfo = {};
            allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocInfo.commandPool = commandPool.m_pHandle;
            allocInfo.commandBufferCount = 1;

            auto* pVkCommandBuffer = EE::New<VulkanCommandBuffer>();
            if ( pVkCommandBuffer )
            {
                VK_SUCCEEDED( vkAllocateCommandBuffers( m_pHandle, &allocInfo, &(pVkCommandBuffer->m_pHandle) ) );
            
                return pVkCommandBuffer;
            }

            // TODO: remember to destroy delete command buffer

            return nullptr;
        }

        EE::RHI::RHICommandQueue* VulkanDevice::GetMainGraphicCommandQueue()
        {
            return m_pGlobalGraphicQueue;
        }

		//-------------------------------------------------------------------------

        RHI::RHITexture* VulkanDevice::CreateTexture( RHI::RHITextureCreateDesc const& createDesc )
        {
            EE_ASSERT( createDesc.IsValid() );

            VkImageCreateInfo imageCreateInfo = {};
            imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageCreateInfo.pNext = nullptr;
            imageCreateInfo.imageType = ToVulkanImageType( createDesc.m_type );
            imageCreateInfo.format = ToVulkanFormat( createDesc.m_format );
            imageCreateInfo.extent = { createDesc.m_width, createDesc.m_height, createDesc.m_depth };
            imageCreateInfo.mipLevels = createDesc.m_mipmap;
            imageCreateInfo.arrayLayers = createDesc.m_array;
            imageCreateInfo.samples = ToVulkanSampleCountFlags( createDesc.m_sample );
            imageCreateInfo.tiling = ToVulkanImageTiling( createDesc.m_tiling );
            imageCreateInfo.usage = ToVulkanImageUsageFlags( createDesc.m_usage );
            imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            imageCreateInfo.flags = ToVulkanImageCreateFlags( createDesc.m_flag );
            // TODO: queue specified resource
            imageCreateInfo.pQueueFamilyIndices = nullptr;
            imageCreateInfo.queueFamilyIndexCount = 0;

            bool bRequireDedicatedMemory = false;
            uint32_t allcatedMemorySize = 0;

            VulkanTexture* pVkTexture = EE::New<VulkanTexture>();

            #if VULKAN_USE_VMA_ALLOCATION
            VmaAllocationCreateInfo vmaAllocationCI = {};
            vmaAllocationCI.flags = VMA_ALLOCATION_CREATE_STRATEGY_BEST_FIT_BIT;
            if ( createDesc.m_usage.AreAnyFlagsSet( RHI::ETextureUsage::Color, RHI::ETextureUsage::DepthStencil )
                 && createDesc.m_width >= 1024
                 && createDesc.m_height >= 1024 )
            {
                vmaAllocationCI.flags |= VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
                bRequireDedicatedMemory = true;
            }

            // TODO: For now, all texture should be GPU Only.
            vmaAllocationCI.usage = ToVmaMemoryUsage( createDesc.m_memoryUsage );

            if ( pVkTexture )
            {
                VmaAllocationInfo allocInfo = {};
                VK_SUCCEEDED( vmaCreateImage( m_globalMemoryAllcator.m_pHandle, &imageCreateInfo, &vmaAllocationCI, &(pVkTexture->m_pHandle), &(pVkTexture->m_allocation), &allocInfo ) );
            
                allcatedMemorySize = static_cast<uint32_t>( allocInfo.size );
            }
            else
            {
                EE::Delete( pVkTexture );
            }
            #else
            if ( pVkTexture )
            {
                VK_SUCCEEDED( vkCreateImage( m_pHandle, &imageCreateInfo, nullptr, &(pVkTexture->m_pHandle) ) );
            }
            else
            {
                EE::Delete( pVkTexture );
            }

            VkMemoryRequirements memReqs = {};
            vkGetImageMemoryRequirements( m_pHandle, pVkTexture->m_pHandle, &memReqs );

            allcatedMemorySize = static_cast<uint32_t>( memReqs.size );

            VkMemoryAllocateInfo memAllloc = {};
            memAllloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            memAllloc.allocationSize = memReqs.size;
            GetMemoryType( memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, memAllloc.memoryTypeIndex );

            vkAllocateMemory( m_pHandle, &memAllloc, nullptr, &(pVkTexture->m_allocation) );
            vkBindImageMemory( m_pHandle, pVkTexture->m_pHandle, pVkTexture->m_allocation, 0);
            #endif // VULKAN_USE_VMA_ALLOCATION

            pVkTexture->m_desc = createDesc;
            // TODO: [WARNING] User's observation is different before render resource is created.
            // We modified or overwrite some property during creation, but user doesn't know it at all.
            // This is no a good design i think.
            if ( bRequireDedicatedMemory )
            {
                pVkTexture->m_desc.m_memoryFlag.SetFlag( RHI::ERenderResourceMemoryFlag::DedicatedMemory );
            }
            pVkTexture->m_desc.m_allocatedSize = allcatedMemorySize;

            return pVkTexture;
        }

        void VulkanDevice::DestroyTexture( RHI::RHITexture* pTexture )
        {
            EE_ASSERT( pTexture != nullptr );
            auto* pVkTexture = RHI::RHIDowncast<VulkanTexture>( pTexture );
            EE_ASSERT( pVkTexture->m_pHandle != nullptr );

            pVkTexture->ClearAllViews( this );
        
            #if VULKAN_USE_VMA_ALLOCATION
            if ( pVkTexture->m_allocation )
            {
                vmaDestroyImage( m_globalMemoryAllcator.m_pHandle, pVkTexture->m_pHandle, pVkTexture->m_allocation );
            }
            #else
            if ( pVkTexture->m_allocation != 0 )
            {
                vkDestroyImage( m_pHandle, pVkTexture->m_pHandle, nullptr);
                vkFreeMemory( m_pHandle, pVkTexture->m_allocation, nullptr );
            }
            #endif // VULKAN_USE_VMA_ALLOCATION

            EE::Delete( pTexture );
        }

        RHI::RHIBuffer* VulkanDevice::CreateBuffer( RHI::RHIBufferCreateDesc const& createDesc )
        {
            EE_ASSERT( createDesc.IsValid() );

            VkDeviceSize bufferSize = createDesc.m_desireSize;

            VkBufferCreateInfo bufferCreateInfo = {};
            bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferCreateInfo.size = bufferSize;
            bufferCreateInfo.usage = ToVulkanBufferUsageFlags( createDesc.m_usage );
            bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            uint32_t allcatedMemorySize = 0;

            VulkanBuffer* pVkBuffer = EE::New<VulkanBuffer>();

            #if VULKAN_USE_VMA_ALLOCATION
            VmaAllocationCreateInfo vmaAllocationCI = {};
            vmaAllocationCI.usage = ToVmaMemoryUsage( createDesc.m_memoryUsage );
            vmaAllocationCI.flags = VMA_ALLOCATION_CREATE_STRATEGY_BEST_FIT_BIT;
            if ( createDesc.m_memoryFlag.IsFlagSet(RHI::ERenderResourceMemoryFlag::DedicatedMemory) )
                vmaAllocationCI.flags |= VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
            if ( createDesc.m_memoryFlag.IsFlagSet( RHI::ERenderResourceMemoryFlag::PersistentMapping ) )
                vmaAllocationCI.flags |= VMA_ALLOCATION_CREATE_MAPPED_BIT;

            if ( pVkBuffer )
            {
                VmaAllocationInfo allocInfo = {};
                VK_SUCCEEDED( vmaCreateBuffer( m_globalMemoryAllcator.m_pHandle, &bufferCreateInfo, &vmaAllocationCI, &(pVkBuffer->m_pHandle), &(pVkBuffer->m_allocation), &allocInfo));
            
                allcatedMemorySize = static_cast<uint32_t>( allocInfo.size );
                if ( createDesc.m_memoryFlag.IsFlagSet( RHI::ERenderResourceMemoryFlag::PersistentMapping ) )
                {
                    pVkBuffer->m_pMappedMemory = allocInfo.pMappedData;
                }
            }
            else
            {
                EE::Delete( pVkBuffer );
            }
            #else
            if ( pVkBuffer )
            {
                VK_SUCCEEDED( vkCreateBuffer( m_pHandle, &bufferCreateInfo, nullptr, &(pVkBuffer->m_pHandle) ) );

            }
            else
            {
                EE::Delete( pVkBuffer );
            }

            VkMemoryRequirements memReqs;
            vkGetBufferMemoryRequirements( m_pHandle, pVkBuffer->m_pHandle, &memReqs );
            VkMemoryAllocateInfo memAlloc = {};
            memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            memAlloc.allocationSize = memReqs.size;

            allcatedMemorySize = static_cast<uint32_t>( memReqs.size );

            if ( createDesc.m_memoryUsage == RHI::ERenderResourceMemoryUsage::GPUOnly )
            {
                GetMemoryType( memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, memAlloc.memoryTypeIndex );
            }
            else if ( createDesc.m_memoryUsage == RHI::ERenderResourceMemoryUsage::CPUToGPU
                      || createDesc.m_memoryUsage == RHI::ERenderResourceMemoryUsage::CPUOnly )
            {
                GetMemoryType( memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, memAlloc.memoryTypeIndex );
            }
            else
            {
                EE_UNREACHABLE_CODE();
            }

            VK_SUCCEEDED( vkAllocateMemory( m_pHandle, &memAlloc, nullptr, &(pVkBuffer->m_allocation) ));
            VK_SUCCEEDED( vkBindBufferMemory( m_pHandle, pVkBuffer->m_pHandle, pVkBuffer->m_allocation, 0));
            #endif // VULKAN_USE_VMA_ALLOCATION

            pVkBuffer->m_desc = createDesc;
            pVkBuffer->m_desc.m_allocatedSize = allcatedMemorySize;

            return pVkBuffer;
        }

        void VulkanDevice::DestroyBuffer( RHI::RHIBuffer* pBuffer )
        {
            EE_ASSERT( pBuffer != nullptr );
            auto* pVkBuffer = RHI::RHIDowncast<VulkanBuffer>( pBuffer );
            EE_ASSERT( pVkBuffer->m_pHandle != nullptr );

            #if VULKAN_USE_VMA_ALLOCATION
            if ( pVkBuffer->m_allocation )
            {
                vmaDestroyBuffer( m_globalMemoryAllcator.m_pHandle, pVkBuffer->m_pHandle, pVkBuffer->m_allocation );
            }
            #else
            if ( pVkBuffer->m_allocation != 0 )
            {
                vkDestroyBuffer( m_pHandle, pVkBuffer->m_pHandle, nullptr );
                vkFreeMemory( m_pHandle, pVkBuffer->m_allocation, nullptr );
            }
            #endif // VULKAN_USE_VMA_ALLOCATION

            EE::Delete( pVkBuffer );
        }

        RHI::RHISemaphore* VulkanDevice::CreateSyncSemaphore( RHI::RHISemaphoreCreateDesc const& createDesc )
        {
            EE_ASSERT( createDesc.IsValid() );
            VulkanSemaphore* pVkSemaphore = EE::New<VulkanSemaphore>();
            EE_ASSERT( pVkSemaphore );

            VkSemaphoreCreateInfo semaphoreCI = {};
            semaphoreCI.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
            semaphoreCI.pNext = nullptr;
            semaphoreCI.flags = VkFlags( 0 );

            VK_SUCCEEDED( vkCreateSemaphore( m_pHandle, &semaphoreCI, nullptr, &(pVkSemaphore->m_pHandle) ) );

            return pVkSemaphore;
        }

        void VulkanDevice::DestroySyncSemaphore( RHI::RHISemaphore* pShader )
        {
            EE_ASSERT( pShader != nullptr );
            auto* pVkSemaphore = RHI::RHIDowncast<VulkanSemaphore>( pShader );
            EE_ASSERT( pVkSemaphore->m_pHandle != nullptr );

            vkDestroySemaphore( m_pHandle, pVkSemaphore->m_pHandle, nullptr );
            
            EE::Delete( pVkSemaphore );
        }

        RHI::RHIShader* VulkanDevice::CreateShader( RHI::RHIShaderCreateDesc const& createDesc )
        {
            EE_ASSERT( createDesc.IsValid() );
            VulkanShader* pVkShader = EE::New<VulkanShader>();
            EE_ASSERT( pVkShader );

            VkShaderModuleCreateInfo shaderModuleCI = {};
            shaderModuleCI.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            shaderModuleCI.pCode = reinterpret_cast<uint32_t const*>( createDesc.m_byteCode.data() );
            shaderModuleCI.codeSize = createDesc.m_byteCode.size(); // Note: here is the size in bytes!!

            VK_SUCCEEDED( vkCreateShaderModule( m_pHandle, &shaderModuleCI, nullptr, &(pVkShader->m_pModule) ) );

            return pVkShader;
        }

		void VulkanDevice::DestroyShader( RHI::RHIShader* pShader )
		{
            EE_ASSERT( pShader != nullptr );
            auto* pVkShader = RHI::RHIDowncast<VulkanShader>( pShader );
            EE_ASSERT( pVkShader->m_pModule != nullptr );

            vkDestroyShaderModule( m_pHandle, pVkShader->m_pModule, nullptr );

            EE::Delete( pVkShader );
		}

        //-------------------------------------------------------------------------

        RHI::RHIRenderPass* VulkanDevice::CreateRenderPass( RHI::RHIRenderPassCreateDesc const& createDesc )
        {
            EE_ASSERT( createDesc.IsValid() );
            VulkanRenderPass* pVkRenderPass = EE::New<VulkanRenderPass>();
            EE_ASSERT( pVkRenderPass );

            // Fill attachment descriptions
            //-------------------------------------------------------------------------
            
            TFixedVector<VkAttachmentDescription, RHI::RHIRenderPassCreateDesc::NumMaxAttachmentCount> attachmentDescriptions;

            for ( auto const& colorAttachment : createDesc.m_colorAttachments )
            {
                VkAttachmentDescription vkAttachmentDesc = ToVulkanAttachmentDescription( colorAttachment );
                vkAttachmentDesc.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                vkAttachmentDesc.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                attachmentDescriptions.push_back( vkAttachmentDesc );
            }

            // depth attachment is the last element (if any)
            if ( createDesc.m_depthAttachment.has_value() )
            {
                VkAttachmentDescription vkAttachmentDesc = ToVulkanAttachmentDescription( createDesc.m_depthAttachment.value() );
                vkAttachmentDesc.initialLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;
                vkAttachmentDesc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;
                attachmentDescriptions.push_back( vkAttachmentDesc );
            }

            // Fill attachment references
            //-------------------------------------------------------------------------
        
            TFixedVector<VkAttachmentReference, RHI::RHIRenderPassCreateDesc::NumMaxColorAttachmentCount> colorAttachmentRefs;
            VkAttachmentReference depthAttachmentRef;

            for ( uint32_t i = 0; i < createDesc.m_colorAttachments.size(); ++i )
            {
                VkAttachmentReference attachmentRef = {};
                attachmentRef.attachment = i;
                attachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                colorAttachmentRefs.push_back( attachmentRef );
            }

            depthAttachmentRef.attachment = static_cast<uint32_t>( createDesc.m_colorAttachments.size() );
            depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;

            // Fill subpass description
            //-------------------------------------------------------------------------

            VkSubpassDescription subpassDescription = {};
            subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpassDescription.colorAttachmentCount = static_cast<uint32_t>( createDesc.m_colorAttachments.size() );
            subpassDescription.pColorAttachments = colorAttachmentRefs.data();
            subpassDescription.pDepthStencilAttachment = createDesc.m_depthAttachment.has_value() ? &depthAttachmentRef : nullptr;
            
            // Create render pass
            //-------------------------------------------------------------------------

            // Not support subpass for now
            VkRenderPassCreateInfo renderPassCI = {};
            renderPassCI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            renderPassCI.attachmentCount = static_cast<uint32_t>( attachmentDescriptions.size() );
            renderPassCI.pAttachments = attachmentDescriptions.data();
            renderPassCI.subpassCount = 1;
            renderPassCI.pSubpasses = &subpassDescription;
            
            VK_SUCCEEDED( vkCreateRenderPass( m_pHandle, &renderPassCI, nullptr, &(pVkRenderPass->m_pHandle) ) );
            if ( !pVkRenderPass->m_pFramebufferCache->Initialize( pVkRenderPass, createDesc ) )
            {
                pVkRenderPass->m_pFramebufferCache->ClearUp( this );
                EE::Delete( pVkRenderPass );
                return nullptr;
            }
            
            pVkRenderPass->m_desc = createDesc;
            return pVkRenderPass;
        }

        void VulkanDevice::DestroyRenderPass( RHI::RHIRenderPass* pRenderPass )
        {
            EE_ASSERT( pRenderPass != nullptr );
            auto* pVkRenderPass = RHI::RHIDowncast<VulkanRenderPass>( pRenderPass );
            EE_ASSERT( pVkRenderPass->m_pHandle != nullptr );

            pVkRenderPass->m_pFramebufferCache->ClearUp( this );
            vkDestroyRenderPass( m_pHandle, pVkRenderPass->m_pHandle, nullptr );

            EE::Delete( pVkRenderPass );
        }

        //-------------------------------------------------------------------------

        RHI::RHIPipelineState* VulkanDevice::CreateRasterPipelineState( RHI::RHIRasterPipelineStateCreateDesc const& createDesc, CompiledShaderArray const& compiledShaders )
        {
            EE_ASSERT( createDesc.IsValid() );
            EE_ASSERT( !compiledShaders.empty() );

            VulkanPipelineState* pVkPipelineStage = EE::New<VulkanPipelineState>();
            EE_ASSERT( pVkPipelineStage );

            // Create pipeline layout
            //-------------------------------------------------------------------------

            if ( !CreateRasterPipelineStateLayout( createDesc, compiledShaders, pVkPipelineStage ) )
            {
                EE_LOG_WARNING( "RHI", "RHI::VulkanDevice", "Failed to create raster pipeline state!" );
                EE::Delete( pVkPipelineStage );
                return nullptr;
            }

            // Fill VkPipelineShaderStageCreateInfos
            //-------------------------------------------------------------------------
            
            TFixedVector<VkPipelineShaderStageCreateInfo, Render::NumPipelineStages> pipelineShaderStages;
            Render::Shader const* pVertexShader = nullptr;

            for ( auto const& compiledShader : compiledShaders )
            {
                // this shader must be loaded from ResourceLoader
                EE_ASSERT( compiledShader->GetRHIShader() != nullptr );

                VulkanShader* pVkShader = static_cast<VulkanShader*>( compiledShader->GetRHIShader() );

                auto& newPipelineShader = pipelineShaderStages.push_back();
                newPipelineShader.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
                newPipelineShader.flags = 0;
                newPipelineShader.pNext = nullptr;
                newPipelineShader.pSpecializationInfo = nullptr;
                newPipelineShader.pName = compiledShader->GetEntryName().c_str();
                newPipelineShader.stage = ToVulkanShaderStageFlags( compiledShader->GetPipelineStage() );
                newPipelineShader.module = pVkShader->m_pModule;

                if ( compiledShader->GetPipelineStage() == Render::PipelineStage::Vertex )
                {
                    pVertexShader = compiledShader;
                }
            }

            //-------------------------------------------------------------------------

            TInlineVector<VkVertexInputAttributeDescription, 7> vertexInputAttributions = {};
            TInlineVector<VkVertexInputBindingDescription, 2> vertexInputBindings = {};

            if ( pVertexShader != nullptr )
            {
                Render::VertexShader const* pShader = static_cast<Render::VertexShader const* const>( pVertexShader );
                auto const& vertexLayoutDesc = pShader->GetVertexLayoutDesc();

                uint32_t currentLocation = 0;
                for ( auto const& vertexElement : vertexLayoutDesc.m_elementDescriptors )
                {
                    VkVertexInputAttributeDescription attribute = {};
                    attribute.format = ToVulkanFormat( vertexElement.m_format );
                    attribute.binding = static_cast<uint32_t>( vertexElement.m_semanticIndex );
                    attribute.location = currentLocation;
                    attribute.offset = vertexElement.m_offset;

                    vertexInputAttributions.push_back(attribute);

                    ++currentLocation;
                }

                VkVertexInputBindingDescription vertexInputBinding = {};
                vertexInputBinding.binding = 0;
                // TODO: support instance input rage
                vertexInputBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
                vertexInputBinding.stride = vertexLayoutDesc.m_byteSize;

                vertexInputBindings.push_back( vertexInputBinding );
            }

            VkPipelineVertexInputStateCreateInfo vertexInputState = {};
            vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
            vertexInputState.vertexBindingDescriptionCount = static_cast<uint32_t>( vertexInputBindings.size() );
            vertexInputState.pVertexBindingDescriptions = vertexInputBindings.data();
            vertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>( vertexInputAttributions.size() );
            vertexInputState.pVertexAttributeDescriptions = vertexInputAttributions.data();

            //-------------------------------------------------------------------------

            VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = {};
            inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
            inputAssemblyState.topology = ToVulkanPrimitiveTopology( createDesc.m_primitiveTopology );

            //-------------------------------------------------------------------------

            VkPipelineRasterizationStateCreateInfo rasterizationState = {};
            rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
            rasterizationState.cullMode = ToVulkanCullModeFlags( createDesc.m_rasterizerState.m_cullMode );
            rasterizationState.frontFace = ToVulkanFrontFace( createDesc.m_rasterizerState.m_WindingMode );
            rasterizationState.polygonMode = ToVulkanPolygonMode( createDesc.m_rasterizerState.m_fillMode );
            rasterizationState.lineWidth = 1.0f;
            rasterizationState.depthBiasEnable = createDesc.m_enableDepthBias;
            
            //-------------------------------------------------------------------------

            // Don't specified scissors and viewport here, we bind it dynamically.
            VkPipelineViewportStateCreateInfo viewportState = {};
            viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
            viewportState.scissorCount = 1;
            viewportState.viewportCount = 1;

            //-------------------------------------------------------------------------

            // TODO: support multi-sample
            VkPipelineMultisampleStateCreateInfo multisampleState = {};
            multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
            multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

            //-------------------------------------------------------------------------

            VkStencilOpState stencilOpState = {};
            stencilOpState.failOp = VK_STENCIL_OP_KEEP;
            stencilOpState.passOp = VK_STENCIL_OP_KEEP;
            stencilOpState.depthFailOp = VK_STENCIL_OP_KEEP;
            stencilOpState.compareOp = VK_COMPARE_OP_ALWAYS;

            //-------------------------------------------------------------------------

            VkPipelineDepthStencilStateCreateInfo depthStencilState = {};
            depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
            depthStencilState.depthTestEnable = createDesc.m_enableDepthTest;
            depthStencilState.depthWriteEnable = createDesc.m_enableDepthWrite;
            // Note: Use reverse depth to gain better z-depth precision
            depthStencilState.depthCompareOp = VK_COMPARE_OP_GREATER_OR_EQUAL;
            depthStencilState.front = stencilOpState;
            depthStencilState.back = stencilOpState;
            depthStencilState.maxDepthBounds = 1.0f;
            depthStencilState.stencilTestEnable = false;

            //-------------------------------------------------------------------------

            VkPipelineColorBlendAttachmentState colorBlendAttachmentState = {};
            colorBlendAttachmentState.blendEnable = createDesc.m_blendState.m_blendEnable;
            colorBlendAttachmentState.srcColorBlendFactor = ToVulkanBlendFactor( createDesc.m_blendState.m_srcValue );
            colorBlendAttachmentState.dstColorBlendFactor = ToVulkanBlendFactor( createDesc.m_blendState.m_dstValue );
            colorBlendAttachmentState.colorBlendOp = ToVulkanBlendOp( createDesc.m_blendState.m_blendOp );
            colorBlendAttachmentState.srcAlphaBlendFactor = ToVulkanBlendFactor( createDesc.m_blendState.m_srcAlphaValue );
            colorBlendAttachmentState.dstAlphaBlendFactor = ToVulkanBlendFactor( createDesc.m_blendState.m_dstAlphaValue );
            colorBlendAttachmentState.alphaBlendOp = ToVulkanBlendOp( createDesc.m_blendState.m_blendOpAlpha );
            colorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

            VulkanRenderPass* pVkRenderPass = reinterpret_cast<VulkanRenderPass*>( createDesc.m_pRenderpass );
            
            TFixedVector<VkPipelineColorBlendAttachmentState, RHI::RHIRenderPassCreateDesc::NumMaxColorAttachmentCount> colorBlendAttachmentStates(
                pVkRenderPass->m_pFramebufferCache->GetColorAttachmentCount(),
                colorBlendAttachmentState
            );

            VkPipelineColorBlendStateCreateInfo colorBlendState = {};
            colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
            colorBlendState.attachmentCount = pVkRenderPass->m_pFramebufferCache->GetColorAttachmentCount();
            colorBlendState.pAttachments = colorBlendAttachmentStates.data();

            //-------------------------------------------------------------------------

            // enable dynamically bind viewport and scissor in command buffer
            TInlineVector<VkDynamicState, 9> dynamicStates = {};
            VkPipelineDynamicStateCreateInfo dynamicState = {};
            dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
            if ( createDesc.m_enableDepthBias )
            {
                dynamicStates.push_back( VkDynamicState::VK_DYNAMIC_STATE_VIEWPORT );
                dynamicStates.push_back( VkDynamicState::VK_DYNAMIC_STATE_SCISSOR );
                dynamicStates.push_back( VkDynamicState::VK_DYNAMIC_STATE_DEPTH_BIAS );

                dynamicState.pDynamicStates = dynamicStates.data();
            }
            else
            {
                dynamicStates.push_back( VkDynamicState::VK_DYNAMIC_STATE_VIEWPORT );
                dynamicStates.push_back( VkDynamicState::VK_DYNAMIC_STATE_SCISSOR );

                dynamicState.pDynamicStates = dynamicStates.data();
            }
            dynamicState.dynamicStateCount = static_cast<uint32_t>( dynamicStates.size() );

            //-------------------------------------------------------------------------

            VkGraphicsPipelineCreateInfo graphicsPipelineCI = {};
            graphicsPipelineCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
            graphicsPipelineCI.stageCount = static_cast<uint32_t>( pipelineShaderStages.size() );
            graphicsPipelineCI.pStages = pipelineShaderStages.data();
            graphicsPipelineCI.layout = pVkPipelineStage->m_pPipelineLayout;
            graphicsPipelineCI.pVertexInputState = &vertexInputState;
            graphicsPipelineCI.pInputAssemblyState = &inputAssemblyState;
            graphicsPipelineCI.pRasterizationState = &rasterizationState;
            graphicsPipelineCI.pViewportState = &viewportState;
            graphicsPipelineCI.pMultisampleState = &multisampleState;
            graphicsPipelineCI.pDepthStencilState = &depthStencilState;
            graphicsPipelineCI.pColorBlendState = &colorBlendState;
            graphicsPipelineCI.pDynamicState = &dynamicState;
            graphicsPipelineCI.renderPass = pVkRenderPass->m_pHandle;

            // TODO: support batched pipeline creation
            VK_SUCCEEDED( vkCreateGraphicsPipelines( m_pHandle, nullptr, 1, &graphicsPipelineCI, nullptr, &(pVkPipelineStage->m_pPipeline) ) );

            return pVkPipelineStage;
        }

        void VulkanDevice::DestroyRasterPipelineState( RHI::RHIPipelineState* pPipelineState )
        {
            EE_ASSERT( pPipelineState != nullptr );
            auto* pVkPipelineState = RHI::RHIDowncast<VulkanPipelineState>( pPipelineState );
            EE_ASSERT( pVkPipelineState->m_pPipeline != nullptr );

            DestroyRasterPipelineStateLayout( pVkPipelineState );
            vkDestroyPipeline( m_pHandle, pVkPipelineState->m_pPipeline, nullptr );

            EE::Delete( pVkPipelineState );
        }

		//-------------------------------------------------------------------------

        void VulkanDevice::PickPhysicalDeviceAndCreate( InitConfig const& config )
        {
            auto pdDevices = m_pInstance->EnumeratePhysicalDevice();

            for ( auto& pd : pdDevices )
            {
                pd.CalculatePickScore( m_pSurface );
            }

            auto physicalDevice = PickMostSuitablePhysicalDevice( pdDevices );
            m_physicalDevice = std::move( physicalDevice );

            EE_ASSERT( CheckAndCollectDeviceLayers( config ) );
            EE_ASSERT( CheckAndCollectDeviceExtensions( config ) );

            EE_ASSERT( CreateDevice( config ) );
        }

		bool VulkanDevice::CheckAndCollectDeviceLayers( InitConfig const& config )
		{
			uint32_t layerCount = 0;
			VK_SUCCEEDED( vkEnumerateDeviceLayerProperties( m_physicalDevice.m_pHandle, &layerCount, nullptr ) );

			EE_ASSERT( layerCount > 0 );

			TVector<VkLayerProperties> layerProps( layerCount );
			VK_SUCCEEDED( vkEnumerateDeviceLayerProperties( m_physicalDevice.m_pHandle, &layerCount, layerProps.data() ) );
			m_collectInfos.m_deviceLayerProps = layerProps;

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
					EE_LOG_ERROR( "Render", "Vulkan Backend", "Device layer not found: %s", required );
					return false;
				}
			}

			return true;
		}

		bool VulkanDevice::CheckAndCollectDeviceExtensions( InitConfig const& config )
		{
			uint32_t extCount = 0;
			VK_SUCCEEDED( vkEnumerateDeviceExtensionProperties( m_physicalDevice.m_pHandle, nullptr, &extCount, nullptr ) );

			EE_ASSERT( extCount > 0 );

			TVector<VkExtensionProperties> extProps( extCount );
			VK_SUCCEEDED( vkEnumerateDeviceExtensionProperties( m_physicalDevice.m_pHandle, nullptr, &extCount, extProps.data() ) );
			m_collectInfos.m_deviceExtensionProps = extProps;

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
					EE_LOG_ERROR( "Render", "Vulkan Backend", "Device extension not found: %s", required );
					return false;
				}
			}

			return true;
		}

		bool VulkanDevice::CreateDevice( InitConfig const& config )
		{
			// device queue creation info population
			//-------------------------------------------------------------------------

			TVector<VkDeviceQueueCreateInfo> deviceQueueCIs = {};
			TVector<QueueFamily> deviceQueueFamilies = {};

			float priorities[] = { 1.0f };

			// only create one graphic queue for now
			for ( auto const& qf : m_physicalDevice.m_queueFamilies )
			{
				if ( qf.IsGraphicQueue() )
				{
					VkDeviceQueueCreateInfo dqCI = {};
					dqCI.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
					dqCI.flags = VkFlags( 0 );
					dqCI.pNext = nullptr;

					dqCI.queueCount = 1;
					dqCI.queueFamilyIndex = qf.m_index;
					dqCI.pQueuePriorities = priorities;

					deviceQueueCIs.push_back( dqCI );
					deviceQueueFamilies.push_back( qf );
					break;
				}
			}

			if ( deviceQueueCIs.empty() )
			{
				EE_LOG_ERROR( "Render", "Vulkan Backend", "Invalid physical device which not supports graphic queue!" );
				return false;
			}

			// physical device features2 validation
			//-------------------------------------------------------------------------

			auto descriptor_indexing = VkPhysicalDeviceDescriptorIndexingFeaturesEXT{};
			descriptor_indexing.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT;

			auto imageless_framebuffer = VkPhysicalDeviceImagelessFramebufferFeaturesKHR{};
			imageless_framebuffer.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGELESS_FRAMEBUFFER_FEATURES_KHR;

			auto buffer_address = VkPhysicalDeviceBufferDeviceAddressFeaturesEXT{};
			buffer_address.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES_EXT;

			// TODO: pNext chain
			VkPhysicalDeviceFeatures2 physicalDeviceFeatures2 = {};
			physicalDeviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;

			physicalDeviceFeatures2.pNext = &descriptor_indexing;
			descriptor_indexing.pNext = &imageless_framebuffer;
			imageless_framebuffer.pNext = &buffer_address;

			vkGetPhysicalDeviceFeatures2( m_physicalDevice.m_pHandle, &physicalDeviceFeatures2 );

			EE_ASSERT( imageless_framebuffer.imagelessFramebuffer );
			EE_ASSERT( descriptor_indexing.descriptorBindingPartiallyBound );
			EE_ASSERT( buffer_address.bufferDeviceAddress );

			// device creation
			//-------------------------------------------------------------------------

			VkDeviceCreateInfo deviceCI = {};
			deviceCI.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
			deviceCI.flags = VkFlags( 0 );
			deviceCI.pNext = &physicalDeviceFeatures2;

			deviceCI.pQueueCreateInfos = deviceQueueCIs.data();
			deviceCI.queueCreateInfoCount = static_cast<uint32_t>( deviceQueueCIs.size() );

			deviceCI.ppEnabledLayerNames = config.m_requiredLayers.data();
			deviceCI.enabledLayerCount = static_cast<uint32_t>( config.m_requiredLayers.size() );
			deviceCI.ppEnabledExtensionNames = config.m_requiredExtensions.data();
			deviceCI.enabledExtensionCount = static_cast<uint32_t>( config.m_requiredExtensions.size() );

			VK_SUCCEEDED( vkCreateDevice( m_physicalDevice.m_pHandle, &deviceCI, nullptr, &m_pHandle ) );

			// fetch global device queue
			//-------------------------------------------------------------------------

            m_pGlobalGraphicQueue = EE::New<VulkanCommandQueue>( *this, deviceQueueFamilies[0] );

            // create global render command pools
            //-------------------------------------------------------------------------

            for ( auto& commandPool : m_commandBufferPool )
            {
                VkCommandPoolCreateInfo poolCI = {};
                poolCI.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
                poolCI.queueFamilyIndex = m_pGlobalGraphicQueue->GetDeviceIndex();

                VK_SUCCEEDED( vkCreateCommandPool( m_pHandle, &poolCI, nullptr, &(commandPool.m_pHandle) ) );
            }

			return true;
		}

        //-------------------------------------------------------------------------

        bool VulkanDevice::CreateRasterPipelineStateLayout( RHI::RHIRasterPipelineStateCreateDesc const& createDesc, CompiledShaderArray const& compiledShaders, VulkanPipelineState* pPipelineState )
        {
            EE_ASSERT( pPipelineState != nullptr );
            EE_ASSERT( pPipelineState->m_pPipelineLayout == nullptr);
            EE_ASSERT( pPipelineState->m_pPipeline == nullptr);
            EE_ASSERT( !createDesc.m_pipelineShaders.empty() );

            // Combined different shaders' set layout into one set layouts
            //-------------------------------------------------------------------------
            
            CombinedShaderSetLayout combinedSetLayouts = CombinedAllShaderSetLayouts( compiledShaders );

            // Find the number of set
            //-------------------------------------------------------------------------
            
            uint32_t setCount = 0;
            for ( auto const& combineSetLayout : combinedSetLayouts )
            {
                setCount = Math::Max( combineSetLayout.first + 1, setCount );
            }

            // Create descriptor set layout
            //-------------------------------------------------------------------------
            TInlineVector<VkDescriptorSetLayout, Render::Shader::NumMaxResourceBindingSet> vkSetLayouts;
            TInlineVector<TMap<uint32_t, VkDescriptorType>, Render::Shader::NumMaxResourceBindingSet> vkSetDescripotrTypes;
            for ( uint32_t set = 0; set < setCount; ++set )
            {
                auto layout = CreateDescriptorSetLayout( set, combinedSetLayouts[set], VK_SHADER_STAGE_ALL_GRAPHICS );
                vkSetLayouts.push_back( layout.first );
                vkSetDescripotrTypes.push_back( layout.second );
            }

            // Combine all push constants
            //-------------------------------------------------------------------------

            TBitFlags<PipelineStage> pipelineStageFlag = {};
            VkPushConstantRange pushConstantRange = {};
            // do check after first pushConstantRange assignment
            bool bDoConsistencyCheck = false;
            uint32_t checkID = 0;

            for ( auto const& compiledShader : compiledShaders )
            {
                auto const& pushConstant = compiledShader->GetPushConstant();

                // if the size of a push constant is zero, it has no push constant.
                if ( pushConstant.m_size == 0 )
                {
                    continue;
                }

                if ( bDoConsistencyCheck )
                {
                    EE_ASSERT( checkID == pushConstant.m_ID );
                    EE_ASSERT( pushConstantRange.size == pushConstant.m_size );
                    EE_ASSERT( pushConstantRange.offset == pushConstant.m_offset );

                    pipelineStageFlag.SetFlag( compiledShader->GetPipelineStage() );
                    continue;
                }

                checkID = pushConstant.m_ID;
                pushConstantRange.size = pushConstant.m_size;
                pushConstantRange.offset = pushConstant.m_offset;

                pipelineStageFlag.SetFlag( compiledShader->GetPipelineStage() );
                bDoConsistencyCheck = true;
            }

            pushConstantRange.stageFlags = ToVulkanShaderStageFlags( pipelineStageFlag );

            //-------------------------------------------------------------------------

            VkPipelineLayoutCreateInfo pipelineLayoutCI = {};
            pipelineLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipelineLayoutCI.pNext = nullptr;
            pipelineLayoutCI.setLayoutCount = static_cast<uint32_t>( vkSetLayouts.size() );
            pipelineLayoutCI.pSetLayouts = vkSetLayouts.data();
            pipelineLayoutCI.pushConstantRangeCount = ( pushConstantRange.size == 0 ) ? 0 : 1;
            pipelineLayoutCI.pPushConstantRanges = ( pushConstantRange.size == 0 ) ? nullptr : &pushConstantRange;

            VK_SUCCEEDED( vkCreatePipelineLayout(m_pHandle, &pipelineLayoutCI, nullptr, &(pPipelineState->m_pPipelineLayout) ) );

            //-------------------------------------------------------------------------

            pPipelineState->m_setLayouts = vkSetLayouts;
            pPipelineState->m_setDescriptorLayouts = vkSetDescripotrTypes;
            pPipelineState->m_pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

            return true;
        }

        void VulkanDevice::DestroyRasterPipelineStateLayout( VulkanPipelineState* pPipelineState )
        {
            EE_ASSERT( pPipelineState != nullptr );
            EE_ASSERT( pPipelineState->m_pPipelineLayout != nullptr );

            for ( auto& pSetLayout : pPipelineState->m_setLayouts )
            {
                vkDestroyDescriptorSetLayout( m_pHandle, pSetLayout, nullptr );
            }
            vkDestroyPipelineLayout( m_pHandle, pPipelineState->m_pPipelineLayout, nullptr );
        }

        VulkanDevice::CombinedShaderSetLayout VulkanDevice::CombinedAllShaderSetLayouts( CompiledShaderArray const& compiledShaders )
        {
            CombinedShaderSetLayout combinedSetLayouts;

            for ( auto const& compiledShader : compiledShaders )
            {
                if ( compiledShader )
                {
                    auto const& resourceBindingSetLayout = compiledShader->GetResourceBindingSetLayout();

                    // initialize combinedSetLayouts 
                    for ( uint32_t i = 0; i < static_cast<uint32_t>( resourceBindingSetLayout.size() ); ++i )
                    {
                        if ( combinedSetLayouts.find( i ) == combinedSetLayouts.end() )
                        {
                            combinedSetLayouts.insert( i );
                        }
                    }

                    // for each binding in set
                    for ( uint32_t i = 0; i < static_cast<uint32_t>( resourceBindingSetLayout.size() ); ++i )
                    {
                        auto& setLayout = resourceBindingSetLayout[i];
                        auto& combinedBindingLayout = combinedSetLayouts.at( i );

                        for ( Render::Shader::ResourceBinding const& binding : setLayout )
                        {
                            auto combinedBinding = combinedBindingLayout.find( binding.m_slot );
                            if ( combinedBinding == combinedBindingLayout.end() )
                            {
                                combinedBindingLayout.insert_or_assign( binding.m_slot, binding );
                            }
                            else
                            {
                                // do check
                                Render::Shader::ResourceBinding const& existsBinding = combinedBinding->second;
                                EE_ASSERT( existsBinding.m_ID == binding.m_ID );
                                EE_ASSERT( existsBinding.m_slot == binding.m_slot );
                                EE_ASSERT( existsBinding.m_bindingCount == binding.m_bindingCount );
                                EE_ASSERT( existsBinding.m_bindingResourceType == binding.m_bindingResourceType );
                            }
                        }
                    }

                }
            }

            return combinedSetLayouts;
        }

        TPair<VkDescriptorSetLayout, TMap<uint32_t, VkDescriptorType>> VulkanDevice::CreateDescriptorSetLayout( uint32_t set, CombinedShaderBindingLayout const& combinedSetBindingLayout, VkShaderStageFlags stage )
        {
            TVector<VkDescriptorSetLayoutBinding> vkBindings;
            vkBindings.reserve( combinedSetBindingLayout.size() );

            // Enable partially binding.
            // Note for VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT: 
            // If a descriptor has no memory access by shader, it can be invalid descriptor.
            TVector<VkDescriptorBindingFlags> vkBindingFlags;
            vkBindingFlags.resize( combinedSetBindingLayout.size() );
            for ( VkDescriptorBindingFlags& flag : vkBindingFlags )
            {
                flag = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;
            }

            VkDescriptorSetLayoutCreateFlags vkSetLayoutCreateFlag = 0;
            TInlineList<VkSampler const, 8> vkSamplerStackHolder;

            TMap<uint32_t, VkDescriptorType> bindingTypeInfo;

            for ( auto const& bindingLayout : combinedSetBindingLayout )
            {
                auto bindingResourceType = bindingLayout.second.m_bindingResourceType;
                VkDescriptorType vkDescriptorType = ToVulkanDescriptorType( bindingLayout.second.m_bindingResourceType );

                switch ( bindingResourceType )
                {
                    case EE::Render::Shader::ReflectedBindingResourceType::SampledImage:
                    {
                        uint32_t descriptorCount = static_cast<uint32_t>( bindingLayout.second.m_bindingCount.m_count );

                        if ( bindingLayout.second.m_bindingCount.m_type == Render::Shader::BindingCountType::Dynamic )
                        {
                            // Bindless descriptor can only be used at the end of this set.
                            EE_ASSERT( bindingLayout.first == static_cast<uint32_t>( combinedSetBindingLayout.size() - 1 ) );

                            // Enable all bindless descriptor flags.
                            // This binding can be updated as long as it is not dynamically used by any shader invocations.
                            // Note: dynamically used by any shader invocations means any shader invocation executes an instruction that performs any memory access using this descriptor.
                            vkBindingFlags[bindingLayout.first] |= VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT
                                | VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT
                                | VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT;

                            // Indicate that the descriptors inside this descriptor pool can be updated after binding.
                            vkSetLayoutCreateFlag |= VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;

                            descriptorCount = GetMaxBindlessDescriptorSampledImageCount();
                        }

                        VkDescriptorSetLayoutBinding newBinding = {};
                        newBinding.binding = bindingLayout.first;
                        newBinding.descriptorCount = descriptorCount;
                        newBinding.descriptorType = vkDescriptorType;
                        newBinding.pImmutableSamplers = nullptr;
                        newBinding.stageFlags = stage;

                        vkBindings.push_back( newBinding );

                        break;
                    }
                    case EE::Render::Shader::ReflectedBindingResourceType::StorageTexelBuffer:
                    case EE::Render::Shader::ReflectedBindingResourceType::StorageImage:
                    case EE::Render::Shader::ReflectedBindingResourceType::UniformTexelBuffer:
                    case EE::Render::Shader::ReflectedBindingResourceType::UniformBuffer:
                    case EE::Render::Shader::ReflectedBindingResourceType::StorageBuffer:
                    {
                        if ( bindingLayout.second.m_bindingCount.m_type == Render::Shader::BindingCountType::Dynamic )
                        {
                            EE_LOG_ERROR( "Render", "Vulkan Device", "StorageImage/UniformTexelBuffer/UniformBuffer/StorageBuffer doesn't support bindless descriptor set!");
                            EE_ASSERT( false );
                        }

                        VkDescriptorSetLayoutBinding newBinding = {};
                        newBinding.binding = bindingLayout.first;
                        newBinding.descriptorCount = static_cast<uint32_t>( bindingLayout.second.m_bindingCount.m_count );
                        newBinding.descriptorType = vkDescriptorType;
                        newBinding.pImmutableSamplers = nullptr;
                        newBinding.stageFlags = stage;

                        vkBindings.push_back( newBinding );

                        break;
                    }
                    case EE::Render::Shader::ReflectedBindingResourceType::Sampler:
                    {
                        VkSampler const vkSampler = FindImmutableSampler( bindingLayout.second.m_extraInfos );
                        vkSamplerStackHolder.push_front( vkSampler );

                        VkDescriptorSetLayoutBinding newBinding = {};
                        newBinding.binding = bindingLayout.first;
                        newBinding.descriptorCount = 1;
                        newBinding.descriptorType = vkDescriptorType;
                        newBinding.pImmutableSamplers = &vkSamplerStackHolder.front();
                        newBinding.stageFlags = stage;

                        vkBindings.push_back( newBinding );

                        break;
                    }
                    case EE::Render::Shader::ReflectedBindingResourceType::CombinedImageSampler:
                    case EE::Render::Shader::ReflectedBindingResourceType::InputAttachment:
                    {
                        EE_UNIMPLEMENTED_FUNCTION();
                        break;
                    }
                    default:
                    EE_UNREACHABLE_CODE();
                    break;
                }

                bindingTypeInfo.insert( { bindingLayout.first, vkDescriptorType } );
            }

            VkDescriptorSetLayoutBindingFlagsCreateInfo setLayoutBindingFlagsCreateInfo = {};
            setLayoutBindingFlagsCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
            setLayoutBindingFlagsCreateInfo.pNext = nullptr;
            setLayoutBindingFlagsCreateInfo.bindingCount = static_cast<uint32_t>( vkBindingFlags.size() );
            setLayoutBindingFlagsCreateInfo.pBindingFlags = vkBindingFlags.data();

            VkDescriptorSetLayoutCreateInfo setLayoutCI = {};
            setLayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            setLayoutCI.pNext = &setLayoutBindingFlagsCreateInfo;
            setLayoutCI.bindingCount = static_cast<uint32_t>( vkBindings.size() );
            setLayoutCI.pBindings = vkBindings.data();
            setLayoutCI.flags = vkSetLayoutCreateFlag;

            VkDescriptorSetLayout vkDescriptorSetLayout;
            VK_SUCCEEDED( vkCreateDescriptorSetLayout( m_pHandle, &setLayoutCI, nullptr, &vkDescriptorSetLayout ) );

            return { vkDescriptorSetLayout, bindingTypeInfo };
        }

        // Static Resources
        //-------------------------------------------------------------------------

        void VulkanDevice::CreateStaticSamplers()
        {
            #define SIZE_OF_ARRAY(arr) (sizeof(arr) / sizeof(arr[0]))

            VkFilter const allFilters[] = { VK_FILTER_LINEAR, VK_FILTER_NEAREST };
            VkSamplerMipmapMode const allMipmapModes[] = { VK_SAMPLER_MIPMAP_MODE_LINEAR, VK_SAMPLER_MIPMAP_MODE_NEAREST };
            VkSamplerAddressMode const allAddressModes[] = {
                VK_SAMPLER_ADDRESS_MODE_REPEAT,
                VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
                VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
                VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE
            };

            for ( uint32_t filter = 0; filter < SIZE_OF_ARRAY( allFilters ); ++filter )
            {
                for ( uint32_t mipmap = 0; mipmap < SIZE_OF_ARRAY( allMipmapModes ); ++mipmap )
                {
                    for ( uint32_t address = 0; address < SIZE_OF_ARRAY( allAddressModes ); ++address )
                    {
                        VkSamplerCreateInfo samplerCreateInfo = {};
                        samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
                        samplerCreateInfo.minFilter = allFilters[filter];
                        samplerCreateInfo.magFilter = allFilters[filter];
                        samplerCreateInfo.mipmapMode = allMipmapModes[mipmap];
                        samplerCreateInfo.addressModeU = allAddressModes[address];
                        samplerCreateInfo.addressModeV = allAddressModes[address];
                        samplerCreateInfo.addressModeW = allAddressModes[address];
                        samplerCreateInfo.maxLod = VK_LOD_CLAMP_NONE;
                        samplerCreateInfo.maxAnisotropy = 16.0f;
                        samplerCreateInfo.anisotropyEnable = allFilters[filter] == VK_FILTER_LINEAR;

                        VulkanStaticSamplerDesc desc{ allFilters[filter], allMipmapModes[mipmap], allAddressModes[address] };
                        //VulkanStaticSamplerDesc desc{ .filter = allFilters[filter], .mipmap = allMipmapModes[mipmap], .address = allAddressModes[address] };

                        VkSampler newSampler = nullptr;
                        VK_SUCCEEDED( vkCreateSampler( m_pHandle, &samplerCreateInfo, nullptr, &newSampler ) );

                        m_immutableSamplers[desc] = newSampler;
                    }
                }
            }

            #undef SIZE_OF_ARRAY
        }

        VkSampler VulkanDevice::FindImmutableSampler( String const& indicateString )
        {
            EE_ASSERT( !indicateString.empty() );

            VulkanStaticSamplerDesc desc = {};

            size_t currPos = 0;
            char const& filterIndicator = indicateString[currPos++];
            if ( filterIndicator == 'l' )
            {
                desc.filter = VK_FILTER_LINEAR;
            }
            else if ( filterIndicator == 'n' )
            {
                desc.filter = VK_FILTER_NEAREST;
            }
            else
            {
                EE_LOG_WARNING( "Render", "VulkanDevice::CreateDescriptorSetLayout", "Invalid sampler filter type!" );
                return nullptr;
            }

            char const& mipmapIndicator = indicateString[currPos++];
            if ( mipmapIndicator == 'l' )
            {
                desc.mipmap = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            }
            else if ( mipmapIndicator == 'n' )
            {
                desc.mipmap = VK_SAMPLER_MIPMAP_MODE_NEAREST;
            }
            else
            {
                EE_LOG_WARNING( "Render", "VulkanDevice::CreateDescriptorSetLayout", "Invalid sampler mipmap mode type!" );
                return nullptr;
            }

            char const& addressIndicator = indicateString[currPos];
            if ( addressIndicator == 'c' )
            {
                if ( indicateString.size() < currPos + 1 )
                {
                    EE_LOG_WARNING( "Render", "VulkanDevice::CreateDescriptorSetLayout", "Invalid sampler info: %s", indicateString.c_str() );
                    return nullptr;
                }

                if ( indicateString[currPos + 1] == 'e' )
                {
                    desc.address = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
                }
                else if ( indicateString[currPos + 1] == 'b' )
                {
                    desc.address = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
                }
                else
                {
                    EE_LOG_WARNING( "Render", "VulkanDevice::CreateDescriptorSetLayout", "Invalid sampler address type!" );
                    return nullptr;
                }
            }
            else if ( addressIndicator == 'r' )
            {
                desc.address = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            }
            else if ( addressIndicator == 'm' )
            {
                if ( indicateString.size() < currPos + 1 )
                {
                    EE_LOG_WARNING( "Render", "VulkanDevice::CreateDescriptorSetLayout", "Invalid sampler info: %s", indicateString.c_str() );
                    return nullptr;
                }

                if ( indicateString.find( 'r', currPos ) != String::npos )
                {
                    desc.address = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
                }
                else if ( indicateString.find( "ce", currPos ) != String::npos )
                {
                    desc.address = VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
                }
                else
                {
                    EE_LOG_WARNING( "Render", "VulkanDevice::CreateDescriptorSetLayout", "Invalid sampler address type!" );
                    return nullptr;
                }
            }
            else
            {
                EE_LOG_WARNING( "Render", "VulkanDevice::CreateDescriptorSetLayout", "Invalid sampler address type!" );
                return nullptr;
            }

            auto sampler = m_immutableSamplers.find( desc );
            if ( sampler != m_immutableSamplers.end() )
            {
                return sampler->second;
            }
            else
            {
                return nullptr;
            }
        }

        void VulkanDevice::DestroyStaticSamplers()
        {
            for ( auto& sampler : m_immutableSamplers )
            {
                EE_ASSERT( sampler.second != nullptr );
                vkDestroySampler( m_pHandle, sampler.second, nullptr );
            }

            m_immutableSamplers.clear();
        }

        // Utility functions
        //-------------------------------------------------------------------------

        bool VulkanDevice::GetMemoryType( uint32_t typeBits, VkMemoryPropertyFlags properties, uint32_t& OutProperties ) const
        {
            for ( uint32_t i = 0; i < m_physicalDevice.m_memoryProps.memoryTypeCount; i++ )
            {
                if ( ( typeBits & 1 ) == 1 )
                {
                    if ( ( m_physicalDevice.m_memoryProps.memoryTypes[i].propertyFlags & properties ) == properties )
                    {
                        OutProperties = i;
                        return true;
                    }
                }
                typeBits >>= 1;
            }

            OutProperties = 0;
            return false;
        }

        uint32_t VulkanDevice::GetMaxBindlessDescriptorSampledImageCount() const
        {
            return Math::Min(
                m_physicalDevice.m_props.limits.maxPerStageDescriptorSampledImages - DescriptorSetReservedSampledImageCount,
                BindlessDescriptorSetDesiredSampledImageCount
                );
        }

        VulkanCommandBufferPool& VulkanDevice::GetCurrentFrameCommandBufferPool()
        {
            auto deviceFrameIndex = m_deviceFrameCount % VulkanDevice::NumDeviceFrameCount;
            return m_commandBufferPool[deviceFrameIndex];
        }
    }
}

#endif