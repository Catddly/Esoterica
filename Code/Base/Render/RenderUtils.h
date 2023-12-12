#pragma once
#include "Base/_Module/API.h"
#include "Base/Esoterica.h"

//-------------------------------------------------------------------------

namespace EE::RHI { struct RHITextureBufferData; }
namespace EE::FileSystem { class Path; }

//-------------------------------------------------------------------------

namespace EE::Render
{
    class Texture;
    class RenderDevice;

    //-------------------------------------------------------------------------

    namespace Utils
    {
        EE_BASE_API bool CreateTextureFromFile( RenderDevice* pRenderDevice, FileSystem::Path const& path, Texture& texture );
        EE_BASE_API bool CreateTextureFromBase64( RenderDevice* pRenderDevice, uint8_t const* pData, size_t size, Texture& texture );

        EE_BASE_API bool FetchTextureBufferDataFromFile( RHI::RHITextureBufferData& bufferData, FileSystem::Path const& path );
        EE_BASE_API bool FetchTextureBufferDataFromBase64( RHI::RHITextureBufferData& bufferData, uint8_t const* pData, size_t size );
    }
}