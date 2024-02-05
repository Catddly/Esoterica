#pragma once

#include "Engine/Render/IRenderer.h"
#include "Base/Render/RenderDevice.h"
#include "Base/Math/Matrix.h"

//-------------------------------------------------------------------------

namespace EE::RG
{
    class RenderGraph;
    class RGRenderCommandContext;
    class RGBoundPipeline;
}
namespace EE::RHI
{ 
    class RHIBuffer;
    class RHIRenderPass;
}

//-------------------------------------------------------------------------

namespace EE::Render
{
    class DirectionalLightComponent;
    class GlobalEnvironmentMapComponent;
    class PointLightComponent;
    class StaticMeshComponent;
    class SkeletalMeshComponent;
    class SkeletalMesh;
    class StaticMesh;
    class Viewport;
    class Material;

    //-------------------------------------------------------------------------

    class EE_ENGINE_API WorldRenderer : public IRenderer
    {
        enum
        {
            LIGHTING_ENABLE_SUN = ( 1 << 0 ),
            LIGHTING_ENABLE_SUN_SHADOW = ( 1 << 1 ),
            LIGHTING_ENABLE_SKYLIGHT = ( 1 << 2 ),
        };

        enum
        {
            MATERIAL_USE_ALBEDO_TEXTURE = ( 1 << 0 ),
            MATERIAL_USE_NORMAL_TEXTURE = ( 1 << 1 ),
            MATERIAL_USE_METALNESS_TEXTURE = ( 1 << 2 ),
            MATERIAL_USE_ROUGHNESS_TEXTURE = ( 1 << 3 ),
            MATERIAL_USE_AO_TEXTURE = ( 1 << 4 ),
        };

        constexpr static int32_t const s_maxPunctualLights = 16;

        struct PunctualLight
        {
            Vector              m_positionInvRadiusSqr;
            Vector              m_dir;
            Vector              m_color;
            Vector              m_spotAngles;
        };

        struct LightData
        {
            Vector              m_SunDirIndirectIntensity = Vector::Zero;// TODO: refactor to Float3 and float
            Vector              m_SunColorRoughnessOneLevel = Vector::Zero;// TODO: refactor to Float3 and float
            Matrix              m_sunShadowMapMatrix = Matrix( ZeroInit );
            float               m_manualExposure = -1.0f;
            uint32_t            m_lightingFlags = 0;
            uint32_t            m_numPunctualLights = 0;
            PunctualLight       m_punctualLights[s_maxPunctualLights];
        };

        struct alignas(16) PickingData
        {
            PickingData() = default;

            inline PickingData( uint64_t v0, uint64_t v1 )
            {
                m_ID[0] = (uint32_t) ( v0 & 0x00000000FFFFFFFF );
                m_ID[1] = (uint32_t) ( ( v0 >> 32 ) & 0x00000000FFFFFFFF );
                m_ID[2] = (uint32_t) ( v1 & 0x00000000FFFFFFFF );
                m_ID[3] = (uint32_t) ( ( v1 >> 32 ) & 0x00000000FFFFFFFF );
            }

            uint32_t        m_ID[4];
            Float4          m_padding;
        };

        struct MaterialData
        {
            uint32_t    m_surfaceFlags = 0;
            float       m_metalness = 0.0f;
            float       m_roughness = 0.0f;
            float       m_normalScaler = 1.0f;
            Vector      m_albedo;
        };

        struct ObjectTransforms
        {
            Matrix      m_worldTransform = Matrix( ZeroInit );
            Matrix      m_normalTransform = Matrix( ZeroInit );
            Matrix      m_viewprojTransform = Matrix( ZeroInit );
        };

        struct RenderData //TODO: optimize - there should not be per frame updates
        {
            ObjectTransforms                            m_transforms;
            LightData                                   m_lightData;
            CubemapTexture const*                       m_pSkyboxRadianceTexture;
            CubemapTexture const*                       m_pSkyboxTexture;
            TVector<StaticMeshComponent const*>&        m_staticMeshComponents;
            TVector<SkeletalMeshComponent const*>&      m_skeletalMeshComponents;

            RenderData(
                ObjectTransforms const& transforms, LightData const& lightData,
                CubemapTexture const* pSkyboxRadianceTexture, CubemapTexture const* pSkyboxTexture,
                TVector<StaticMeshComponent const*>& staticMeshComponents, TVector<SkeletalMeshComponent const*>& skeletalMeshComponents
            )
                : m_transforms( transforms ), m_lightData( lightData ),
                m_pSkyboxRadianceTexture( pSkyboxRadianceTexture ), m_pSkyboxTexture( pSkyboxTexture ),
                m_staticMeshComponents( staticMeshComponents ), m_skeletalMeshComponents( skeletalMeshComponents )
            {
            }
        };

    public:

        EE_RENDERER_ID( WorldRenderer, Render::RendererPriorityLevel::Game );

        static void SetMaterial( RG::RGRenderCommandContext& context, RG::RGBoundPipeline& boundPipeline, RHI::RHIBuffer* pMaterialDataBuffer, Material const* pMaterial );
        static void SetDefaultMaterial( RG::RGRenderCommandContext& context, RG::RGBoundPipeline& boundPipeline, RHI::RHIBuffer* pMaterialDataBuffer );

    public:

        inline bool IsInitialized() const { return m_initialized; }
        bool Initialize( RenderDevice* pRenderDevice );
        void Shutdown();

        virtual void RenderWorld( Seconds const deltaTime, Viewport const& viewport, RenderTarget const& renderTarget, EntityWorld* pWorld ) override final;
        virtual void RenderWorld_Test( RG::RenderGraph& renderGraph, Seconds const deltaTime, Viewport const& viewport, RenderTarget const& renderTarget, EntityWorld* pWorld ) override final;

    private:

        void ComputeBrdfLut( RG::RenderGraph& renderGraph );

        void RenderSunShadows( RG::RenderGraph& renderGraph, Viewport const& viewport, DirectionalLightComponent* pDirectionalLightComponent );
        void RenderStaticMeshes( RG::RenderGraph& renderGraph, Viewport const& viewport, RenderTarget const& renderTarget );
        void RenderSkeletalMeshes( Viewport const& viewport, RenderTarget const& renderTarget );
        void RenderSkybox( RG::RenderGraph& renderGraph, Viewport const& viewport, RenderTarget const& renderTarget );

        void SetupRenderStates( Viewport const& viewport, PixelShader* pShader, RenderData const& data );

    private:

        bool                                                m_initialized = false;
        bool                                                m_bBrdfLutReady = false;

        // Render State
        RenderDevice*                                       m_pRenderDevice = nullptr;
        //RHI::RHIRenderPass*                                 m_pSkyboxRenderPass = nullptr;
        RHI::RHIRenderPass*                                 m_pOpaqueRenderPass = nullptr;
        RHI::RHIRenderPass*                                 m_pOpaquePickingRenderPass = nullptr;
        RHI::RHIRenderPass*                                 m_pShadowRenderPass = nullptr;

        //VertexShader                                            m_vertexShaderSkybox;
        //PixelShader                                             m_pixelShaderSkybox;
        //VertexShader                                            m_vertexShaderStatic;
        //VertexShader                                            m_vertexShaderSkeletal;
        //PixelShader                                             m_pixelShader;
        //PixelShader                                             m_emptyPixelShader;
        //BlendState                                              m_blendState;
        //RasterizerState                                         m_rasterizerState;
        //SamplerState                                            m_bilinearSampler;
        //SamplerState                                            m_bilinearClampedSampler;
        //SamplerState                                            m_shadowSampler;
        //ShaderInputBindingHandle                                m_inputBindingStatic;
        //ShaderInputBindingHandle                                m_inputBindingSkeletal;

        // TODO: we change the origin PipelineState to RasterPipelineState
        //RasterPipelineState                                     m_pipelineStateStatic;
        //RasterPipelineState                                     m_pipelineStateSkeletal;
        //RasterPipelineState                                     m_pipelineStateStaticShadow;
        //RasterPipelineState                                     m_pipelineStateSkeletalShadow;
        //RasterPipelineState                                     m_pipelineSkybox;
        //ComputeShader                                           m_precomputeDFGComputeShader;
        //Texture                                                 m_precomputedBRDF;
        //Texture                                                 m_shadowMap;
        //ComputePipelineState                                    m_pipelinePrecomputeBRDF;

        //PixelShader                                             m_pixelShaderPicking;
        //RasterPipelineState                                     m_pipelineStateStaticPicking;
        //RasterPipelineState                                     m_pipelineStateSkeletalPicking;

        // This is unsafe!!!
        // In render graph render system we defer the command execution after compiling,
        // any data reference by render graph node should be valid (i.e. alive) unless rendering is complete.
        // User should manually keep this data alive and it will cause some issues.
        // TODO: Add functionality to allow render graph allow any associative data within node.
        // It is render graph's responsibility to keep this data valid.
        RenderData*                                         m_pRenderData = nullptr;
    };
}