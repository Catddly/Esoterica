#include "RHIResourceCreationCommons.h"

#include "Base/Math/Math.h"

namespace EE::RHI
{
    //-------------------------------------------------------------------------

    void GetPixelFormatByteSize( uint32_t width, uint32_t height, EPixelFormat format, uint32_t& outNumBytes, uint32_t& outNumBytesPerRow )
    {
        bool bIsBCtexture = false;
        uint32_t bytesPerElement = 0;

        switch ( format )
        {
            case EE::RHI::EPixelFormat::BC1Unorm:
            case EE::RHI::EPixelFormat::BC1Srgb:
            case EE::RHI::EPixelFormat::BC4Unorm:
            {
                bytesPerElement = 8;
                bIsBCtexture = true;
                break;
            }
            case EE::RHI::EPixelFormat::R8UInt:
            case EE::RHI::EPixelFormat::R8Unorm:
            {
                bytesPerElement = 1;
                break;
            }

            case EE::RHI::EPixelFormat::BC2Unorm:
            case EE::RHI::EPixelFormat::BC2Srgb:
            case EE::RHI::EPixelFormat::BC3Unorm:
            case EE::RHI::EPixelFormat::BC3Srgb:
            case EE::RHI::EPixelFormat::BC5Unorm:
            case EE::RHI::EPixelFormat::BC6HUFloat16:
            case EE::RHI::EPixelFormat::BC6HSFloat16:
            case EE::RHI::EPixelFormat::BC7Unorm:
            case EE::RHI::EPixelFormat::BC7Srgb:
            {
                bytesPerElement = 16;
                bIsBCtexture = true;
                break;
            }

            case EE::RHI::EPixelFormat::R16Float:
            case EE::RHI::EPixelFormat::RG8UInt:
            case EE::RHI::EPixelFormat::RG8Unorm:
            {
                bytesPerElement = 2;
                break;
            }
            case EE::RHI::EPixelFormat::R32UInt:
            case EE::RHI::EPixelFormat::R32SInt: 
            case EE::RHI::EPixelFormat::RG16Float:
            case EE::RHI::EPixelFormat::RGBA8UInt:
            case EE::RHI::EPixelFormat::RGBA8Unorm:
            case EE::RHI::EPixelFormat::BGRA8Srgb:
            case EE::RHI::EPixelFormat::BGRA8Unorm:
            case EE::RHI::EPixelFormat::Depth32:
            case EE::RHI::EPixelFormat::R32Float:
            {
                bytesPerElement = 4;
                break;
            }
            case EE::RHI::EPixelFormat::RG32UInt:
            case EE::RHI::EPixelFormat::RG32SInt:
            case EE::RHI::EPixelFormat::RGBA16Float:
            case EE::RHI::EPixelFormat::RG32Float:
            {
                bytesPerElement = 8;
                break;
            }
            case EE::RHI::EPixelFormat::RGB32UInt:
            case EE::RHI::EPixelFormat::RGB32SInt:
            case EE::RHI::EPixelFormat::RGB32Float:
            {
                bytesPerElement = 12;
                break;
            }
            case EE::RHI::EPixelFormat::RGBA32UInt:
            case EE::RHI::EPixelFormat::RGBA32Float:
            {
                bytesPerElement = 16;
                break;
            }

            case EE::RHI::EPixelFormat::Undefined:
            default:
            break;
        }

        if ( bIsBCtexture )
        {
            uint32_t numBlocksWide = 0;
            if ( width > 0 )
            {
                // See https://learn.microsoft.com/en-us/windows/win32/direct3ddds/dx-graphics-dds-pguide
                numBlocksWide = Math::Max( 1u, ( width + 3 ) / 4 );
            }
            uint32_t numBlocksHigh = 0;
            if ( height > 0 )
            {
                // See https://learn.microsoft.com/en-us/windows/win32/direct3ddds/dx-graphics-dds-pguide
                numBlocksHigh = Math::Max( 1u, ( height + 3 ) / 4 );
            }
            outNumBytesPerRow = numBlocksWide * bytesPerElement;
            outNumBytes = outNumBytesPerRow * numBlocksHigh;
        }
        else
        {
            outNumBytesPerRow = width * bytesPerElement;
            outNumBytes = outNumBytesPerRow * height;
        }
    }

    //-------------------------------------------------------------------------

    bool RHITextureBufferData::CanBeUsedBy( RHITextureCreateDesc const& textureCreateDesc ) const
    {
        return textureCreateDesc.m_width == m_textureWidth
            && textureCreateDesc.m_height == m_textureHeight
            && textureCreateDesc.m_depth == m_textureDepth;
    }

    //-------------------------------------------------------------------------

    RHITextureCreateDesc RHITextureCreateDesc::GetDefault()
    {
        RHITextureCreateDesc desc = {};
        desc.m_width = 0;
        desc.m_height = 0;
        desc.m_depth = 0;

        desc.m_array = 1;
        desc.m_mipmap = 1;

        // we can infer usage by its resource barrier type, so user do not need to explicitly fill in here,
        // but we still give user choice to add usage flags if needed.
        desc.m_usage = ETextureUsage::Color;
        desc.m_tiling = ETextureMemoryTiling::Optimal;
        desc.m_format = EPixelFormat::RGBA8Unorm;
        desc.m_sample = ESampleCount::SC1;
        desc.m_type = ETextureType::T2D;
        desc.m_memoryUsage = ERenderResourceMemoryUsage::GPUOnly;
        return desc;
    }

    RHITextureCreateDesc RHITextureCreateDesc::New1D( uint32_t width, EPixelFormat format )
    {
        auto desc = RHITextureCreateDesc::GetDefault();
        desc.m_width = width;
        desc.m_height = 1;
        desc.m_depth = 1;

        desc.m_format = format;
        desc.m_type = ETextureType::T1D;
        return desc;
    }

    RHITextureCreateDesc RHITextureCreateDesc::New1DArray( uint32_t width, EPixelFormat format, uint32_t array )
    {
        auto desc = RHITextureCreateDesc::GetDefault();
        desc.m_width = width;
        desc.m_height = 1;
        desc.m_depth = 1;

        desc.m_format = format;
        desc.m_type = ETextureType::T1DArray;
        desc.m_array = array;
        return desc;
    }

    RHITextureCreateDesc RHITextureCreateDesc::New2D( uint32_t width, uint32_t height, EPixelFormat format )
    {
        auto desc = RHITextureCreateDesc::GetDefault();
        desc.m_width = width;
        desc.m_height = height;
        desc.m_depth = 1;

        desc.m_format = format;
        desc.m_type = ETextureType::T2D;
        return desc;
    }

    RHITextureCreateDesc RHITextureCreateDesc::New2DArray( uint32_t width, uint32_t height, EPixelFormat format, uint32_t array )
    {
        auto desc = RHITextureCreateDesc::GetDefault();
        desc.m_width = width;
        desc.m_height = height;
        desc.m_depth = 1;

        desc.m_format = format;
        desc.m_type = ETextureType::T2DArray;
        desc.m_array = array;
        return desc;
    }

    RHITextureCreateDesc RHITextureCreateDesc::New3D( uint32_t width, uint32_t height, uint32_t depth, EPixelFormat format )
    {
        auto desc = RHITextureCreateDesc::GetDefault();
        desc.m_width = width;
        desc.m_height = height;
        desc.m_depth = depth;

        desc.m_format = format;
        desc.m_type = ETextureType::T3D;
        return desc;
    }

    RHITextureCreateDesc RHITextureCreateDesc::NewCubemap( uint32_t width, EPixelFormat format )
    {
        auto desc = RHITextureCreateDesc::GetDefault();
        desc.m_width = width;
        desc.m_height = width;
        desc.m_depth = 1;

        desc.m_format = format;
        desc.m_type = ETextureType::TCubemap;
        desc.m_array = 6;
        desc.m_flag = ETextureCreateFlag::CubeCompatible;
        return desc;
    }

    RHITextureCreateDesc RHITextureCreateDesc::NewInitData( RHITextureBufferData texBufferData, EPixelFormat format )
    {
        EE_ASSERT( texBufferData.HasValidData() );
        // TODO: assertion format must be compatitable with texBufferData

        auto desc = RHITextureCreateDesc::GetDefault();
        desc.m_width = texBufferData.m_textureWidth;
        desc.m_height = texBufferData.m_textureHeight;
        desc.m_depth = texBufferData.m_textureDepth;
        desc.m_mipmap = texBufferData.m_totalMipmaps;

        desc.m_format = format;
        desc.m_type = desc.m_depth == 1 ? ETextureType::T2D : ETextureType::T3D;
        desc.m_bufferData = std::move( texBufferData );
        return desc;
    }

	bool RHITextureCreateDesc::IsValid() const
	{
        bool isValid = m_height != 0
            && m_width != 0
            && m_depth != 0
            && m_mipmap != 0
            && m_array != 0;

        if ( !isValid )
        {
            return isValid;
        }

        switch ( m_type )
        {
            case ETextureType::T1D:
            break;
            case ETextureType::T1DArray:
            break;
            case ETextureType::T2D:
            break;
            case ETextureType::T2DArray:
            break;
            case ETextureType::T3D:
            break;
            case ETextureType::TCubemap:
            {
                return m_array == 6
                    && m_width == m_height
                    && m_depth == 1
                    && m_flag.IsFlagSet( ETextureCreateFlag::CubeCompatible );
            }
            case ETextureType::TCubemapArray:
            {
                return m_array >= 6
                    && m_width == m_height
                    && m_depth == 1
                    && m_flag.IsFlagSet( ETextureCreateFlag::CubeCompatible );
            }
            break;
            default:
            break;
        }

        isValid = !(m_memoryUsage == ERenderResourceMemoryUsage::CPUCopy
            || m_memoryUsage == ERenderResourceMemoryUsage::CPUCopy
            || m_memoryUsage == ERenderResourceMemoryUsage::CPUToGPU
            || m_memoryUsage == ERenderResourceMemoryUsage::GPUToCPU);

        return isValid;
	}

    //-------------------------------------------------------------------------

    bool RHIBufferCreateDesc::IsValid() const
    {
        return m_desireSize != 0
            && m_usage.IsAnyFlagSet();
    }

    RHIBufferCreateDesc RHIBufferCreateDesc::NewSize( uint32_t sizeInByte )
    {
        EE_ASSERT( sizeInByte > 0 );

        RHIBufferCreateDesc bufferDesc = {};
        bufferDesc.m_desireSize = sizeInByte;
        bufferDesc.m_usage.SetFlag(EBufferUsage::Uniform);
        bufferDesc.m_memoryUsage = ERenderResourceMemoryUsage::CPUToGPU;
        return bufferDesc;
    }

    RHIBufferCreateDesc RHIBufferCreateDesc::NewAlignedSize( uint32_t sizeInByte, uint32_t alignment )
    {
        EE_ASSERT( sizeInByte > 0 && alignment >= 2 && Math::IsPowerOfTwo( alignment ) );

        uint32_t aligned = Math::MinValueAlignTo( sizeInByte, alignment );

        RHIBufferCreateDesc bufferDesc = {};
        bufferDesc.m_desireSize = aligned;
        bufferDesc.m_usage.SetFlag( EBufferUsage::Uniform );
        bufferDesc.m_memoryUsage = ERenderResourceMemoryUsage::CPUToGPU;
        return bufferDesc;
    }

    RHIBufferCreateDesc RHIBufferCreateDesc::NewDeviceAddressable( uint32_t sizeInByte )
    {
        EE_ASSERT( sizeInByte > 0 );

        RHIBufferCreateDesc bufferDesc = {};
        bufferDesc.m_desireSize = sizeInByte;
        bufferDesc.m_usage.SetFlag( EBufferUsage::ShaderDeviceAddress );
        bufferDesc.m_memoryUsage = ERenderResourceMemoryUsage::GPUOnly;
        return bufferDesc;
    }

    RHIBufferCreateDesc RHIBufferCreateDesc::NewVertexBuffer( uint32_t sizeInByte )
    {
        EE_ASSERT( sizeInByte > 0 );

        RHIBufferCreateDesc bufferDesc = {};
        bufferDesc.m_desireSize = sizeInByte;
        bufferDesc.m_usage.SetFlag( EBufferUsage::Vertex );
        bufferDesc.m_memoryUsage = ERenderResourceMemoryUsage::GPUOnly;
        return bufferDesc;
    }

    RHIBufferCreateDesc RHIBufferCreateDesc::NewIndexBuffer( uint32_t sizeInByte )
    {
        EE_ASSERT( sizeInByte > 0 );

        RHIBufferCreateDesc bufferDesc = {};
        bufferDesc.m_desireSize = sizeInByte;
        bufferDesc.m_usage.SetFlag( EBufferUsage::Index );
        bufferDesc.m_memoryUsage = ERenderResourceMemoryUsage::GPUOnly;
        return bufferDesc;
    }

    RHIBufferCreateDesc RHIBufferCreateDesc::NewUniformBuffer( uint32_t sizeInByte )
    {
        EE_ASSERT( sizeInByte > 0 );

        RHIBufferCreateDesc bufferDesc = {};
        bufferDesc.m_desireSize = sizeInByte;
        bufferDesc.m_usage.SetFlag( EBufferUsage::Uniform );
        bufferDesc.m_memoryUsage = ERenderResourceMemoryUsage::GPUOnly;
        return bufferDesc;
    }

    RHIBufferCreateDesc RHIBufferCreateDesc::NewStorageBuffer( uint32_t sizeInByte )
    {
        EE_ASSERT( sizeInByte > 0 );

        RHIBufferCreateDesc bufferDesc = {};
        bufferDesc.m_desireSize = sizeInByte;
        bufferDesc.m_usage.SetFlag( EBufferUsage::Storage );
        bufferDesc.m_memoryUsage = ERenderResourceMemoryUsage::GPUOnly;
        return bufferDesc;
    }

    //-------------------------------------------------------------------------

    bool RHIRenderPassCreateDesc::IsValid() const
    {
        return !m_colorAttachments.empty()
            || ( m_colorAttachments.empty() && m_depthAttachment.has_value() );
    }

    //-------------------------------------------------------------------------

    RHIPipelineShader::RHIPipelineShader( ResourcePath shaderPath, String entryName )
        : m_entryName( entryName )
    {
        SetShaderPath( shaderPath );
        EE_ASSERT( m_shaderPath.IsValid() && m_shaderPath.IsFile() );
    }
}