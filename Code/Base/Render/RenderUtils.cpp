#include "RenderUtils.h"
#include "RenderDevice.h"
#include "Base/Types/Arrays.h"
#include "Base/Encoding/Encoding.h"
#include "Base/ThirdParty/stb/stb_image.h"
#include "Base/RHI/Resource/RHIResourceCreationCommons.h"

#if defined(_WIN32)
#include "Base/Render/Platform/Windows/TextureLoader_Win32.h"
#endif

// TODO: move DirectX utility file
//-------------------------------------------------------------------------

namespace DirectX
{
    EE::RHI::EPixelFormat ToEnginePixelFormat( DXGI_FORMAT format )
    {
        switch ( format )
        {
            case DXGI_FORMAT_UNKNOWN: return EE::RHI::EPixelFormat::Undefined;

            case DXGI_FORMAT_R8_UINT: return EE::RHI::EPixelFormat::R8UInt;
            case DXGI_FORMAT_R8_UNORM: return EE::RHI::EPixelFormat::R8Unorm;
            case DXGI_FORMAT_R32_UINT: return EE::RHI::EPixelFormat::R32UInt;
            case DXGI_FORMAT_R32_SINT: return EE::RHI::EPixelFormat::R32SInt;
            case DXGI_FORMAT_R16_FLOAT: return EE::RHI::EPixelFormat::R16Float;
            case DXGI_FORMAT_R32_FLOAT: return EE::RHI::EPixelFormat::R32Float;
            case DXGI_FORMAT_R8G8_UINT: return EE::RHI::EPixelFormat::RG8UInt;
            case DXGI_FORMAT_R8G8_UNORM: return EE::RHI::EPixelFormat::RG8Unorm;
            case DXGI_FORMAT_R32G32_UINT: return EE::RHI::EPixelFormat::RG32UInt;
            case DXGI_FORMAT_R32G32_SINT: return EE::RHI::EPixelFormat::RG32SInt;
            case DXGI_FORMAT_R16G16_FLOAT: return EE::RHI::EPixelFormat::RG16Float;
            case DXGI_FORMAT_R32G32_FLOAT: return EE::RHI::EPixelFormat::RG32Float;
            case DXGI_FORMAT_R32G32B32_UINT: return EE::RHI::EPixelFormat::RGB32UInt;
            case DXGI_FORMAT_R32G32B32_SINT: return EE::RHI::EPixelFormat::RGB32SInt;
            case DXGI_FORMAT_R32G32B32_FLOAT: return EE::RHI::EPixelFormat::RGB32Float;
            case DXGI_FORMAT_R8G8B8A8_UINT: return EE::RHI::EPixelFormat::RGBA8UInt;
            case DXGI_FORMAT_R8G8B8A8_UNORM: return EE::RHI::EPixelFormat::RGBA8Unorm;
            case DXGI_FORMAT_R32G32B32A32_UINT: return EE::RHI::EPixelFormat::RGBA32UInt;
            case DXGI_FORMAT_R16G16B16A16_FLOAT: return EE::RHI::EPixelFormat::RGBA16Float;
            case DXGI_FORMAT_R32G32B32A32_FLOAT: return EE::RHI::EPixelFormat::RGBA32Float;

            case DXGI_FORMAT_B8G8R8A8_UNORM: return EE::RHI::EPixelFormat::BGRA8Unorm;
            case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB: return EE::RHI::EPixelFormat::BGRA8Srgb;
            case DXGI_FORMAT_D32_FLOAT: return EE::RHI::EPixelFormat::Depth32;

            case DXGI_FORMAT_BC1_UNORM: return EE::RHI::EPixelFormat::BC1Unorm;
            case DXGI_FORMAT_BC1_UNORM_SRGB: return EE::RHI::EPixelFormat::BC1Srgb;
            case DXGI_FORMAT_BC2_UNORM: return EE::RHI::EPixelFormat::BC2Unorm;
            case DXGI_FORMAT_BC2_UNORM_SRGB: return EE::RHI::EPixelFormat::BC2Srgb;
            case DXGI_FORMAT_BC3_UNORM: return EE::RHI::EPixelFormat::BC3Unorm;
            case DXGI_FORMAT_BC3_UNORM_SRGB: return EE::RHI::EPixelFormat::BC3Srgb;
            case DXGI_FORMAT_BC4_UNORM: return EE::RHI::EPixelFormat::BC4Unorm;
            case DXGI_FORMAT_BC5_UNORM: return EE::RHI::EPixelFormat::BC5Unorm;
            case DXGI_FORMAT_BC6H_UF16: return EE::RHI::EPixelFormat::BC6HUFloat16;
            case DXGI_FORMAT_BC6H_SF16: return EE::RHI::EPixelFormat::BC6HSFloat16;
            case DXGI_FORMAT_BC7_UNORM: return EE::RHI::EPixelFormat::BC7Unorm;
            case DXGI_FORMAT_BC7_UNORM_SRGB: return EE::RHI::EPixelFormat::BC7Srgb;

            case DXGI_FORMAT_B8G8R8A8_TYPELESS:
            case DXGI_FORMAT_B8G8R8X8_TYPELESS:
            case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
            case DXGI_FORMAT_R32G32B32A32_TYPELESS:
            case DXGI_FORMAT_R32G32B32A32_SINT:
            case DXGI_FORMAT_R32G32B32_TYPELESS:
            case DXGI_FORMAT_R16G16B16A16_TYPELESS:
            case DXGI_FORMAT_R16G16B16A16_UNORM:
            case DXGI_FORMAT_R16G16B16A16_UINT:
            case DXGI_FORMAT_R16G16B16A16_SNORM:
            case DXGI_FORMAT_R16G16B16A16_SINT:
            case DXGI_FORMAT_R32G32_TYPELESS:
            case DXGI_FORMAT_R32G8X24_TYPELESS:
            case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
            case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
            case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
            case DXGI_FORMAT_R10G10B10A2_TYPELESS:
            case DXGI_FORMAT_R10G10B10A2_UNORM:
            case DXGI_FORMAT_R10G10B10A2_UINT:
            case DXGI_FORMAT_R11G11B10_FLOAT:
            case DXGI_FORMAT_R8G8B8A8_TYPELESS:
            case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
            case DXGI_FORMAT_R8G8B8A8_SNORM:
            case DXGI_FORMAT_R8G8B8A8_SINT:
            case DXGI_FORMAT_R16G16_TYPELESS:
            case DXGI_FORMAT_R16G16_UNORM:
            case DXGI_FORMAT_R16G16_UINT:
            case DXGI_FORMAT_R16G16_SNORM:
            case DXGI_FORMAT_R16G16_SINT:
            case DXGI_FORMAT_R32_TYPELESS:
            case DXGI_FORMAT_R24G8_TYPELESS:
            case DXGI_FORMAT_D24_UNORM_S8_UINT:
            case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
            case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
            case DXGI_FORMAT_R8G8_TYPELESS:
            case DXGI_FORMAT_R8G8_SNORM:
            case DXGI_FORMAT_R8G8_SINT:
            case DXGI_FORMAT_R16_TYPELESS:
            case DXGI_FORMAT_D16_UNORM:
            case DXGI_FORMAT_R16_UNORM:
            case DXGI_FORMAT_R16_UINT:
            case DXGI_FORMAT_R16_SNORM:
            case DXGI_FORMAT_R16_SINT:
            case DXGI_FORMAT_R8_TYPELESS:
            case DXGI_FORMAT_R8_SNORM:
            case DXGI_FORMAT_R8_SINT:
            case DXGI_FORMAT_A8_UNORM:
            case DXGI_FORMAT_R1_UNORM:
            case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
            case DXGI_FORMAT_R8G8_B8G8_UNORM:
            case DXGI_FORMAT_G8R8_G8B8_UNORM:
            case DXGI_FORMAT_BC1_TYPELESS:
            case DXGI_FORMAT_BC2_TYPELESS:
            case DXGI_FORMAT_BC3_TYPELESS:
            case DXGI_FORMAT_BC4_TYPELESS:
            case DXGI_FORMAT_BC4_SNORM:
            case DXGI_FORMAT_BC5_TYPELESS:
            case DXGI_FORMAT_BC5_SNORM:
            case DXGI_FORMAT_B5G6R5_UNORM:
            case DXGI_FORMAT_B5G5R5A1_UNORM:
            case DXGI_FORMAT_B8G8R8X8_UNORM:
            case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
            case DXGI_FORMAT_BC6H_TYPELESS:
            case DXGI_FORMAT_BC7_TYPELESS:
            case DXGI_FORMAT_AYUV:
            case DXGI_FORMAT_Y410:
            case DXGI_FORMAT_Y416:
            case DXGI_FORMAT_NV12:
            case DXGI_FORMAT_P010:
            case DXGI_FORMAT_P016:
            case DXGI_FORMAT_420_OPAQUE:
            case DXGI_FORMAT_YUY2:
            case DXGI_FORMAT_Y210:
            case DXGI_FORMAT_Y216:
            case DXGI_FORMAT_NV11:
            case DXGI_FORMAT_AI44:
            case DXGI_FORMAT_IA44:
            case DXGI_FORMAT_P8:
            case DXGI_FORMAT_A8P8:
            case DXGI_FORMAT_B4G4R4A4_UNORM:
            case DXGI_FORMAT_P208:
            case DXGI_FORMAT_V208:
            case DXGI_FORMAT_V408:
            case DXGI_FORMAT_SAMPLER_FEEDBACK_MIN_MIP_OPAQUE:
            case DXGI_FORMAT_SAMPLER_FEEDBACK_MIP_REGION_USED_OPAQUE:
            case DXGI_FORMAT_FORCE_UINT:
            default:
            break;
        }
        EE_UNIMPLEMENTED_FUNCTION();
        return EE::RHI::EPixelFormat::Undefined;
    }
}

//-------------------------------------------------------------------------

namespace EE::Render::Utils
{
    bool CreateTextureFromFile( RenderDevice* pRenderDevice, FileSystem::Path const& path, Texture& texture )
    {
        EE_ASSERT( pRenderDevice != nullptr );
        EE_ASSERT( path.Exists() );
        EE_ASSERT( !texture.IsValid() );

        //-------------------------------------------------------------------------

        int32_t width, height, channels;
        uint8_t* pImage = stbi_load( path.c_str(), &width, &height, &channels, 4 );
        if ( pImage == nullptr )
        {
            return false;
        }

        texture = Texture( Int2( width, height ) );
        size_t const imageSize = size_t( width ) * height * channels; // 8 bits per channel
        pRenderDevice->CreateDataTexture( texture, RawTextureDataFormat::Raw, pImage, imageSize );
        stbi_image_free( pImage );

        return true;
    }

    bool CreateTextureFromBase64( RenderDevice* pRenderDevice, uint8_t const* pData, size_t size, Texture& texture )
    {
        EE_ASSERT( pRenderDevice != nullptr );
        EE_ASSERT( pData != nullptr && size > 0 );
        EE_ASSERT( !texture.IsValid() );

        //-------------------------------------------------------------------------

        Blob const decodedData = Encoding::Base64::Decode( pData, size );

        //-------------------------------------------------------------------------

        int32_t width, height, channels;
        uint8_t* pImage = stbi_load_from_memory( decodedData.data(), (int32_t) decodedData.size(), &width, &height, &channels, 4 );
        if ( pImage == nullptr )
        {
            return false;
        }

        texture = Texture( Int2( width, height ) );
        size_t const imageSize = size_t( width ) * height * channels; // 8 bits per channel
        pRenderDevice->CreateDataTexture( texture, RawTextureDataFormat::Raw, pImage, imageSize );
        stbi_image_free( pImage );

        return true;
    }

    RHI::EPixelFormat ReadDDSTextureFormat( uint8_t* pRawDDSTextureData, size_t ddsTextureSize )
    {
        #if defined(_WIN32)
        return ::DirectX::ToEnginePixelFormat( ::DirectX::ReadDDSTextureFormat( pRawDDSTextureData, ddsTextureSize ) );
        #else
        return RHI::EPixelFormat::Undefined;
        #endif
    }

    bool FetchRawDDSTextureBufferDataFromMemory( RHI::RHITextureBufferData& bufferData, uint8_t* pRawDDSTextureData, size_t ddsTextureSize )
    {
        #if defined(_WIN32)
        return ::DirectX::LoadDDSTextureFromMemory( pRawDDSTextureData, ddsTextureSize, bufferData ) == S_OK;
        #else
        #error unimplemented
        return false;
        #endif
    }

    bool FetchTextureBufferDataFromFile( RHI::RHITextureBufferData& bufferData, FileSystem::Path const& path )
    {
        EE_ASSERT( path.Exists() );

        //-------------------------------------------------------------------------

        int32_t width, height, channels;
        uint8_t* pImage = stbi_load( path.c_str(), &width, &height, &channels, 4 );
        if ( pImage == nullptr )
        {
            return false;
        }

        size_t const imageSize = size_t( width ) * height * channels; // 8 bits per channel

        bufferData.m_textureWidth = static_cast<uint32_t>( width );
        bufferData.m_textureHeight = static_cast<uint32_t>( height );
        bufferData.m_textureDepth = 1;
        bufferData.m_pixelByteLength = static_cast<uint32_t>( channels );
        bufferData.m_binary.resize( imageSize );

        memcpy( bufferData.m_binary.data(), pImage, imageSize );

        stbi_image_free( pImage );
        return true;
    }

    bool FetchTextureBufferDataFromBase64( RHI::RHITextureBufferData& bufferData, uint8_t const* pData, size_t size )
    {
        Blob const decodedData = Encoding::Base64::Decode( pData, size );

        int32_t width, height, channels;
        uint8_t* pImage = stbi_load_from_memory( decodedData.data(), (int32_t) decodedData.size(), &width, &height, &channels, 4 );
        if ( pImage == nullptr )
        {
            return false;
        }

        size_t const imageSize = size_t( width ) * height * channels; // 8 bits per channel
        
        bufferData.m_textureWidth = static_cast<uint32_t>( width );
        bufferData.m_textureHeight = static_cast<uint32_t>( height );
        bufferData.m_textureDepth = 1;
        bufferData.m_pixelByteLength = static_cast<uint32_t>( channels );
        bufferData.m_binary.resize( imageSize );

        memcpy( bufferData.m_binary.data(), pImage, imageSize );

        stbi_image_free( pImage );
        return true;
    }
}