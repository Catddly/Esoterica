#pragma once

#include "Base/_Module/API.h"
#include "Base/Types/Arrays.h"
#include "Base/Types/BitFlags.h"
#include "Base/Types/Set.h"
#include "Base/Types/Optional.h"
#include "Base/Encoding/Hash.h"
#include "Base/Render/RenderAPI.h"
#include "Base/Render/RenderShader.h"
#include "Base/Resource/ResourcePath.h"
#include "Base/Resource/ResourceTypeID.h"

#include <numeric>

namespace EE::RHI
{
    enum class ESampleCount : uint8_t
    {
        SC1 = 0,
        SC2,
        SC4,
        SC8,
        SC16,
        SC32,
        SC64,
    };

    //-------------------------------------------------------------------------

    enum class ERenderResourceMemoryUsage : uint8_t
    {
        CPUToGPU,
        GPUToCPU,
        CPUOnly,
        GPUOnly,
        CPUCopy,
        GPULazily,
    };

    enum class ERenderResourceMemoryFlag : uint8_t
    {
        // For large memory allocation.
        DedicatedMemory = 0,
        // Mapping GPU memory to CPU persistently.
        PersistentMapping,
    };

    //-------------------------------------------------------------------------

    enum class EPixelFormat : uint8_t
    {
        RGBA8Unorm,
        BGRA8Unorm,

        RGBA32UInt,

        Depth32,

        Undefined
    };

    uint32_t GetPixelFormatByteSize( EPixelFormat format );

    enum class ETextureType : uint8_t
    {
        T1D,
        T1DArray,
        T2D,
        T2DArray,
        T3D,
        TCubemap,
        TCubemapArray
    };

    enum class ETextureUsage : uint8_t
    {
        TransferSrc = 0,
        TransferDst,
        Sampled,
        Storage,
        Color,
        DepthStencil,
        Transient,
        Input,
    };

    enum class ETextureLayout : uint8_t
    {
        Undefined,
        General,
        ColorOptimal,
        DepthStencilOptimal,
        DepthStencilReadOnlyOptimal,
        ShaderReadOnlyOptimal,
        TransferSrcOptimal,
        TransferDstOptimal,
        Preinitialized
    };

    enum class ETextureMemoryTiling : uint8_t
    {
        Optimal,
        Linear,
    };

    enum class ETextureCreateFlag : uint8_t
    {
        CubeCompatible = 0,
    };

    struct RHITextureCreateDesc;

    struct RHITextureBufferData
    {
        Blob                m_binary;
        uint32_t            m_textureWidth = 0;
        uint32_t            m_textureHeight = 0;
        uint32_t            m_textureDepth = 1;
        uint32_t            m_pixelByteLength = 0;

        inline bool HasValidData() const
        {
            return !m_binary.empty()
                && m_binary.size() == m_textureWidth * m_textureHeight * m_textureDepth * m_pixelByteLength;
        }

        // Return whether this texture buffer data can be used(initialized) within a texture. 
        bool CanBeUsedBy( RHITextureCreateDesc const& textureCreateDesc ) const;
    };

    struct RHITextureCreateDesc
    {
    public:

        static RHITextureCreateDesc New1D( uint32_t width, EPixelFormat format );
        static RHITextureCreateDesc New1DArray( uint32_t width, EPixelFormat format, uint32_t array );
        static RHITextureCreateDesc New2D( uint32_t width, uint32_t height, EPixelFormat format );
        static RHITextureCreateDesc New2DArray( uint32_t width, uint32_t height, EPixelFormat format, uint32_t array );
        static RHITextureCreateDesc New3D( uint32_t width, uint32_t height, uint32_t depth, EPixelFormat format );
        static RHITextureCreateDesc NewCubemap( uint32_t width, EPixelFormat format );
        static RHITextureCreateDesc NewInitData( RHITextureBufferData texBufferData, EPixelFormat format );

        // Pass by value, accept both lvalue and rvalue
        void WithInitialData( RHITextureBufferData initBufferData )
        {
            if ( initBufferData.HasValidData() && initBufferData.CanBeUsedBy( *this ) )
            {
                m_bufferData = initBufferData;
            }
        }

        bool IsValid() const;

        size_t GetHash() const
        {
            size_t hash = 0;
            Hash::HashCombine( hash, m_width );
            Hash::HashCombine( hash, m_height );
            Hash::HashCombine( hash, m_depth );

            Hash::HashCombine( hash, m_array );
            Hash::HashCombine( hash, m_mipmap );

            Hash::HashCombine( hash, m_format );
            Hash::HashCombine( hash, m_usage.Get() );
            Hash::HashCombine( hash, m_tiling );
            Hash::HashCombine( hash, m_sample.Get() );
            Hash::HashCombine( hash, m_type );
            Hash::HashCombine( hash, m_flag.Get() );

            Hash::HashCombine( hash, m_memoryUsage );
            return hash;
        }

        friend bool operator==( RHITextureCreateDesc const& lhs, RHITextureCreateDesc const& rhs )
        {
            return eastl::tie( lhs.m_width, lhs.m_height, lhs.m_depth,
                               lhs.m_array, lhs.m_mipmap, lhs.m_format,
                               lhs.m_usage, lhs.m_tiling, lhs.m_sample,
                               lhs.m_type, lhs.m_flag, lhs.m_memoryUsage, lhs.m_memoryFlag )
                == eastl::tie( rhs.m_width, rhs.m_height, rhs.m_depth,
                               rhs.m_array, rhs.m_mipmap, rhs.m_format,
                               rhs.m_usage, rhs.m_tiling, rhs.m_sample,
                               rhs.m_type, rhs.m_flag, rhs.m_memoryUsage, rhs.m_memoryFlag );
        }

        friend bool operator!=( RHITextureCreateDesc const& lhs, RHITextureCreateDesc const& rhs )
        {
            return eastl::tie( lhs.m_width, lhs.m_height, lhs.m_depth,
                               lhs.m_array, lhs.m_mipmap, lhs.m_format,
                               lhs.m_usage, lhs.m_tiling, lhs.m_sample,
                               lhs.m_type, lhs.m_flag, lhs.m_memoryUsage, lhs.m_memoryFlag )
                != eastl::tie( rhs.m_width, rhs.m_height, rhs.m_depth,
                               rhs.m_array, rhs.m_mipmap, rhs.m_format,
                               rhs.m_usage, rhs.m_tiling, rhs.m_sample,
                               rhs.m_type, rhs.m_flag, rhs.m_memoryUsage, rhs.m_memoryFlag );
        }

    private:

        static RHITextureCreateDesc GetDefault();

    public:

        uint32_t						        m_width;
        uint32_t						        m_height;
        uint32_t						        m_depth;

        uint32_t						        m_array;
        uint16_t						        m_mipmap;

        EPixelFormat                            m_format;
        TBitFlags<ETextureUsage>                m_usage;
        ETextureMemoryTiling			        m_tiling;
        TBitFlags<ESampleCount>	                m_sample;
        ETextureType                            m_type;
        TBitFlags<ETextureCreateFlag>           m_flag;

        // Actually allocated size in byte. (May be due to alignment of certain type of texture usage)
        uint32_t                                m_allocatedSize;
        ERenderResourceMemoryUsage              m_memoryUsage;
        TBitFlags<ERenderResourceMemoryFlag>    m_memoryFlag;

        RHITextureBufferData                    m_bufferData;
    };

    enum class ETextureViewType : uint8_t
    {
        TV1D,
        TV2D,
        TV3D,
        TVCubemap,
        TV1DArray,
        TV2DArray,
        TVCubemapArray,
    };

    enum class ETextureViewAspect : uint8_t
    {
        Color = 0,
        Depth,
        Stencil,
        Metadata,
        Plane0,
        Plane1,
        Plane2,
        None
    };

    struct RHITextureViewCreateDesc
    {
        // If this is not set, infer texture view type from ETextureType in RHITextureCreateDesc.
        TOptional<ETextureViewType>                   m_viewType = {};
        // If this is not set, infer texture format from EPixelFormat in RHITextureCreateDesc.
        TOptional<EPixelFormat>                       m_format = {};
        // If this is not set, infer texture view aspect from TBitFlags<ETextureUsage> in RHITextureCreateDesc.
        TBitFlags<ETextureViewAspect>                 m_viewAspect = TBitFlags<ETextureViewAspect>( ETextureViewAspect::Color );
        // If this is not set, the level count will always be the left mipmaps.
        // (i.e. m_levelCount = RHITextureCreateDesc::m_mipmap - m_baseMipmap)
        TOptional<uint16_t>                           m_levelCount = {};

        uint16_t                                      m_baseMipmap = 0;

        friend bool operator==( RHITextureViewCreateDesc const& lhs, RHITextureViewCreateDesc const& rhs )
        {
            return lhs.m_viewType == rhs.m_viewType
                && lhs.m_format == rhs.m_format
                && lhs.m_viewAspect == rhs.m_viewAspect
                && lhs.m_levelCount == rhs.m_levelCount
                && lhs.m_baseMipmap == rhs.m_baseMipmap;
        }
    };

    //-------------------------------------------------------------------------

    enum class EBufferUsage : uint8_t
    {
        TransferSrc = 0,
        TransferDst,
        UniformTexel,
        StorageTexel,
        Uniform,
        Storage,
        Index,
        Vertex,
        Indirect,
        ShaderDeviceAddress,
    };

    struct EE_BASE_API RHIBufferCreateDesc
    {
    public:

        static RHIBufferCreateDesc NewSize( uint32_t sizeInByte );
        static RHIBufferCreateDesc NewAlignedSize( uint32_t sizeInByte, uint32_t alignment );
        static RHIBufferCreateDesc NewDeviceAddressable( uint32_t sizeInByte );
        static RHIBufferCreateDesc NewVertexBuffer( uint32_t sizeInByte );
        static RHIBufferCreateDesc NewIndexBuffer( uint32_t sizeInByte );
        static RHIBufferCreateDesc NewUniformBuffer( uint32_t sizeInByte );
        static RHIBufferCreateDesc NewStorageBuffer( uint32_t sizeInByte );

        bool IsValid() const;

        size_t GetHash() const
        {
            size_t hash = 0;
            Hash::HashCombine( hash, m_desireSize );
            Hash::HashCombine( hash, m_usage.Get() );
            Hash::HashCombine( hash, m_memoryUsage );
            Hash::HashCombine( hash, m_memoryFlag.Get() );
            return hash;
        }

        friend bool operator==( RHIBufferCreateDesc const& lhs, RHIBufferCreateDesc const& rhs )
        {
            return eastl::tie( lhs.m_desireSize, lhs.m_usage, lhs.m_memoryUsage, lhs.m_memoryFlag )
                == eastl::tie( rhs.m_desireSize, rhs.m_usage, rhs.m_memoryUsage, rhs.m_memoryFlag );
        }

        friend bool operator!=( RHIBufferCreateDesc const& lhs, RHIBufferCreateDesc const& rhs )
        {
            return eastl::tie( lhs.m_desireSize, lhs.m_usage, lhs.m_memoryUsage, lhs.m_memoryFlag )
                != eastl::tie( rhs.m_desireSize, rhs.m_usage, rhs.m_memoryUsage, rhs.m_memoryFlag );
        }

    public:

        // TODO: alignment
        // User requested size in byte.
        uint32_t                                m_desireSize;
        // The alignment of allocated memory in the GPU.
        TBitFlags<EBufferUsage>                 m_usage;

        // Actually allocated size in byte. (May be due to alignment of certain type of buffer usage)
        uint32_t                                m_allocatedSize;
        ERenderResourceMemoryUsage              m_memoryUsage;
        TBitFlags<ERenderResourceMemoryFlag>    m_memoryFlag;
    };

    //-------------------------------------------------------------------------

    enum class ERenderPassAttachmentLoadOp : uint8_t
    {
        Load,
        Clear,
        DontCare,
    };

    enum class ERenderPassAttachmentStoreOp : uint8_t
    {
        Store,
        DontCare,
    };

    struct RHIRenderPassAttachmentDesc
    {
        static RHIRenderPassAttachmentDesc TrivialColor( EPixelFormat pixelFormat )
        {
            RHIRenderPassAttachmentDesc desc;
            desc.m_pixelFormat = pixelFormat;
            desc.m_loadOp = ERenderPassAttachmentLoadOp::Load;
            desc.m_storeOp = ERenderPassAttachmentStoreOp::Store;
            desc.m_stencilLoadOp = ERenderPassAttachmentLoadOp::DontCare;
            desc.m_stencilStoreOp = ERenderPassAttachmentStoreOp::DontCare;
            desc.m_sample = ESampleCount::SC1;
            return desc;
        }

        static RHIRenderPassAttachmentDesc UselessInput( EPixelFormat pixelFormat )
        {
            RHIRenderPassAttachmentDesc desc = TrivialColor( pixelFormat );
            desc.m_loadOp = ERenderPassAttachmentLoadOp::DontCare;
            return desc;
        }

        static RHIRenderPassAttachmentDesc ClearInput( EPixelFormat pixelFormat )
        {
            RHIRenderPassAttachmentDesc desc = TrivialColor( pixelFormat );
            desc.m_loadOp = ERenderPassAttachmentLoadOp::Clear;
            return desc;
        }

        static RHIRenderPassAttachmentDesc DiscardOutput( EPixelFormat pixelFormat )
        {
            RHIRenderPassAttachmentDesc desc = TrivialColor( pixelFormat );
            desc.m_storeOp = ERenderPassAttachmentStoreOp::DontCare;
            return desc;
        }

        EPixelFormat                                    m_pixelFormat;
        ERenderPassAttachmentLoadOp                     m_loadOp;
        ERenderPassAttachmentStoreOp                    m_storeOp;
        ERenderPassAttachmentLoadOp                     m_stencilLoadOp;
        ERenderPassAttachmentStoreOp                    m_stencilStoreOp;
        ESampleCount                                    m_sample = ESampleCount::SC1;
    };

    struct RHIRenderPassCreateDesc
    {
        // TODO: do some code clear up. This constants should be placed in a single setting file.
        static constexpr size_t NumMaxColorAttachmentCount = 9;
        static constexpr size_t NumMaxAttachmentCount = 10;

        bool IsValid() const;

        TFixedVector<RHIRenderPassAttachmentDesc, NumMaxColorAttachmentCount>       m_colorAttachments;
        TOptional<RHIRenderPassAttachmentDesc>                                      m_depthAttachment;
    };

    //-------------------------------------------------------------------------

    // TODO: Is this redundant? Maybe we can infer this from RHITextureCreateDesc
    // Necessary attachment flags to determine a unique framebuffer attachment.
    struct RHIFramebufferAttachmentHash
    {
        TBitFlags<ETextureUsage>                   m_usage;
        TBitFlags<ETextureCreateFlag>              m_createFlags;

        RHIFramebufferAttachmentHash( TBitFlags<ETextureUsage> usage, TBitFlags<ETextureCreateFlag> createFlags )
            : m_usage( usage ), m_createFlags( createFlags )
        {}

        inline friend bool operator==( RHIFramebufferAttachmentHash const& lhs, RHIFramebufferAttachmentHash const& rhs )
        {
            return lhs.m_createFlags == rhs.m_createFlags
                && lhs.m_usage == rhs.m_usage;
        }

        inline friend bool operator!=( RHIFramebufferAttachmentHash const& lhs, RHIFramebufferAttachmentHash const& rhs )
        {
            return !operator==( lhs, rhs );
        }
    };

    // Key to determine a unique framebuffer.
    struct RHIFramebufferCacheKey
    {
        using RHIFrameBufferAttachmentHashes = TFixedVector<RHIFramebufferAttachmentHash, RHIRenderPassCreateDesc::NumMaxAttachmentCount>;

        uint32_t                                    m_extentX;
        uint32_t                                    m_extentY;
        RHIFrameBufferAttachmentHashes              m_attachmentHashs;

        friend bool operator==( RHIFramebufferCacheKey const& lhs, RHIFramebufferCacheKey const& rhs )
        {
            bool result = lhs.m_extentX == rhs.m_extentX
                && lhs.m_extentY == rhs.m_extentY
                && lhs.m_attachmentHashs.size() == rhs.m_attachmentHashs.size();

            if ( !result )
            {
                return result;
            }

            for ( uint32_t i = 0; i < lhs.m_attachmentHashs.size(); ++i )
            {
                if ( lhs.m_attachmentHashs[i] != rhs.m_attachmentHashs[i] )
                {
                    return false;
                }
            }

            return true;
        }
    };

    //-------------------------------------------------------------------------

    enum class RHIPipelineType : uint8_t
    {
        Raster,
        Compute,
        Transfer
    };

    struct RHIPipelineRasterizerState
    {
        static RHIPipelineRasterizerState NoCulling()
        {
            RHIPipelineRasterizerState defaultState = {};
            defaultState.m_cullMode = Render::CullMode::None;
            return defaultState;
        }

        Render::FillMode                m_fillMode = Render::FillMode::Solid;
        Render::CullMode                m_cullMode = Render::CullMode::BackFace;
        Render::WindingMode             m_WindingMode = Render::WindingMode::CounterClockwise;
        bool                            m_enableScissorCulling = false;
    };

    struct RHIPipelineBlendState
    {
        static RHIPipelineBlendState ColorAdditiveAlpha()
        {
            RHIPipelineBlendState defaultState = {};
            defaultState.m_blendEnable = true;
            defaultState.m_srcValue = Render::BlendValue::SourceAlpha;
            defaultState.m_dstValue = Render::BlendValue::InverseSourceAlpha;
            defaultState.m_blendOp = Render::BlendOp::Add;
            defaultState.m_srcAlphaValue = Render::BlendValue::One;
            defaultState.m_dstAlphaValue = Render::BlendValue::InverseSourceAlpha;
            defaultState.m_blendOpAlpha = Render::BlendOp::Add;
            return defaultState;
        }

        Render::BlendValue              m_srcValue = Render::BlendValue::One;
        Render::BlendValue              m_dstValue = Render::BlendValue::Zero;
        Render::BlendOp                 m_blendOp = Render::BlendOp::Add;
        Render::BlendValue              m_srcAlphaValue = Render::BlendValue::One;
        Render::BlendValue              m_dstAlphaValue = Render::BlendValue::Zero;
        Render::BlendOp                 m_blendOpAlpha = Render::BlendOp::Add;
        bool                            m_blendEnable = false;
    };

    enum class ERHIPipelinePirmitiveTopology : uint8_t
    {
        PointList = 0,
        LineList,
        LineStrip,
        TriangleList,
        TriangleStrip,

        //None,
    };

    struct EE_BASE_API RHIPipelineShader
    {
        RHIPipelineShader( ResourcePath shaderPath, String entryName = "main" );

        inline RHIPipelineShader& SetShaderPath( ResourcePath shaderPath )
        {
            EE_ASSERT( shaderPath.IsValid() && shaderPath.IsFile() );
            char const* pExtension = shaderPath.GetExtension();
            EE_ASSERT( strlen(pExtension) == 4 ); // Extension is a TypeFourCC

            if ( ResourceTypeID( pExtension ) == Render::VertexShader::GetStaticResourceTypeID() )
            {
                this->m_stage = Render::PipelineStage::Vertex;
            }
            else if ( ResourceTypeID( pExtension ) == Render::PixelShader::GetStaticResourceTypeID() )
            {
                this->m_stage = Render::PipelineStage::Pixel;
            }
            else if( ResourceTypeID( pExtension ) == Render::GeometryShader::GetStaticResourceTypeID() )
            {
                this->m_stage = Render::PipelineStage::Geometry;
            }
            else if ( ResourceTypeID( pExtension ) == Render::ComputeShader::GetStaticResourceTypeID() )
            {
                this->m_stage = Render::PipelineStage::Compute;
            }

            this->m_shaderPath = shaderPath;
            return *this;
        }

        inline RHIPipelineShader& SetEntryName( String entryName )
        {
            EE_ASSERT( !entryName.empty() );
            this->m_entryName = entryName;
            return *this;
        }

        inline bool IsValid() const
        {
            return m_stage != Render::PipelineStage::None && m_shaderPath.IsValid() && m_shaderPath.IsFile() && !m_entryName.empty();
        }

    public:

        friend bool operator<( RHIPipelineShader const& lhs, RHIPipelineShader const& rhs )
        {
            return static_cast<uint8_t>( lhs.m_stage ) < static_cast<uint8_t>( rhs.m_stage );
        }

    public:

        Render::PipelineStage   m_stage = Render::PipelineStage::Vertex;
        ResourcePath            m_shaderPath;
        String                  m_entryName = "main";
    };

    class RHIRenderPass;

    struct RHIRasterPipelineStateCreateDesc
    {
        inline RHIRasterPipelineStateCreateDesc& AddShader( RHIPipelineShader&& pipelineShader )
        {
            EE_ASSERT( pipelineShader.IsValid() );

            for ( RHIPipelineShader const& ps : this->m_pipelineShaders )
            {
                if ( pipelineShader.m_stage == ps.m_stage )
                {
                    // Do override
                    this->m_pipelineShaders.erase( ps );
                    this->m_pipelineShaders.emplace( std::move( pipelineShader ) );
                    return *this;
                }
            }

            this->m_pipelineShaders.emplace( std::move( pipelineShader ) );
            return *this;
        }

        inline RHIRasterPipelineStateCreateDesc& AddShader( ResourcePath shaderPath, String entryName = "main" )
        {
            RHIPipelineShader pipelineShader( shaderPath, entryName );
            EE_ASSERT( pipelineShader.IsValid() );

            for ( RHIPipelineShader const& ps : this->m_pipelineShaders )
            {
                if ( pipelineShader.m_stage == ps.m_stage )
                {
                    // Do override
                    this->m_pipelineShaders.erase( ps );
                    this->m_pipelineShaders.emplace( std::move( pipelineShader ) );
                    return *this;
                }
            }

            this->m_pipelineShaders.emplace( std::move( pipelineShader ) );
            return *this;
        }

        inline RHIRasterPipelineStateCreateDesc& SetRasterizerState( RHIPipelineRasterizerState rasterizerState )
        {
            this->m_rasterizerState = rasterizerState;
            return *this;
        }

        inline RHIRasterPipelineStateCreateDesc& SetBlendState( RHIPipelineBlendState blendState )
        {
            this->m_blendState = blendState;
            return *this;
        }

        inline RHIRasterPipelineStateCreateDesc& SetTriangleTopology( ERHIPipelinePirmitiveTopology topology )
        {
            this->m_primitiveTopology = topology;
            return *this;
        }

        inline RHIRasterPipelineStateCreateDesc& SetRenderPass( RHIRenderPass* pRenderPass )
        {
            m_pRenderpass = pRenderPass;
        }

        inline RHIRasterPipelineStateCreateDesc& DepthTest( bool enable )
        {
            this->m_enableDepthTest = enable;
            return *this;
        }

        inline RHIRasterPipelineStateCreateDesc& DepthWrite( bool enable )
        {
            this->m_enableDepthWrite = enable;
            return *this;
        }

        inline bool IsValid() const
        {
            return !m_pipelineShaders.empty() && m_pRenderpass != nullptr;
        }

    public:

        size_t GetHash() const
        {
            size_t hash = 0;
            for ( auto const& pipelineShader : m_pipelineShaders )
            {
                Hash::HashCombine( hash, pipelineShader.m_shaderPath.c_str() );
                Hash::HashCombine( hash, pipelineShader.m_entryName );
            }
            return hash;
        }

        friend bool operator==( RHIRasterPipelineStateCreateDesc const& lhs, RHIRasterPipelineStateCreateDesc const& rhs )
        {
            // TODO: this is not right for pipelines which share same set of shaders but have different pipeline states.
            return lhs.GetHash() == rhs.GetHash();
        }

    public:

        TSet<RHIPipelineShader>         m_pipelineShaders;
        RHIRenderPass*                  m_pRenderpass = nullptr;
        RHIPipelineRasterizerState      m_rasterizerState;
        RHIPipelineBlendState           m_blendState;
        ERHIPipelinePirmitiveTopology   m_primitiveTopology = ERHIPipelinePirmitiveTopology::TriangleList;
        ESampleCount                    m_multisampleCount = ESampleCount::SC1;

        bool                            m_enableDepthTest = false;
        bool                            m_enableDepthWrite = false;
        bool                            m_enableDepthBias = false;
    };

    //-------------------------------------------------------------------------

    enum class EBindingResourceType : uint8_t
    {
        Sampler,
        CombinedTextureSampler,
        SampleTexture,
        StorageTexture,
        UniformTexelBuffer,
        StorageTexelBuffer,
        UniformBuffer,
        StorageBuffer,
        UniformBufferDynamic,
        StorageBufferDynamic,
        InputAttachment
    };

    static constexpr uint32_t NumMaxResourceBindingSet = 4;

    //-------------------------------------------------------------------------

    struct RHIShaderCreateDesc
    {
    public:

        inline bool IsValid() const { return !m_byteCode.empty(); }

    public:

        // Avoid copy, but be careful with the life time.
        Blob const&                 m_byteCode; // Compiled shader byte code.
    };

    // Synchronization Primitives
    //-------------------------------------------------------------------------

    struct RHISemaphoreCreateDesc
    {
    public:

        inline bool IsValid() const { return true; }

    public:
    };

    class RHIDevice;

    struct IRHICPUGPUSyncImpl
    {
        virtual ~IRHICPUGPUSyncImpl() = default;

        virtual void* Create( RHIDevice* pDevice, bool bSignaled = false ) = 0;
        virtual void  Destroy( RHIDevice* pDevice, void*& pCPUGPUSync ) = 0;

        virtual void  Reset( RHIDevice* pDevice, void* pCPUGPUSync ) = 0;
        virtual void  WaitFor( RHIDevice* pDevice, void* pCPUGPUSync, uint64_t waitTime = std::numeric_limits<uint64_t>::max() ) = 0;
    };

    // Used to synchronize CPU and GPU.
    class RHICPUGPUSync
    {
    public:

        void*                           m_pHandle = nullptr;

    public:

        inline static RHICPUGPUSync Create( RHIDevice* pDevice, IRHICPUGPUSyncImpl* pImpl, bool bSignaled = false )
        {
            EE_ASSERT( pImpl );
            RHICPUGPUSync sync;
            sync.m_pImpl = pImpl;
            sync.m_pHandle = pImpl->Create( pDevice, bSignaled );
            return sync;
        }
        inline static void Destroy( RHIDevice* pDevice, RHICPUGPUSync& sync )
        {
            EE_ASSERT( sync.m_pImpl );
            sync.m_pImpl->Destroy( pDevice, sync.m_pHandle );
        }

        inline void Reset( RHIDevice* pDevice )
        {
            EE_ASSERT( m_pImpl );
            m_pImpl->Reset( pDevice, m_pHandle );
        }

        inline void WaitFor( RHIDevice* pDevice )
        {
            EE_ASSERT( m_pImpl );
            m_pImpl->WaitFor( pDevice, m_pHandle );
        }

    public:

        RHICPUGPUSync() = default;
        ~RHICPUGPUSync() = default;

        RHICPUGPUSync( RHICPUGPUSync const& ) = delete;
        RHICPUGPUSync& operator=( RHICPUGPUSync const& ) = delete;

        RHICPUGPUSync( RHICPUGPUSync&& ) = default;
        RHICPUGPUSync& operator=( RHICPUGPUSync&& ) = default;

    public:

        inline bool IsValid() const { return m_pHandle != nullptr; }

    private:

        IRHICPUGPUSyncImpl*                         m_pImpl = nullptr;
    };
}

// Hash
//-------------------------------------------------------------------------

namespace eastl
{
    template <>
    struct hash<EE::RHI::RHIBufferCreateDesc>
    {
        EE_FORCE_INLINE eastl_size_t operator()( EE::RHI::RHIBufferCreateDesc const& bufferCreateDesc ) const noexcept
        {
            return bufferCreateDesc.GetHash();
        }
    };

    template <>
    struct hash<EE::RHI::RHITextureCreateDesc>
    {
        EE_FORCE_INLINE eastl_size_t operator()( EE::RHI::RHITextureCreateDesc const& textureCreateDesc ) const noexcept
        {
            return textureCreateDesc.GetHash();
        }
    };

    template <>
    struct hash<EE::RHI::RHITextureViewCreateDesc>
    {
        eastl_size_t operator()( EE::RHI::RHITextureViewCreateDesc const& textureViewCreateDesc ) const noexcept
        {
            eastl::hash<eastl::optional<EE::RHI::ETextureViewType>> hasher;
            eastl_size_t hash = hasher( textureViewCreateDesc.m_viewType );
            EE::Hash::HashCombine( hash, textureViewCreateDesc.m_format );
            EE::Hash::HashCombine( hash, textureViewCreateDesc.m_viewAspect.Get() );
            EE::Hash::HashCombine( hash, textureViewCreateDesc.m_levelCount );
            EE::Hash::HashCombine( hash, textureViewCreateDesc.m_baseMipmap );
            return hash;
        }
    };

    template <>
    struct hash<EE::RHI::RHIFramebufferAttachmentHash>
    {
        eastl_size_t operator()( EE::RHI::RHIFramebufferAttachmentHash const& attachmentHash ) const noexcept
        {
            eastl_size_t hash = eastl::hash<uint32_t>()( attachmentHash.m_createFlags.Get() );
            EE::Hash::HashCombine( hash, attachmentHash.m_usage.Get() );
            return hash;
        }
    };

    template <>
    struct hash<EE::RHI::RHIFramebufferCacheKey>
    {
        eastl_size_t operator()( EE::RHI::RHIFramebufferCacheKey const& key ) const noexcept
        {
            eastl_size_t hash = eastl::hash<uint32_t>()( key.m_extentX );
            EE::Hash::HashCombine( hash, key.m_extentY );
            for ( auto const& attachmentHash : key.m_attachmentHashs )
            {
                EE::Hash::HashCombine( hash, attachmentHash );
            }
            return hash;
        }
    };
    
    //-------------------------------------------------------------------------

    template<>
    struct hash<EE::RHI::RHIPipelineShader>
    {
        eastl_size_t operator()( EE::RHI::RHIPipelineShader const& pipelineShader ) const noexcept
        {
            eastl_size_t hash = eastl::hash<uint8_t>()( static_cast<uint8_t>( pipelineShader.m_stage ) );
            EE::Hash::HashCombine(hash, pipelineShader.m_shaderPath.c_str() );
            EE::Hash::HashCombine(hash, pipelineShader.m_entryName );
            return hash;
        }
    };

    template<>
    struct hash<EE::RHI::RHIRasterPipelineStateCreateDesc>
    {
        EE_FORCE_INLINE eastl_size_t operator()( EE::RHI::RHIRasterPipelineStateCreateDesc const& pipelineDesc ) const noexcept
        {
            return pipelineDesc.GetHash();
        }
    };

}