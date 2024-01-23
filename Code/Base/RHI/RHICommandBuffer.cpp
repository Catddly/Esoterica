#include "RHICommandBuffer.h"

namespace EE::RHI
{
    namespace Barrier
    {
        bool IsCommonReadOnlyAccess( RenderResourceBarrierState const& access )
        {
            switch ( access )
            {
                case RenderResourceBarrierState::IndirectBuffer:
                case RenderResourceBarrierState::VertexBuffer:
                case RenderResourceBarrierState::IndexBuffer:
                case RenderResourceBarrierState::VertexShaderReadUniformBuffer:
                case RenderResourceBarrierState::VertexShaderReadSampledImageOrUniformTexelBuffer:
                case RenderResourceBarrierState::VertexShaderReadOther:
                case RenderResourceBarrierState::TessellationControlShaderReadUniformBuffer:
                case RenderResourceBarrierState::TessellationControlShaderReadSampledImageOrUniformTexelBuffer:
                case RenderResourceBarrierState::TessellationControlShaderReadOther:
                case RenderResourceBarrierState::TessellationEvaluationShaderReadUniformBuffer:
                case RenderResourceBarrierState::TessellationEvaluationShaderReadSampledImageOrUniformTexelBuffer:
                case RenderResourceBarrierState::TessellationEvaluationShaderReadOther:
                case RenderResourceBarrierState::GeometryShaderReadUniformBuffer:
                case RenderResourceBarrierState::GeometryShaderReadSampledImageOrUniformTexelBuffer:
                case RenderResourceBarrierState::GeometryShaderReadOther:
                case RenderResourceBarrierState::FragmentShaderReadUniformBuffer:
                case RenderResourceBarrierState::FragmentShaderReadSampledImageOrUniformTexelBuffer:
                case RenderResourceBarrierState::FragmentShaderReadColorInputAttachment:
                case RenderResourceBarrierState::FragmentShaderReadDepthStencilInputAttachment:
                case RenderResourceBarrierState::FragmentShaderReadOther:
                case RenderResourceBarrierState::ColorAttachmentRead:
                case RenderResourceBarrierState::DepthStencilAttachmentRead:
                case RenderResourceBarrierState::ComputeShaderReadUniformBuffer:
                case RenderResourceBarrierState::ComputeShaderReadSampledImageOrUniformTexelBuffer:
                case RenderResourceBarrierState::ComputeShaderReadOther:
                case RenderResourceBarrierState::AnyShaderReadUniformBuffer:
                case RenderResourceBarrierState::AnyShaderReadUniformBufferOrVertexBuffer:
                case RenderResourceBarrierState::AnyShaderReadSampledImageOrUniformTexelBuffer:
                case RenderResourceBarrierState::AnyShaderReadOther:
                case RenderResourceBarrierState::TransferRead:
                case RenderResourceBarrierState::HostRead:
                case RenderResourceBarrierState::Present:

                case RenderResourceBarrierState::RayTracingShaderReadSampledImageOrUniformTexelBuffer:
                case RenderResourceBarrierState::RayTracingShaderReadColorInputAttachment:
                case RenderResourceBarrierState::RayTracingShaderReadDepthStencilInputAttachment:
                case RenderResourceBarrierState::RayTracingShaderReadAccelerationStructure:
                case RenderResourceBarrierState::RayTracingShaderReadOther:

                case RenderResourceBarrierState::AccelerationStructureBuildRead:

                return true;
                break;

                default:
                return false;
                break;
            }
        }

        bool IsCommonWriteAccess( RenderResourceBarrierState const& access )
        {
            switch ( access )
            {
                case RenderResourceBarrierState::VertexShaderWrite:
                case RenderResourceBarrierState::TessellationControlShaderWrite:
                case RenderResourceBarrierState::TessellationEvaluationShaderWrite:
                case RenderResourceBarrierState::GeometryShaderWrite:
                case RenderResourceBarrierState::FragmentShaderWrite:
                case RenderResourceBarrierState::ColorAttachmentWrite:
                case RenderResourceBarrierState::DepthStencilAttachmentWrite:
                case RenderResourceBarrierState::DepthAttachmentWriteStencilReadOnly:
                case RenderResourceBarrierState::StencilAttachmentWriteDepthReadOnly:
                case RenderResourceBarrierState::ComputeShaderWrite:
                case RenderResourceBarrierState::AnyShaderWrite:
                case RenderResourceBarrierState::TransferWrite:
                case RenderResourceBarrierState::HostWrite:

                // TODO: Should we put General in write access?
                case RenderResourceBarrierState::General:

                case RenderResourceBarrierState::ColorAttachmentReadWrite:

                case RenderResourceBarrierState::AccelerationStructureBuildWrite:
                case RenderResourceBarrierState::AccelerationStructureBufferWrite:

                return true;
                break;

                default:
                return false;
                break;
            }
        }

        bool IsRasterReadOnlyAccess( RenderResourceBarrierState const& access )
        {
            switch ( access )
            {
                case RenderResourceBarrierState::VertexBuffer:
                case RenderResourceBarrierState::IndexBuffer:

                case RenderResourceBarrierState::VertexShaderReadUniformBuffer:
                case RenderResourceBarrierState::VertexShaderReadSampledImageOrUniformTexelBuffer:
                case RenderResourceBarrierState::VertexShaderReadOther:
                case RenderResourceBarrierState::TessellationControlShaderReadUniformBuffer:
                case RenderResourceBarrierState::TessellationControlShaderReadSampledImageOrUniformTexelBuffer:
                case RenderResourceBarrierState::TessellationControlShaderReadOther:
                case RenderResourceBarrierState::TessellationEvaluationShaderReadUniformBuffer:
                case RenderResourceBarrierState::TessellationEvaluationShaderReadSampledImageOrUniformTexelBuffer:
                case RenderResourceBarrierState::TessellationEvaluationShaderReadOther:
                case RenderResourceBarrierState::GeometryShaderReadUniformBuffer:
                case RenderResourceBarrierState::GeometryShaderReadSampledImageOrUniformTexelBuffer:
                case RenderResourceBarrierState::GeometryShaderReadOther:
                case RenderResourceBarrierState::FragmentShaderReadUniformBuffer:
                case RenderResourceBarrierState::FragmentShaderReadSampledImageOrUniformTexelBuffer:
                case RenderResourceBarrierState::FragmentShaderReadColorInputAttachment:
                case RenderResourceBarrierState::FragmentShaderReadDepthStencilInputAttachment:
                case RenderResourceBarrierState::FragmentShaderReadOther:
                case RenderResourceBarrierState::ColorAttachmentRead:
                case RenderResourceBarrierState::DepthStencilAttachmentRead:

                case RenderResourceBarrierState::DepthAttachmentWriteStencilReadOnly:
                case RenderResourceBarrierState::StencilAttachmentWriteDepthReadOnly:
                case RenderResourceBarrierState::ColorAttachmentReadWrite:

                case RenderResourceBarrierState::AnyShaderReadUniformBuffer:
                case RenderResourceBarrierState::AnyShaderReadUniformBufferOrVertexBuffer:
                case RenderResourceBarrierState::AnyShaderReadSampledImageOrUniformTexelBuffer:
                case RenderResourceBarrierState::AnyShaderReadOther:

                return true;
                break;

                default:
                return false;
                break;
            }
        }

        bool IsRasterWriteAccess( RenderResourceBarrierState const& access )
        {
            switch ( access )
            {
                case RenderResourceBarrierState::ColorAttachmentReadWrite:

                case RenderResourceBarrierState::VertexShaderWrite:
                case RenderResourceBarrierState::TessellationControlShaderWrite:
                case RenderResourceBarrierState::TessellationEvaluationShaderWrite:
                case RenderResourceBarrierState::GeometryShaderWrite:
                case RenderResourceBarrierState::FragmentShaderWrite:
                case RenderResourceBarrierState::ColorAttachmentWrite:
                case RenderResourceBarrierState::DepthStencilAttachmentWrite:
                case RenderResourceBarrierState::DepthAttachmentWriteStencilReadOnly:
                case RenderResourceBarrierState::StencilAttachmentWriteDepthReadOnly:

                case RenderResourceBarrierState::AnyShaderWrite:


                return true;
                break;

                default:
                return false;
                break;
            }
        }
    }

    TBitFlags<TextureAspectFlags> PixelFormatToAspectFlags( EPixelFormat format )
    {
        switch ( format )
        {
            case EE::RHI::EPixelFormat::R8UInt:
            case EE::RHI::EPixelFormat::R8Unorm:
            case EE::RHI::EPixelFormat::R32UInt:
            case EE::RHI::EPixelFormat::R32SInt:
            case EE::RHI::EPixelFormat::R16Float:
            case EE::RHI::EPixelFormat::R32Float:
            case EE::RHI::EPixelFormat::RG8UInt:
            case EE::RHI::EPixelFormat::RG8Unorm:
            case EE::RHI::EPixelFormat::RG32UInt:
            case EE::RHI::EPixelFormat::RG32SInt:
            case EE::RHI::EPixelFormat::RG16Float:
            case EE::RHI::EPixelFormat::RG32Float:
            case EE::RHI::EPixelFormat::RGB32UInt:
            case EE::RHI::EPixelFormat::RGB32SInt:
            case EE::RHI::EPixelFormat::RGB32Float:
            case EE::RHI::EPixelFormat::RGBA8UInt:
            case EE::RHI::EPixelFormat::RGBA8Unorm:
            case EE::RHI::EPixelFormat::RGBA32UInt:
            case EE::RHI::EPixelFormat::RGBA16Float:
            case EE::RHI::EPixelFormat::RGBA32Float:
            case EE::RHI::EPixelFormat::BGRA8Unorm:
            case EE::RHI::EPixelFormat::BGRA8Srgb:
            case EE::RHI::EPixelFormat::BC1Unorm:
            case EE::RHI::EPixelFormat::BC1Srgb:
            case EE::RHI::EPixelFormat::BC2Unorm:
            case EE::RHI::EPixelFormat::BC2Srgb:
            case EE::RHI::EPixelFormat::BC3Unorm:
            case EE::RHI::EPixelFormat::BC3Srgb:
            case EE::RHI::EPixelFormat::BC4Unorm:
            case EE::RHI::EPixelFormat::BC5Unorm:
            case EE::RHI::EPixelFormat::BC6HUFloat16:
            case EE::RHI::EPixelFormat::BC6HSFloat16:
            case EE::RHI::EPixelFormat::BC7Unorm:
            case EE::RHI::EPixelFormat::BC7Srgb: return TextureAspectFlags::Color;

            case EE::RHI::EPixelFormat::Depth32: return TextureAspectFlags::Depth;

            case EE::RHI::EPixelFormat::Undefined:
            default:
            break;
        }

        EE_UNREACHABLE_CODE();
        return TextureAspectFlags::Color;
    }
}