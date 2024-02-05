#pragma once

#include "Base/_Module/API.h"
#include "RenderAPI.h"
#include "Base/Resource/IResource.h"
#include "Base/Serialization/BinarySerialization.h"
#include "Base/Math/Math.h"
#include "Base/Types/BitFlags.h"

//-------------------------------------------------------------------------

namespace EE::RHI
{
    class RHIDevice;
    class RHITexture;
}

//-------------------------------------------------------------------------

namespace EE::Render
{
    // Specified the texel usage aspect.
    enum class ImageAspectFlags : uint8_t
    {
        Color = 0, // Used as color attachment.
        Depth, // Used as depth attachment.
        Stencil, // Used as stencil attachment.
        Metadata, // Used as metadata to store information about other data.
    };

    //struct TextureSubresourceRange
    //{
    //    TBitFlags<ImageAspectFlags>		    m_aspectFlags;
    //    uint32_t							m_baseMipLevel;
    //    uint32_t							m_levelCount;
    //    uint32_t							m_baseArrayLayer;
    //    uint32_t							m_layerCount;
    //};

    enum class ImageMemoryLayout
    {
        /// Choose the most optimal layout for each usage. Performs layout transitions as appropriate for the access.
        Optimal,
        /// Layout accessible by all Vulkan access types on a device - no layout transitions except for presentation
        General,
        /// Similar to `General`, but also allows presentation engines to access it - no layout transitions.
        /// Requires `VK_KHR_shared_presentable_image` to be enabled, and this can only be used for shared presentable
        /// images (i.e. single-buffered swap chains).
        GeneralAndPresentation,
    };

    enum class RawTextureDataFormat : uint8_t
    {
        Raw,
        DDS,
    };

    //-------------------------------------------------------------------------

    struct SamplerState
    {
        friend class RenderDevice;

        inline bool IsValid() const { return m_resourceHandle.IsValid(); }

        SamplerStateHandle const& GetResourceHandle() const { return m_resourceHandle; }

    public:

        TextureFiltering        m_filtering = TextureFiltering::MinMagMipLinear;
        TextureAddressMode      m_addressModeU = TextureAddressMode::Wrap;
        TextureAddressMode      m_addressModeV = TextureAddressMode::Wrap;
        TextureAddressMode      m_addressModeW = TextureAddressMode::Wrap;
        Float4                  m_borderColor = Float4(0.0f);
        uint32_t                m_maxAnisotropyValue = 1;
        float                   m_LODBias = 0;
        float                   m_minLOD = -FLT_MAX;
        float                   m_maxLOD = FLT_MAX;

    private:

        SamplerStateHandle      m_resourceHandle;
    };

    //-------------------------------------------------------------------------
    // Texture
    //-------------------------------------------------------------------------
    // Abstraction for a render texture resource

    class EE_BASE_API Texture : public Resource::IResource
    {
        friend class RenderDevice;
        friend class TextureCompiler;
        friend class TextureLoader;

        friend class RHI::RHIDevice;

        EE_RESOURCE( 'txtr', "Render Texture" );
        EE_SERIALIZE( m_format, m_rawData );

    public:

        Texture() = default;
        Texture( Int2 const& dimensions ) : m_dimensions( dimensions ) {}

        virtual bool IsValid() const override { return m_pTexture != nullptr; }
        inline Int2 const& GetDimensions() const { return m_dimensions; }

        inline bool operator==( Texture const& rhs ) const { return m_pTexture == m_pTexture; }
        inline bool operator!=( Texture const& rhs ) const { return m_pTexture != m_pTexture; }

        inline RHI::RHITexture* GetRHITexture() const { return m_pTexture; }

        // Resource Views
        //-------------------------------------------------------------------------

        //inline bool HasShaderResourceView() const { return m_shaderResourceView.IsValid(); }
        //inline ViewSRVHandle const& GetShaderResourceView() const { return m_shaderResourceView; }

        //inline bool HasUnorderedAccessView() const { return m_unorderedAccessView.IsValid(); }
        //inline ViewUAVHandle const& GetUnorderedAccessView() const { return m_unorderedAccessView; }
        //
        //inline bool HasRenderTargetView() const { return m_renderTargetView.IsValid(); }
        //inline ViewRTHandle const& GetRenderTargetView() const { return m_renderTargetView; }
        //
        //inline bool HasDepthStencilView() const { return m_depthStencilView.IsValid(); }
        //inline ViewDSHandle const& GetDepthStencilView() const { return m_depthStencilView; }

    protected:

        //TextureHandle           m_textureHandle;
        RHI::RHITexture*        m_pTexture = nullptr;
        //ViewSRVHandle           m_shaderResourceView;
        //ViewUAVHandle           m_unorderedAccessView;
        //ViewRTHandle            m_renderTargetView;
        //ViewDSHandle            m_depthStencilView;
        Int2                    m_dimensions = Int2(0, 0);
        RawTextureDataFormat    m_format;
        Blob                    m_rawData; // Temporary storage for the raw data used during installation, cleared when installation completes
    };

    //-------------------------------------------------------------------------
    // Cubemap Texture
    //-------------------------------------------------------------------------
    // Abstraction for a cubemap render texture resource
    // Needed to define a new resource type

    class EE_BASE_API CubemapTexture : public Texture
    {
        friend class RenderDevice;
        friend class TextureCompiler;
        friend class TextureLoader;

        EE_RESOURCE( 'cbmp', "Render Cubemap Texture" );
        EE_SERIALIZE( EE_SERIALIZE_BASE( Texture ) );

    public:

        CubemapTexture() = default;
    };
}