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
            case EPixelFormat::RGBA8Unorm:
            case EPixelFormat::BGRA8Unorm:
            {
                return TextureAspectFlags::Color;
            }
            default:
            EE_UNREACHABLE_CODE();
            break;
        }

        EE_UNREACHABLE_CODE();
        return TextureAspectFlags::Color;
    }
}