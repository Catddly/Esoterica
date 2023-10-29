#include "RHIToVulkanSpecification.h"
#if defined(EE_VULKAN)

namespace EE::Render
{
    namespace Backend
    {
        #if VULKAN_USE_VMA_ALLOCATION
        VmaMemoryUsage ToVmaMemoryUsage( RHI::ERenderResourceMemoryUsage memoryUsage )
        {
            switch ( memoryUsage )
            {
                case RHI::ERenderResourceMemoryUsage::CPUToGPU: return VMA_MEMORY_USAGE_CPU_TO_GPU;
                case RHI::ERenderResourceMemoryUsage::GPUToCPU: return VMA_MEMORY_USAGE_GPU_TO_CPU;
                case RHI::ERenderResourceMemoryUsage::CPUOnly: return VMA_MEMORY_USAGE_CPU_ONLY;
                case RHI::ERenderResourceMemoryUsage::GPUOnly: return VMA_MEMORY_USAGE_GPU_ONLY;
                case RHI::ERenderResourceMemoryUsage::CPUCopy: return VMA_MEMORY_USAGE_CPU_COPY;
                // TODO: transient attachment
                case RHI::ERenderResourceMemoryUsage::GPULazily: return VMA_MEMORY_USAGE_GPU_LAZILY_ALLOCATED;
                default:
                break;
            }
            EE_UNREACHABLE_CODE();
            return VMA_MEMORY_USAGE_MAX_ENUM;
        }
        #endif // VULKAN_USE_VMA_ALLOCATION

        //-------------------------------------------------------------------------

		VkFormat ToVulkanFormat( RHI::EPixelFormat format )
		{
            switch ( format )
            {
                case RHI::EPixelFormat::RGBA8Unorm: return VK_FORMAT_R8G8B8A8_UNORM;
                case RHI::EPixelFormat::BGRA8Unorm: return VK_FORMAT_B8G8R8A8_UNORM;
                case RHI::EPixelFormat::Undefined: return VK_FORMAT_UNDEFINED;
                default:
                break;
            }
            EE_UNREACHABLE_CODE();
            return VK_FORMAT_UNDEFINED;
		}

		VkFormat ToVulkanFormat( Render::DataFormat format )
		{
            switch ( format )
            {
                case EE::Render::DataFormat::UInt_R8: return VK_FORMAT_R8_UINT;
                case EE::Render::DataFormat::UInt_R8G8: return VK_FORMAT_R8G8_UINT;
                case EE::Render::DataFormat::UInt_R8G8B8A8: return VK_FORMAT_R8G8B8A8_UINT;
                case EE::Render::DataFormat::UNorm_R8: return VK_FORMAT_R8_UNORM;
                case EE::Render::DataFormat::UNorm_R8G8: return VK_FORMAT_R8G8_UNORM;
                case EE::Render::DataFormat::UNorm_R8G8B8A8: return VK_FORMAT_R8G8B8A8_UNORM;
                case EE::Render::DataFormat::UInt_R32: return VK_FORMAT_R32_UINT;
                case EE::Render::DataFormat::UInt_R32G32: return VK_FORMAT_R32G32_UINT;
                case EE::Render::DataFormat::UInt_R32G32B32: return VK_FORMAT_R32G32B32_UINT;
                case EE::Render::DataFormat::UInt_R32G32B32A32: return VK_FORMAT_R32G32B32A32_UINT;
                case EE::Render::DataFormat::SInt_R32: return VK_FORMAT_R32_SINT;
                case EE::Render::DataFormat::SInt_R32G32: return VK_FORMAT_R32G32_SINT;
                case EE::Render::DataFormat::SInt_R32G32B32: return VK_FORMAT_R32G32B32_SINT;
                case EE::Render::DataFormat::SInt_R32G32B32A32: return VK_FORMAT_R32G32B32A32_SINT;
                case EE::Render::DataFormat::Float_R16: return VK_FORMAT_R16_SFLOAT;
                case EE::Render::DataFormat::Float_R16G16: return VK_FORMAT_R16G16_SFLOAT;
                case EE::Render::DataFormat::Float_R16G16B16A16: return VK_FORMAT_R16G16B16A16_SFLOAT;
                case EE::Render::DataFormat::Float_R32: return VK_FORMAT_R32_SFLOAT;
                case EE::Render::DataFormat::Float_R32G32: return VK_FORMAT_R32G32_SFLOAT;
                case EE::Render::DataFormat::Float_R32G32B32: return VK_FORMAT_R32G32B32_SFLOAT;
                case EE::Render::DataFormat::Float_R32G32B32A32: return VK_FORMAT_R32G32B32A32_SFLOAT;
                case EE::Render::DataFormat::Float_X32:
                EE_UNIMPLEMENTED_FUNCTION();
                break;
                case EE::Render::DataFormat::Unknown:
                case EE::Render::DataFormat::Count:
                default:
                break;
            }

            EE_UNREACHABLE_CODE();
            return VK_FORMAT_UNDEFINED;
		}

        VkImageType ToVulkanImageType( RHI::ETextureType type )
        {
            switch ( type )
            {
                case RHI::ETextureType::T1D: return VK_IMAGE_TYPE_1D;
                case RHI::ETextureType::T1DArray: return VK_IMAGE_TYPE_1D;
                case RHI::ETextureType::T2D: return VK_IMAGE_TYPE_2D;
                case RHI::ETextureType::T2DArray: return VK_IMAGE_TYPE_2D;
                case RHI::ETextureType::T3D: return VK_IMAGE_TYPE_3D;
                case RHI::ETextureType::TCubemap: return VK_IMAGE_TYPE_2D;
                case RHI::ETextureType::TCubemapArray: return VK_IMAGE_TYPE_2D;
                default:
                break;
            }
            EE_UNREACHABLE_CODE();
            return VK_IMAGE_TYPE_MAX_ENUM;
        }

        VkImageViewType ToVulkanImageViewType( RHI::ETextureType type )
        {
            switch ( type )
            {
                case EE::RHI::ETextureType::T1D: return VK_IMAGE_VIEW_TYPE_1D;
                case EE::RHI::ETextureType::T1DArray: return VK_IMAGE_VIEW_TYPE_1D_ARRAY;
                case EE::RHI::ETextureType::T2D: return VK_IMAGE_VIEW_TYPE_2D;
                case EE::RHI::ETextureType::T2DArray: return VK_IMAGE_VIEW_TYPE_2D_ARRAY;
                case EE::RHI::ETextureType::T3D: return VK_IMAGE_VIEW_TYPE_3D;
                case EE::RHI::ETextureType::TCubemap: return VK_IMAGE_VIEW_TYPE_CUBE;
                case EE::RHI::ETextureType::TCubemapArray: return VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
                default:
                break;
            }
            EE_UNREACHABLE_CODE();
            return VK_IMAGE_VIEW_TYPE_MAX_ENUM;
        }

        VkImageViewType ToVulkanImageViewType( RHI::ETextureViewType type )
        {
            switch ( type )
            {
                case EE::RHI::ETextureViewType::TV1D: return VK_IMAGE_VIEW_TYPE_1D;
                case EE::RHI::ETextureViewType::TV2D: return VK_IMAGE_VIEW_TYPE_2D;
                case EE::RHI::ETextureViewType::TV3D: return VK_IMAGE_VIEW_TYPE_3D;
                case EE::RHI::ETextureViewType::TVCubemap: return VK_IMAGE_VIEW_TYPE_CUBE;
                case EE::RHI::ETextureViewType::TV1DArray: return VK_IMAGE_VIEW_TYPE_1D_ARRAY;
                case EE::RHI::ETextureViewType::TV2DArray: return VK_IMAGE_VIEW_TYPE_2D_ARRAY;
                case EE::RHI::ETextureViewType::TVCubemapArray: return VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
                default:
                break;
            }            
            EE_UNREACHABLE_CODE();
            return VK_IMAGE_VIEW_TYPE_MAX_ENUM;
        }

        VkImageAspectFlagBits ToVulkanImageAspectFlags( TBitFlags<RHI::ETextureViewAspect> aspect )
        {
            VkFlags flag = 0u;
            if ( aspect.IsFlagSet( RHI::ETextureViewAspect::Color ) )
            {
                flag |= VK_IMAGE_ASPECT_COLOR_BIT;
            }
            if ( aspect.IsFlagSet( RHI::ETextureViewAspect::Depth ) )
            {
                flag |= VK_IMAGE_ASPECT_DEPTH_BIT;
            }
            if ( aspect.IsFlagSet( RHI::ETextureViewAspect::Stencil ) )
            {
                flag |= VK_IMAGE_ASPECT_STENCIL_BIT;
            }
            if ( aspect.IsFlagSet( RHI::ETextureViewAspect::Metadata ) )
            {
                flag |= VK_IMAGE_ASPECT_METADATA_BIT;
            }
            if ( aspect.IsFlagSet( RHI::ETextureViewAspect::Plane0 ) )
            {
                flag |= VK_IMAGE_ASPECT_PLANE_0_BIT;
            }
            if ( aspect.IsFlagSet( RHI::ETextureViewAspect::Plane1 ) )
            {
                flag |= VK_IMAGE_ASPECT_PLANE_1_BIT;
            }
            if ( aspect.IsFlagSet( RHI::ETextureViewAspect::Plane2 ) )
            {
                flag |= VK_IMAGE_ASPECT_PLANE_2_BIT;
            }
            if ( aspect.IsFlagSet( RHI::ETextureViewAspect::None ) )
            {
                flag = VK_IMAGE_ASPECT_NONE;
            }
            return VkImageAspectFlagBits( flag );
        }

        VkSampleCountFlagBits ToVulkanSampleCountFlags( TBitFlags<RHI::ESampleCount> sample )
        {
            VkFlags flag = 0u;
            if ( sample.IsFlagSet( RHI::ESampleCount::SC1 ) )
            {
                flag |= VK_SAMPLE_COUNT_1_BIT;
            }
            if ( sample.IsFlagSet( RHI::ESampleCount::SC2 ) )
            {
                flag |= VK_SAMPLE_COUNT_2_BIT;
            }
            if ( sample.IsFlagSet( RHI::ESampleCount::SC4 ) )
            {
                flag |= VK_SAMPLE_COUNT_4_BIT;
            }
            if ( sample.IsFlagSet( RHI::ESampleCount::SC8 ) )
            {
                flag |= VK_SAMPLE_COUNT_8_BIT;
            }
            if ( sample.IsFlagSet( RHI::ESampleCount::SC16 ) )
            {
                flag |= VK_SAMPLE_COUNT_16_BIT;
            }
            if ( sample.IsFlagSet( RHI::ESampleCount::SC32 ) )
            {
                flag |= VK_SAMPLE_COUNT_32_BIT;
            }
            if ( sample.IsFlagSet( RHI::ESampleCount::SC64 ) )
            {
                flag |= VK_SAMPLE_COUNT_64_BIT;
            }
            return VkSampleCountFlagBits( flag );
        }

        VkImageUsageFlagBits ToVulkanImageUsageFlags( TBitFlags<RHI::ETextureUsage> usage )
        {
            VkFlags flag = 0u;
            if ( usage.IsFlagSet( RHI::ETextureUsage::TransferSrc ) )
            {
                flag |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
            }
            if ( usage.IsFlagSet( RHI::ETextureUsage::TransferDst ) )
            {
                flag |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
            }
            if ( usage.IsFlagSet( RHI::ETextureUsage::Sampled ) )
            {
                flag |= VK_IMAGE_USAGE_SAMPLED_BIT;
            }
            if ( usage.IsFlagSet( RHI::ETextureUsage::Storage ) )
            {
                flag |= VK_IMAGE_USAGE_STORAGE_BIT;
            }
            if ( usage.IsFlagSet( RHI::ETextureUsage::Color ) )
            {
                flag |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            }
            if ( usage.IsFlagSet( RHI::ETextureUsage::DepthStencil ) )
            {
                flag |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
            }
            if ( usage.IsFlagSet( RHI::ETextureUsage::Transient ) )
            {
                flag |= VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
            }
            if ( usage.IsFlagSet( RHI::ETextureUsage::Input ) )
            {
                flag |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
            }
            return VkImageUsageFlagBits( flag );
        }

		VkImageTiling ToVulkanImageTiling( RHI::ETextureMemoryTiling tiling )
		{
            switch ( tiling )
            {
                case RHI::ETextureMemoryTiling::Optimal: return VK_IMAGE_TILING_OPTIMAL;
                case RHI::ETextureMemoryTiling::Linear: return VK_IMAGE_TILING_LINEAR;
                default:
                break;
            }
            EE_UNREACHABLE_CODE();
            return VK_IMAGE_TILING_MAX_ENUM;
		}

        VkImageCreateFlagBits ToVulkanImageCreateFlags( TBitFlags<RHI::ETextureCreateFlag> createFlag )
        {
            VkFlags flag = 0u;
            if ( createFlag.IsFlagSet( RHI::ETextureCreateFlag::CubeCompatible ) )
            {
                flag |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
            }
            return VkImageCreateFlagBits( flag );
        }

		VkBufferUsageFlagBits ToVulkanBufferUsageFlags( TBitFlags<RHI::EBufferUsage> usage )
		{
            VkFlags flag = 0u;
            if ( usage.IsFlagSet( RHI::EBufferUsage::TransferSrc ) )
            {
                flag |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            }
            if ( usage.IsFlagSet( RHI::EBufferUsage::TransferDst ) )
            {
                flag |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            }
            if ( usage.IsFlagSet( RHI::EBufferUsage::UniformTexel ) )
            {
                flag |= VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT;
            }
            if ( usage.IsFlagSet( RHI::EBufferUsage::StorageTexel ) )
            {
                flag |= VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT;
            }
            if ( usage.IsFlagSet( RHI::EBufferUsage::Uniform ) )
            {
                flag |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
            }
            if ( usage.IsFlagSet( RHI::EBufferUsage::Storage ) )
            {
                flag |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
            }
            if ( usage.IsFlagSet( RHI::EBufferUsage::Index ) )
            {
                flag |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
            }
            if ( usage.IsFlagSet( RHI::EBufferUsage::Vertex ) )
            {
                flag |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
            }
            if ( usage.IsFlagSet( RHI::EBufferUsage::Indirect ) )
            {
                flag |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
            }
            if ( usage.IsFlagSet( RHI::EBufferUsage::ShaderDeviceAddress ) )
            {
                flag |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
            }
            return VkBufferUsageFlagBits( flag );
		}

        VkImageAspectFlags ToVkImageAspectFlags( TBitFlags<RHI::TextureAspectFlags> flags )
        {
            VkFlags vkFlag = 0;
            if ( flags.IsFlagSet( RHI::TextureAspectFlags::Color ) )
            {
                vkFlag |= VK_IMAGE_ASPECT_COLOR_BIT;
            }
            if ( flags.IsFlagSet( RHI::TextureAspectFlags::Depth ) )
            {
                vkFlag |= VK_IMAGE_ASPECT_DEPTH_BIT;
            }
            if ( flags.IsFlagSet( RHI::TextureAspectFlags::Metadata ) )
            {
                vkFlag |= VK_IMAGE_ASPECT_METADATA_BIT;
            }
            if ( flags.IsFlagSet( RHI::TextureAspectFlags::Stencil ) )
            {
                vkFlag |= VK_IMAGE_ASPECT_STENCIL_BIT;
            }
            return VkImageAspectFlags( vkFlag );
        }

        //-------------------------------------------------------------------------

        VkAttachmentLoadOp ToVulkanAttachmentLoadOp( RHI::ERenderPassAttachmentLoadOp loadOP )
        {
            switch ( loadOP )
            {
                case EE::RHI::ERenderPassAttachmentLoadOp::Load: return VK_ATTACHMENT_LOAD_OP_LOAD;
                case EE::RHI::ERenderPassAttachmentLoadOp::Clear: return VK_ATTACHMENT_LOAD_OP_CLEAR;
                case EE::RHI::ERenderPassAttachmentLoadOp::DontCare: return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                default:
                break;
            }
            EE_UNREACHABLE_CODE();
            return VK_ATTACHMENT_LOAD_OP_MAX_ENUM;
        }

        VkAttachmentStoreOp ToVulkanAttachmentStoreOp( RHI::ERenderPassAttachmentStoreOp storeOP )
        {
            switch ( storeOP )
            {
                case EE::RHI::ERenderPassAttachmentStoreOp::Store: return VK_ATTACHMENT_STORE_OP_STORE;
                case EE::RHI::ERenderPassAttachmentStoreOp::DontCare: return VK_ATTACHMENT_STORE_OP_DONT_CARE;
                default:
                break;
            }
            EE_UNREACHABLE_CODE();
            return VK_ATTACHMENT_STORE_OP_MAX_ENUM;
        }

        VkAttachmentDescription ToVulkanAttachmentDescription( RHI::RHIRenderPassAttachmentDesc const& attachmentDesc )
        {
            VkAttachmentDescription vkAttachmentDesc = {};
            vkAttachmentDesc.format = ToVulkanFormat( attachmentDesc.m_pixelFormat );
            vkAttachmentDesc.samples = ToVulkanSampleCountFlags( attachmentDesc.m_sample );
            vkAttachmentDesc.loadOp = ToVulkanAttachmentLoadOp( attachmentDesc.m_loadOp );
            vkAttachmentDesc.storeOp = ToVulkanAttachmentStoreOp( attachmentDesc.m_storeOp );
            vkAttachmentDesc.stencilLoadOp = ToVulkanAttachmentLoadOp( attachmentDesc.m_stencilLoadOp );
            vkAttachmentDesc.stencilStoreOp = ToVulkanAttachmentStoreOp( attachmentDesc.m_stencilStoreOp );
            vkAttachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            vkAttachmentDesc.finalLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            return vkAttachmentDesc;
        }

        //-------------------------------------------------------------------------

		VkDescriptorType ToVulkanDescriptorType( Render::Shader::ReflectedBindingResourceType reflectedBindingType )
		{
            // TODO: support dynamic uniform buffer and storage buffer
            switch ( reflectedBindingType )
            {
                case EE::Render::Shader::ReflectedBindingResourceType::Sampler: return VK_DESCRIPTOR_TYPE_SAMPLER;
                case EE::Render::Shader::ReflectedBindingResourceType::CombinedImageSampler: return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                case EE::Render::Shader::ReflectedBindingResourceType::SampledImage: return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
                case EE::Render::Shader::ReflectedBindingResourceType::StorageImage: return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                case EE::Render::Shader::ReflectedBindingResourceType::UniformTexelBuffer: return VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
                case EE::Render::Shader::ReflectedBindingResourceType::StorageTexelBuffer: return VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
                case EE::Render::Shader::ReflectedBindingResourceType::UniformBuffer: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                case EE::Render::Shader::ReflectedBindingResourceType::StorageBuffer: return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                case EE::Render::Shader::ReflectedBindingResourceType::InputAttachment: return VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
                default:
                break;
            }
            EE_UNREACHABLE_CODE();
            return VK_DESCRIPTOR_TYPE_MAX_ENUM;
		}

        //-------------------------------------------------------------------------

		VkShaderStageFlagBits ToVulkanShaderStageFlags( TBitFlags<Render::PipelineStage> pipelineStage )
		{
            if ( pipelineStage.IsFlagSet( PipelineStage::None ) )
            {
                return VkShaderStageFlagBits( 0u );
            }

            VkFlags flag = 0u;
            if ( pipelineStage.IsFlagSet( PipelineStage::Vertex ) )
            {
                flag |= VK_SHADER_STAGE_VERTEX_BIT;
            }
            if ( pipelineStage.IsFlagSet( PipelineStage::Pixel ) )
            {
                flag |= VK_SHADER_STAGE_FRAGMENT_BIT;
            }
            if ( pipelineStage.IsFlagSet( PipelineStage::Hull ) )
            {
                flag |= VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
            }
            if ( pipelineStage.IsFlagSet( PipelineStage::Domain ) )
            {
                flag |= VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
            }
            if ( pipelineStage.IsFlagSet( PipelineStage::Geometry ) )
            {
                flag |= VK_SHADER_STAGE_GEOMETRY_BIT;
            }
            if ( pipelineStage.IsFlagSet( PipelineStage::Compute ) )
            {
                flag |= VK_SHADER_STAGE_COMPUTE_BIT;
            }
            return VkShaderStageFlagBits( flag );
		}

        VkPrimitiveTopology ToVulkanPrimitiveTopology( RHI::ERHIPipelinePirmitiveTopology topology )
        {
            switch ( topology )
            {
                case EE::RHI::ERHIPipelinePirmitiveTopology::PointList: return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
                case EE::RHI::ERHIPipelinePirmitiveTopology::LineList: return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
                case EE::RHI::ERHIPipelinePirmitiveTopology::LineStrip: return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
                case EE::RHI::ERHIPipelinePirmitiveTopology::TriangleList: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
                case EE::RHI::ERHIPipelinePirmitiveTopology::TriangleStrip: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
                //case EE::RHI::ERHIPipelinePirmitiveTopology::None: return VK_PRIMITIVE_TOPOLOGY_PATCH_LIST
                default:
                break;
            }
            EE_UNREACHABLE_CODE();
            return VK_PRIMITIVE_TOPOLOGY_MAX_ENUM;
        }

        VkCullModeFlagBits ToVulkanCullModeFlags( Render::CullMode cullMode )
        {
            switch ( cullMode )
            {
                case EE::Render::CullMode::BackFace: return VK_CULL_MODE_BACK_BIT;
                case EE::Render::CullMode::FrontFace: return VK_CULL_MODE_FRONT_BIT;
                case EE::Render::CullMode::None: return VK_CULL_MODE_NONE;
                default:
                break;
            }
            EE_UNREACHABLE_CODE();
            return VK_CULL_MODE_FLAG_BITS_MAX_ENUM;
        }

        VkFrontFace ToVulkanFrontFace( Render::WindingMode windingMode )
        {
            switch ( windingMode )
            {
                case EE::Render::WindingMode::Clockwise: return VK_FRONT_FACE_CLOCKWISE;
                case EE::Render::WindingMode::CounterClockwise: return VK_FRONT_FACE_COUNTER_CLOCKWISE;
                default:
                break;
            }
            EE_UNREACHABLE_CODE();
            return VK_FRONT_FACE_MAX_ENUM;
        }

        VkPolygonMode ToVulkanPolygonMode( Render::FillMode fillMode )
        {
            switch ( fillMode )
            {
                case EE::Render::FillMode::Solid: return VK_POLYGON_MODE_FILL;
                case EE::Render::FillMode::Wireframe: return VK_POLYGON_MODE_LINE;
                default:
                break;
            }
            EE_UNREACHABLE_CODE();
            return VK_POLYGON_MODE_MAX_ENUM;
        }

        VkBlendFactor ToVulkanBlendFactor( Render::BlendValue blendValue )
        {
            switch ( blendValue )
            {
                case EE::Render::BlendValue::Zero: return VK_BLEND_FACTOR_ZERO;
                case EE::Render::BlendValue::One: return VK_BLEND_FACTOR_ONE;
                case EE::Render::BlendValue::SourceColor: return VK_BLEND_FACTOR_SRC_COLOR;
                case EE::Render::BlendValue::InverseSourceColor: return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
                case EE::Render::BlendValue::SourceAlpha: return VK_BLEND_FACTOR_SRC_ALPHA;
                case EE::Render::BlendValue::InverseSourceAlpha: return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
                case EE::Render::BlendValue::DestinationColor: return VK_BLEND_FACTOR_DST_COLOR;
                case EE::Render::BlendValue::InverseDestinationColor: return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
                case EE::Render::BlendValue::DestinationAlpha: return VK_BLEND_FACTOR_DST_ALPHA;
                case EE::Render::BlendValue::InverseDestinationAlpha: return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
                case EE::Render::BlendValue::SourceAlphaSaturated: return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
                case EE::Render::BlendValue::Source1Color: return VK_BLEND_FACTOR_SRC1_COLOR;
                case EE::Render::BlendValue::InverseSource1Color: return VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR;
                case EE::Render::BlendValue::Source1Alpha: return VK_BLEND_FACTOR_SRC1_ALPHA;
                case EE::Render::BlendValue::InverseSource1Alpha: return VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA;
                case EE::Render::BlendValue::BlendFactor: 
                case EE::Render::BlendValue::InverseBlendFactor:
                EE_UNIMPLEMENTED_FUNCTION();
                break;
                default:
                break;
            }
            EE_UNREACHABLE_CODE();
            return VK_BLEND_FACTOR_MAX_ENUM;
        }

        VkBlendOp ToVulkanBlendOp( Render::BlendOp blendOp )
        {
            switch ( blendOp )
            {
                case EE::Render::BlendOp::Add: return VK_BLEND_OP_ADD;
                case EE::Render::BlendOp::Min: return VK_BLEND_OP_MIN;
                case EE::Render::BlendOp::Max: return VK_BLEND_OP_MAX;
                case EE::Render::BlendOp::SourceMinusDestination:
                case EE::Render::BlendOp::DestinationMinusSource:
                EE_UNIMPLEMENTED_FUNCTION();
                break;
                default:
                break;
            }
            EE_UNREACHABLE_CODE();
            return VK_BLEND_OP_MAX_ENUM;
        }
    }
}

#endif