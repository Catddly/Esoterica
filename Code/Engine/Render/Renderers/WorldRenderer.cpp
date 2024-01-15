#include "WorldRenderer.h"
#include "Engine/Render/Components/Component_EnvironmentMaps.h"
#include "Engine/Render/Components/Component_Lights.h"
#include "Engine/Render/Components/Component_StaticMesh.h"
#include "Engine/Render/Components/Component_SkeletalMesh.h"
#include "Engine/Render/Shaders/EngineShaders.h"
#include "Engine/Render/Systems/WorldSystem_Renderer.h"
#include "Engine/Entity/Entity.h"
#include "Engine/Entity/EntityWorldUpdateContext.h"
#include "Engine/Entity/EntityWorld.h"
#include "Base/Render/RenderCoreResources.h"
#include "Base/Render/RenderViewport.h"
#include "Base/RenderGraph/RenderGraph.h"
#include "Base/Profiling.h"
#include "Base/RHI/Resource/RHIBuffer.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    static Matrix ComputeShadowMatrix( Viewport const& viewport, Transform const& lightWorldTransform, float shadowDistance )
    {
        Transform lightTransform = lightWorldTransform;
        lightTransform.SetTranslation( Vector::Zero );
        Transform const invLightTransform = lightTransform.GetInverse();

        // Get a modified camera view volume that has the shadow distance as the z far.
        // This will get us the appropriate corners to translate into light space.

        // To make these cascade, you do this in a loop and move the depth range along by your
        // cascade distance.
        EE::Math::ViewVolume camVolume = viewport.GetViewVolume();
        camVolume.SetDepthRange( FloatRange( 1.0f, shadowDistance ) );

        Math::ViewVolume::VolumeCorners corners = camVolume.GetCorners();

        // Translate into light space.
        for ( int32_t i = 0; i < 8; i++ )
        {
            corners.m_points[i] = invLightTransform.TransformPoint( corners.m_points[i] );
        }

        // Note for understanding, cornersMin and cornersMax are in light space, not world space.
        Vector cornersMin = Vector::One * FLT_MAX;
        Vector cornersMax = Vector::One * -FLT_MAX;

        for ( int32_t i = 0; i < 8; i++ )
        {
            cornersMin = Vector::Min( cornersMin, corners.m_points[i] );
            cornersMax = Vector::Max( cornersMax, corners.m_points[i] );
        }

        Vector lightPosition = Vector::Lerp( cornersMin, cornersMax, 0.5f );
        lightPosition = Vector::Select( lightPosition, cornersMax, Vector::Select0100 ); //force lightPosition to the "back" of the box.
        lightPosition = lightTransform.TransformPoint( lightPosition );   //Light position now in world space.
        lightTransform.SetTranslation( lightPosition );   //Assign to the lightTransform, now it's positioned above our view frustum.

        Float3 const delta = ( cornersMax - cornersMin ).ToFloat3();
        float dim = Math::Max( delta.m_x, delta.m_z );
        Math::ViewVolume lightViewVolume( Float2( dim ), FloatRange( 1.0, delta.m_y ), lightTransform.ToMatrix() );

        Matrix viewProjMatrix = lightViewVolume.GetViewProjectionMatrix();
        Matrix viewMatrix = lightViewVolume.GetViewMatrix();
        Matrix projMatrix = lightViewVolume.GetProjectionMatrix();
        Matrix viewProj = lightViewVolume.GetViewProjectionMatrix(); // TODO: inverse z???
        return viewProj;
    }

    //-------------------------------------------------------------------------

    bool WorldRenderer::Initialize( RenderDevice* pRenderDevice )
    {
        EE_ASSERT( m_pRenderDevice == nullptr && pRenderDevice != nullptr );
        m_pRenderDevice = pRenderDevice;

        //TVector<RenderBuffer> cbuffers;
        //RenderBuffer buffer;

        // Create Static Mesh Vertex Shader
        //-------------------------------------------------------------------------

        //cbuffers.clear();

        // World transform const buffer
        //buffer.m_byteSize = sizeof( ObjectTransforms );
        //buffer.m_byteStride = sizeof( Matrix ); // Vector4 aligned
        //buffer.m_usage = RenderBuffer::Usage::CPU_and_GPU;
        //buffer.m_type = RenderBuffer::Type::Constant;
        //buffer.m_slot = 0;
        //cbuffers.push_back( buffer );

        // Shaders
        //auto const vertexLayoutDescStatic = VertexLayoutRegistry::GetDescriptorForFormat( VertexFormat::StaticMesh );
        //m_vertexShaderStatic = VertexShader( g_byteCode_VS_StaticPrimitive, sizeof( g_byteCode_VS_StaticPrimitive ), cbuffers, vertexLayoutDescStatic );
        //m_pRenderDevice->CreateShader( m_vertexShaderStatic );

        // Create Skeletal Mesh Vertex Shader
        //-------------------------------------------------------------------------

        // Vertex shader constant buffer - contains the world view projection matrix and bone transforms
        //buffer.m_byteSize = sizeof( Matrix ) * 255; // ( 1 WVP matrix + 255 bone matrices )
        //buffer.m_byteStride = sizeof( Matrix ); // Vector4 aligned
        //buffer.m_usage = RenderBuffer::Usage::CPU_and_GPU;
        //buffer.m_type = RenderBuffer::Type::Constant;
        //buffer.m_slot = 0;
        //cbuffers.push_back( buffer );

        //auto const vertexLayoutDescSkeletal = VertexLayoutRegistry::GetDescriptorForFormat( VertexFormat::SkeletalMesh );
        //m_vertexShaderSkeletal = VertexShader( g_byteCode_VS_SkinnedPrimitive, sizeof( g_byteCode_VS_SkinnedPrimitive ), cbuffers, vertexLayoutDescSkeletal );
        //pRenderDevice->CreateShader( m_vertexShaderSkeletal );

        //if ( !m_vertexShaderStatic.IsValid() )
        //{
        //    return false;
        //}

        // Create Skybox Vertex Shader
        //-------------------------------------------------------------------------

        //cbuffers.clear();

        // Transform constant buffer
        //buffer.m_byteSize = sizeof( Matrix );
        //buffer.m_byteStride = sizeof( Matrix ); // Vector4 aligned
        //buffer.m_usage = RenderBuffer::Usage::CPU_and_GPU;
        //buffer.m_type = RenderBuffer::Type::Constant;
        //buffer.m_slot = 0;
        //cbuffers.push_back( buffer );

        //auto const vertexLayoutDescNone = VertexLayoutRegistry::GetDescriptorForFormat( VertexFormat::None );
        //m_vertexShaderSkybox = VertexShader( g_byteCode_VS_Cube, sizeof( g_byteCode_VS_Cube ), cbuffers, vertexLayoutDescNone );
        //m_pRenderDevice->CreateShader( m_vertexShaderSkybox );

        //if ( !m_vertexShaderSkybox.IsValid() )
        //{
        //    return false;
        //}

        // Create Pixel Shader
        //-------------------------------------------------------------------------

        //cbuffers.clear();

        // Pixel shader constant buffer - contains light info
        //buffer.m_byteSize = sizeof( LightData );
        //buffer.m_byteStride = sizeof( LightData );
        //buffer.m_usage = RenderBuffer::Usage::CPU_and_GPU;
        //buffer.m_type = RenderBuffer::Type::Constant;
        //buffer.m_slot = 0;
        //cbuffers.push_back( buffer );

        //buffer.m_byteSize = sizeof( MaterialData );
        //buffer.m_byteStride = sizeof( MaterialData );
        //buffer.m_usage = RenderBuffer::Usage::CPU_and_GPU;
        //buffer.m_type = RenderBuffer::Type::Constant;
        //buffer.m_slot = 1;
        //cbuffers.push_back( buffer );

        //m_pixelShader = PixelShader( g_byteCode_PS_Lit, sizeof( g_byteCode_PS_Lit ), cbuffers );
        //m_pRenderDevice->CreateShader( m_pixelShader );

        //if ( !m_pixelShader.IsValid() )
        //{
        //    return false;
        //}

        // Create Skybox Pixel Shader
        //-------------------------------------------------------------------------

        //m_pixelShaderSkybox = PixelShader( g_byteCode_PS_Skybox, sizeof( g_byteCode_PS_Skybox ), cbuffers );
        //m_pRenderDevice->CreateShader( m_pixelShaderSkybox );

        //if ( !m_pixelShaderSkybox.IsValid() )
        //{
        //    return false;
        //}

        // Create Picking-Enabled Pixel Shader
        //-------------------------------------------------------------------------

        //buffer.m_byteSize = sizeof( PickingData );
        //buffer.m_byteStride = sizeof( PickingData );
        //buffer.m_usage = RenderBuffer::Usage::CPU_and_GPU;
        //buffer.m_type = RenderBuffer::Type::Constant;
        //buffer.m_slot = 2;
        //cbuffers.push_back( buffer );

        //m_pixelShaderPicking = PixelShader( g_byteCode_PS_LitPicking, sizeof( g_byteCode_PS_LitPicking ), cbuffers );
        //m_pRenderDevice->CreateShader( m_pixelShaderPicking );

        //if ( !m_pixelShaderPicking.IsValid() )
        //{
        //    return false;
        //}

        // Create Empty Pixel Shader
        //-------------------------------------------------------------------------

        //cbuffers.clear();

        //m_emptyPixelShader = PixelShader( g_byteCode_PS_Empty, sizeof( g_byteCode_PS_Empty ), cbuffers );
        //m_pRenderDevice->CreateShader( m_emptyPixelShader );

        //if ( !m_emptyPixelShader.IsValid() )
        //{
        //    return false;
        //}

        // Create BRDF Integration Compute Shader
        //-------------------------------------------------------------------------

        //cbuffers.clear();

        //m_precomputeDFGComputeShader = ComputeShader( g_byteCode_CS_PrecomputeDFG, sizeof( g_byteCode_CS_PrecomputeDFG ), cbuffers );

        //m_pRenderDevice->CreateShader( m_precomputeDFGComputeShader );
        //if ( !m_precomputeDFGComputeShader.IsValid() )
        //{
        //    return false;
        //}

        // Create blend state
        //-------------------------------------------------------------------------

        //m_blendState.m_srcValue = BlendValue::SourceAlpha;
        //m_blendState.m_dstValue = BlendValue::InverseSourceAlpha;
        //m_blendState.m_blendOp = BlendOp::Add;
        //m_blendState.m_blendEnable = true;

        //m_pRenderDevice->CreateBlendState( m_blendState );

        //if ( !m_blendState.IsValid() )
        //{
        //    return false;
        //}

        // Create rasterizer state
        //-------------------------------------------------------------------------

        //m_rasterizerState.m_cullMode = CullMode::BackFace;
        //m_rasterizerState.m_windingMode = WindingMode::CounterClockwise;
        //m_rasterizerState.m_fillMode = FillMode::Solid;
        //m_pRenderDevice->CreateRasterizerState( m_rasterizerState );
        //if ( !m_rasterizerState.IsValid() )
        //{
        //    return false;
        //}

        // Set up samplers
        //-------------------------------------------------------------------------

        //m_pRenderDevice->CreateSamplerState( m_bilinearSampler );
        //if ( !m_bilinearSampler.IsValid() )
        //{
        //    return false;
        //}

        //m_bilinearClampedSampler.m_addressModeU = TextureAddressMode::Clamp;
        //m_bilinearClampedSampler.m_addressModeV = TextureAddressMode::Clamp;
        //m_bilinearClampedSampler.m_addressModeW = TextureAddressMode::Clamp;
        //m_pRenderDevice->CreateSamplerState( m_bilinearClampedSampler );
        //if ( !m_bilinearClampedSampler.IsValid() )
        //{
        //    return false;
        //}

        //m_shadowSampler.m_addressModeU = TextureAddressMode::Border;
        //m_shadowSampler.m_addressModeV = TextureAddressMode::Border;
        //m_shadowSampler.m_addressModeW = TextureAddressMode::Border;
        //m_shadowSampler.m_borderColor = Float4( 1.0f );
        //m_pRenderDevice->CreateSamplerState( m_shadowSampler );
        //if ( !m_shadowSampler.IsValid() )
        //{
        //    return false;
        //}

        // Set up input bindings
        //-------------------------------------------------------------------------

        //m_pRenderDevice->CreateShaderInputBinding( m_vertexShaderStatic, vertexLayoutDescStatic, m_inputBindingStatic );
        //if ( !m_inputBindingStatic.IsValid() )
        //{
        //    return false;
        //}

        //m_pRenderDevice->CreateShaderInputBinding( m_vertexShaderSkeletal, vertexLayoutDescSkeletal, m_inputBindingSkeletal );
        //if ( !m_inputBindingSkeletal.IsValid() )
        //{
        //    return false;
        //}

        // Set up pipeline states
        //-------------------------------------------------------------------------

        //m_pipelineStateStatic.m_pVertexShader = &m_vertexShaderStatic;
        //m_pipelineStateStatic.m_pPixelShader = &m_pixelShader;
        //m_pipelineStateStatic.m_pBlendState = &m_blendState;
        //m_pipelineStateStatic.m_pRasterizerState = &m_rasterizerState;

        //m_pipelineStateStaticPicking = m_pipelineStateStatic;
        //m_pipelineStateStaticPicking.m_pPixelShader = &m_pixelShaderPicking;

        //m_pipelineStateSkeletal.m_pVertexShader = &m_vertexShaderSkeletal;
        //m_pipelineStateSkeletal.m_pPixelShader = &m_pixelShader;
        //m_pipelineStateSkeletal.m_pBlendState = &m_blendState;
        //m_pipelineStateSkeletal.m_pRasterizerState = &m_rasterizerState;

        //m_pipelineStateSkeletalPicking = m_pipelineStateSkeletal;
        //m_pipelineStateSkeletalPicking.m_pPixelShader = &m_pixelShaderPicking;

        //m_pipelineStateStaticShadow.m_pVertexShader = &m_vertexShaderStatic;
        //m_pipelineStateStaticShadow.m_pPixelShader = &m_emptyPixelShader;
        //m_pipelineStateStaticShadow.m_pBlendState = &m_blendState;
        //m_pipelineStateStaticShadow.m_pRasterizerState = &m_rasterizerState;

        //m_pipelineStateSkeletalShadow.m_pVertexShader = &m_vertexShaderSkeletal;
        //m_pipelineStateSkeletalShadow.m_pPixelShader = &m_emptyPixelShader;
        //m_pipelineStateSkeletalShadow.m_pBlendState = &m_blendState;
        //m_pipelineStateSkeletalShadow.m_pRasterizerState = &m_rasterizerState;

        //m_pipelineSkybox.m_pVertexShader = &m_vertexShaderSkybox;
        //m_pipelineSkybox.m_pPixelShader = &m_pixelShaderSkybox;

        //m_pRenderDevice->CreateTexture( m_precomputedBRDF, DataFormat::Float_R16G16, Float2( 512, 512 ), USAGE_UAV | USAGE_SRV ); // TODO: load from memory?
        //m_pipelinePrecomputeBRDF.m_pComputeShader = &m_precomputeDFGComputeShader;

        //// TODO create on directional light add and destroy on remove
        //m_pRenderDevice->CreateTexture( m_shadowMap, DataFormat::Float_X32, Float2( 2048, 2048 ), USAGE_SRV | USAGE_RT_DS );

        //// Dispatch brdf lut computation once
        ////-------------------------------------------------------------------------
        //{
        //    auto const& renderContext = m_pRenderDevice->GetImmediateContext();
        //    renderContext.SetComputePipelineState( m_pipelinePrecomputeBRDF );
        //    renderContext.SetUnorderedAccess( PipelineStage::Compute, 0, m_precomputedBRDF.GetUnorderedAccessView() );
        //    renderContext.Dispatch( 512 / 16, 512 / 16, 1 );
        //    renderContext.ClearUnorderedAccess( PipelineStage::Compute, 0 );
        //}

        // TODO: render graph auto render pass creation
        if ( !m_pShadowRenderPass )
        {
            RHI::RHIRenderPassCreateDesc renderPassDesc;
            renderPassDesc.m_depthAttachment = RHI::RHIRenderPassAttachmentDesc::UselessInput( RHI::EPixelFormat::Depth32 );
            m_pShadowRenderPass = m_pRenderDevice->GetRHIDevice()->CreateRenderPass( renderPassDesc );

            if ( !m_pShadowRenderPass )
            {
                return false;
            }
        }

        //if ( !m_pSkyboxRenderPass )
        //{
        //    RHI::RHIRenderPassCreateDesc renderPassDesc;
        //    renderPassDesc.m_colorAttachments.push_back( RHI::RHIRenderPassAttachmentDesc::TrivialColor( RHI::EPixelFormat::RGBA8Unorm ) );
        //    m_pSkyboxRenderPass = m_pRenderDevice->GetRHIDevice()->CreateRenderPass( renderPassDesc );

        //    if ( !m_pSkyboxRenderPass )
        //    {
        //        return false;
        //    }
        //}

        if ( !m_pOpaqueRenderPass )
        {
            RHI::RHIRenderPassCreateDesc renderPassDesc;
            renderPassDesc.m_colorAttachments.push_back( RHI::RHIRenderPassAttachmentDesc::TrivialColor( RHI::EPixelFormat::RGBA8Unorm ) );
            renderPassDesc.m_depthAttachment = RHI::RHIRenderPassAttachmentDesc::UselessInput( RHI::EPixelFormat::Depth32 );
            m_pOpaqueRenderPass = m_pRenderDevice->GetRHIDevice()->CreateRenderPass( renderPassDesc );

            if ( !m_pOpaqueRenderPass )
            {
                return false;
            }
        }

        if ( !m_pOpaquePickingRenderPass )
        {
            RHI::RHIRenderPassCreateDesc renderPassDesc;
            renderPassDesc.m_colorAttachments.push_back( RHI::RHIRenderPassAttachmentDesc::TrivialColor( RHI::EPixelFormat::RGBA8Unorm ) );
            renderPassDesc.m_colorAttachments.push_back( RHI::RHIRenderPassAttachmentDesc::ClearInput( RHI::EPixelFormat::RG16Float ) );
            renderPassDesc.m_depthAttachment = RHI::RHIRenderPassAttachmentDesc::UselessInput( RHI::EPixelFormat::Depth32 );
            m_pOpaquePickingRenderPass = m_pRenderDevice->GetRHIDevice()->CreateRenderPass( renderPassDesc );

            if ( !m_pOpaquePickingRenderPass )
            {
                return false;
            }
        }

        m_initialized = true;
        return true;
    }

    void WorldRenderer::Shutdown()
    {
        //m_pipelineStateStatic.Clear();
        //m_pipelineStateSkeletal.Clear();

        //if ( m_inputBindingStatic.IsValid() )
        //{
        //    m_pRenderDevice->DestroyShaderInputBinding( m_inputBindingStatic );
        //}

        //if ( m_inputBindingSkeletal.IsValid() )
        //{
        //    m_pRenderDevice->DestroyShaderInputBinding( m_inputBindingSkeletal );
        //}

        //if ( m_rasterizerState.IsValid() )
        //{
        //    m_pRenderDevice->DestroyRasterizerState( m_rasterizerState );
        //}

        //if ( m_blendState.IsValid() )
        //{
        //    m_pRenderDevice->DestroyBlendState( m_blendState );
        //}

        //if ( m_bilinearSampler.IsValid() )
        //{
        //    m_pRenderDevice->DestroySamplerState( m_bilinearSampler );
        //}

        //if ( m_shadowSampler.IsValid() )
        //{
        //    m_pRenderDevice->DestroySamplerState( m_shadowSampler );
        //}

        //if ( m_bilinearClampedSampler.IsValid() )
        //{
        //    m_pRenderDevice->DestroySamplerState( m_bilinearClampedSampler );
        //}

        //if ( m_vertexShaderStatic.IsValid() )
        //{
        //    m_pRenderDevice->DestroyShader( m_vertexShaderStatic );
        //}

        //if ( m_vertexShaderSkeletal.IsValid() )
        //{
        //    m_pRenderDevice->DestroyShader( m_vertexShaderSkeletal );
        //}

        //if ( m_pixelShader.IsValid() )
        //{
        //    m_pRenderDevice->DestroyShader( m_pixelShader );
        //}

        //if ( m_emptyPixelShader.IsValid() )
        //{
        //    m_pRenderDevice->DestroyShader( m_emptyPixelShader );
        //}

        //if ( m_vertexShaderSkybox.IsValid() )
        //{
        //    m_pRenderDevice->DestroyShader( m_vertexShaderSkybox );
        //}

        //if ( m_pixelShaderSkybox.IsValid() )
        //{
        //    m_pRenderDevice->DestroyShader( m_pixelShaderSkybox );
        //}

        //if ( m_precomputeDFGComputeShader.IsValid() )
        //{
        //    m_pRenderDevice->DestroyShader( m_precomputeDFGComputeShader );
        //}

        //if ( m_precomputedBRDF.IsValid() )
        //{
        //    m_pRenderDevice->DestroyTexture( m_precomputedBRDF );
        //}

        //if ( m_shadowMap.IsValid() )
        //{
        //    m_pRenderDevice->DestroyTexture( m_shadowMap );
        //}

        //if ( m_pixelShaderPicking.IsValid() )
        //{
        //    m_pRenderDevice->DestroyShader( m_pixelShaderPicking );
        //}

        //if ( m_pSkyboxRenderPass )
        //{
        //    m_pRenderDevice->GetRHIDevice()->DestroyRenderPass( m_pSkyboxRenderPass );
        //}

        if ( m_pOpaquePickingRenderPass )
        {
            m_pRenderDevice->GetRHIDevice()->DestroyRenderPass( m_pOpaquePickingRenderPass );
        }

        if ( m_pOpaqueRenderPass )
        {
            m_pRenderDevice->GetRHIDevice()->DestroyRenderPass( m_pOpaqueRenderPass );
        }

        if ( m_pShadowRenderPass )
        {
            m_pRenderDevice->GetRHIDevice()->DestroyRenderPass( m_pShadowRenderPass );
        }

        if ( m_pRenderData )
        {
            EE::Delete( m_pRenderData );
        }

        m_pRenderDevice = nullptr;
        m_initialized = false;
    }

    //-------------------------------------------------------------------------

    void WorldRenderer::SetMaterial( RG::RGRenderCommandContext& context, RG::RGBoundPipeline& boundPipeline, RHI::RHIBuffer* pMaterialDataBuffer, Material const* pMaterial )
    {
        EE_ASSERT( pMaterial != nullptr );

        RHI::RHITexture* pDefaultTexture = CoreResources::GetMissingTexture();

        // TODO: cache on GPU in buffer
        MaterialData materialData{};
        materialData.m_surfaceFlags |= pMaterial->HasAlbedoTexture() ? MATERIAL_USE_ALBEDO_TEXTURE : materialData.m_surfaceFlags;
        materialData.m_surfaceFlags |= pMaterial->HasMetalnessTexture() ? MATERIAL_USE_METALNESS_TEXTURE : materialData.m_surfaceFlags;
        materialData.m_surfaceFlags |= pMaterial->HasRoughnessTexture() ? MATERIAL_USE_ROUGHNESS_TEXTURE : materialData.m_surfaceFlags;
        materialData.m_surfaceFlags |= pMaterial->HasNormalMapTexture() ? MATERIAL_USE_NORMAL_TEXTURE : materialData.m_surfaceFlags;
        materialData.m_surfaceFlags |= pMaterial->HasAOTexture() ? MATERIAL_USE_AO_TEXTURE : materialData.m_surfaceFlags;
        materialData.m_metalness = pMaterial->GetMetalnessValue();
        materialData.m_roughness = pMaterial->GetRoughnessValue();
        materialData.m_normalScaler = pMaterial->GetNormalScalerValue();
        materialData.m_albedo = pMaterial->GetAlbedoValue().ToFloat4();

        //renderContext.WriteToBuffer( pixelShader.GetConstBuffer( 1 ), &materialData, sizeof( materialData ) );

        {
            auto* pMapped = pMaterialDataBuffer->MapTo<MaterialData*>( context.GetRHIDevice() );
            *pMapped = materialData;
            pMaterialDataBuffer->Unmap( context.GetRHIDevice() );
        }

        auto defaultTexBinding = RHI::RHITextureBinding{};
        defaultTexBinding.m_view = pDefaultTexture->GetOrCreateView( context.GetRHIDevice(), RHI::RHITextureViewCreateDesc{} );
        defaultTexBinding.m_layout = RHI::ETextureLayout::ShaderReadOnlyOptimal;

        auto* pAlbedoTexture = pMaterial->GetAlbedoTexture()->GetRHITexture();
        auto albedoTexBinding = RHI::RHITextureBinding{};
        if ( pAlbedoTexture )
        {
            albedoTexBinding.m_view = pAlbedoTexture->GetOrCreateView(context.GetRHIDevice(), RHI::RHITextureViewCreateDesc{});
            albedoTexBinding.m_layout = RHI::ETextureLayout::ShaderReadOnlyOptimal;
        }

        auto* pNormalTexture = pMaterial->GetNormalMapTexture()->GetRHITexture();
        auto normalTexBinding = RHI::RHITextureBinding{};
        if ( pNormalTexture )
        {
            normalTexBinding.m_view = pNormalTexture->GetOrCreateView( context.GetRHIDevice(), RHI::RHITextureViewCreateDesc{} );
            normalTexBinding.m_layout = RHI::ETextureLayout::ShaderReadOnlyOptimal;
        }

        auto* pMetalnessTexture = pMaterial->GetMetalnessTexture()->GetRHITexture();
        auto metalnessTexBinding = RHI::RHITextureBinding{};
        if ( pMetalnessTexture )
        {
            metalnessTexBinding.m_view = pMetalnessTexture->GetOrCreateView( context.GetRHIDevice(), RHI::RHITextureViewCreateDesc{} );
            metalnessTexBinding.m_layout = RHI::ETextureLayout::ShaderReadOnlyOptimal;
        }

        auto* pRoughnessTexture = pMaterial->GetRoughnessTexture()->GetRHITexture();
        auto roughnessTexBinding = RHI::RHITextureBinding{};
        if ( pRoughnessTexture )
        {
            roughnessTexBinding.m_view = pRoughnessTexture->GetOrCreateView( context.GetRHIDevice(), RHI::RHITextureViewCreateDesc{} );
            roughnessTexBinding.m_layout = RHI::ETextureLayout::ShaderReadOnlyOptimal;
        }

        auto* pAOTexture = pMaterial->GetAOTexture()->GetRHITexture();
        auto aoTexBinding = RHI::RHITextureBinding{};
        if ( pAOTexture )
        {
            aoTexBinding.m_view = pAOTexture->GetOrCreateView( context.GetRHIDevice(), RHI::RHITextureViewCreateDesc{} );
            aoTexBinding.m_layout = RHI::ETextureLayout::ShaderReadOnlyOptimal;
        }

        RG::RGPipelineBinding const binding[] = {
            RG::BindRaw( { ( materialData.m_surfaceFlags & MATERIAL_USE_ALBEDO_TEXTURE ) ? albedoTexBinding : defaultTexBinding } ),
            RG::BindRaw( { ( materialData.m_surfaceFlags & MATERIAL_USE_NORMAL_TEXTURE ) ? normalTexBinding : defaultTexBinding } ),
            RG::BindRaw( { ( materialData.m_surfaceFlags & MATERIAL_USE_METALNESS_TEXTURE ) ? metalnessTexBinding : defaultTexBinding } ),
            RG::BindRaw( { ( materialData.m_surfaceFlags & MATERIAL_USE_ROUGHNESS_TEXTURE ) ? roughnessTexBinding : defaultTexBinding } ),
            RG::BindRaw( { ( materialData.m_surfaceFlags & MATERIAL_USE_AO_TEXTURE ) ? aoTexBinding : defaultTexBinding } )
        };
        boundPipeline.Bind( 1, binding );
    }

    void WorldRenderer::SetDefaultMaterial( RG::RGRenderCommandContext& context, RG::RGBoundPipeline& boundPipeline, RHI::RHIBuffer* pMaterialDataBuffer )
    {
        RHI::RHITexture* pDefaultTexture = CoreResources::GetMissingTexture();

        MaterialData materialData{};
        materialData.m_surfaceFlags |= MATERIAL_USE_ALBEDO_TEXTURE;
        materialData.m_metalness = 0.0f;
        materialData.m_roughness = 0.0f;
        materialData.m_normalScaler = 1.0f;
        materialData.m_albedo = Float4::One;

        {
            auto* pMapped = pMaterialDataBuffer->MapTo<MaterialData*>( context.GetRHIDevice() );
            *pMapped = materialData;
            pMaterialDataBuffer->Unmap( context.GetRHIDevice() );
        }

        auto defaultTexBinding = RHI::RHITextureBinding{};
        defaultTexBinding.m_view = pDefaultTexture->GetOrCreateView( context.GetRHIDevice(), RHI::RHITextureViewCreateDesc{} );
        defaultTexBinding.m_layout = RHI::ETextureLayout::ShaderReadOnlyOptimal;

        RG::RGPipelineBinding const binding[] = {
            RG::BindRaw( { defaultTexBinding } ),
            RG::BindRaw( { defaultTexBinding } ),
            RG::BindRaw( { defaultTexBinding } ),
            RG::BindRaw( { defaultTexBinding } ),
            RG::BindRaw( { defaultTexBinding } )
        };
        boundPipeline.Bind( 1, binding );

        //renderContext.WriteToBuffer( pixelShader.GetConstBuffer( 1 ), &materialData, sizeof( materialData ) );

        //renderContext.SetShaderResource( PipelineStage::Pixel, 0, defaultSRV );
        //renderContext.SetShaderResource( PipelineStage::Pixel, 1, defaultSRV );
        //renderContext.SetShaderResource( PipelineStage::Pixel, 2, defaultSRV );
        //renderContext.SetShaderResource( PipelineStage::Pixel, 3, defaultSRV );
        //renderContext.SetShaderResource( PipelineStage::Pixel, 4, defaultSRV );
    }

    //-------------------------------------------------------------------------

    void WorldRenderer::SetupRenderStates( Viewport const& viewport, PixelShader* pShader, RenderData const& data )
    {
        //EE_ASSERT( pShader != nullptr && pShader->IsValid() );
        //auto const& renderContext = m_pRenderDevice->GetImmediateContext();

        //renderContext.SetViewport( Float2( viewport.GetDimensions() ), Float2( viewport.GetTopLeftPosition() ) );
        //renderContext.SetDepthTestMode( DepthTestMode::On );

        ////renderContext.SetSampler( PipelineStage::Pixel, 0, m_bilinearSampler );
        ////renderContext.SetSampler( PipelineStage::Pixel, 1, m_bilinearClampedSampler );
        ////renderContext.SetSampler( PipelineStage::Pixel, 2, m_shadowSampler );

        //renderContext.WriteToBuffer( pShader->GetConstBuffer( 0 ), &data.m_lightData, sizeof( data.m_lightData ) );

        //// Shadows
        //if ( data.m_lightData.m_lightingFlags & LIGHTING_ENABLE_SUN_SHADOW )
        //{
        //    renderContext.SetShaderResource( PipelineStage::Pixel, 10, m_shadowMap.GetShaderResourceView() );
        //}
        //else
        //{
        //    //renderContext.SetShaderResource( PipelineStage::Pixel, 10, CoreResources::GetMissingTexture()->GetShaderResourceView() );
        //}

        //// Skybox
        //if ( data.m_pSkyboxRadianceTexture )
        //{
        //    renderContext.SetShaderResource( PipelineStage::Pixel, 11, m_precomputedBRDF.GetShaderResourceView() );
        //    renderContext.SetShaderResource( PipelineStage::Pixel, 12, data.m_pSkyboxRadianceTexture->GetShaderResourceView() );
        //}
        //else
        //{
        //    //renderContext.SetShaderResource( PipelineStage::Pixel, 11, CoreResources::GetMissingTexture()->GetShaderResourceView() );
        //    renderContext.SetShaderResource( PipelineStage::Pixel, 12, ViewSRVHandle{} ); // TODO: fix add default cubemap resource
        //}
    }

    void WorldRenderer::ComputeBrdfLut( RG::RenderGraph& renderGraph )
    {
        auto node = renderGraph.AddNode( "Compute Brdf Lut" );

        auto brdfLutDesc = RG::TextureDesc::New2D( 512, 512, RHI::EPixelFormat::RG16Float );
        auto brdfLutResource = renderGraph.GetOrCreateNamedResource( "Brdf Lut", brdfLutDesc );

        //-------------------------------------------------------------------------

        RHI::RHIComputePipelineStateCreateDesc computePipelineDesc = {};
        computePipelineDesc.WithShader( RHI::RHIPipelineShader( ResourcePath( "data://shaders/engine/PrecomputeDFG.csdr" ) ) );
        computePipelineDesc.WithThreadCount( 512, 512, 1 );

        node.RegisterComputePipeline( eastl::move( computePipelineDesc ) );

        //-------------------------------------------------------------------------

        auto brdfLutBinding = node.CommonWrite( brdfLutResource, RHI::RenderResourceBarrierState::ComputeShaderWrite );

        // TODO: add full dispatch utility function
        node.Execute( [=] ( RG::RGRenderCommandContext& context )
        {
            auto boundPipeline = context.BindPipeline();
            RG::RGPipelineBinding bindings[] = {
                RG::Bind( brdfLutBinding )
            };
            boundPipeline.Bind( 0, bindings );
            context.Dispatch();
        } );
    }

    void WorldRenderer::RenderStaticMeshes( RG::RenderGraph& renderGraph, Viewport const& viewport, RenderTarget const& renderTarget )
    {
        EE_PROFILE_FUNCTION_RENDER();

        //auto const& renderContext = m_pRenderDevice->GetImmediateContext();

        if ( viewport.GetDimensions().m_x == 0 || viewport.GetDimensions().m_y <= 0 )
        {
            return;
        }

        if ( m_pRenderData->m_staticMeshComponents.empty() )
        {
            return;
        }

        // Set primary render state and clear the render buffer
        //-------------------------------------------------------------------------

        bool const bUsePickingPipeline = renderTarget.HasPickingRT();

        //RasterPipelineState* pPipelineState = renderTarget.HasPickingRT() ? &m_pipelineStateStaticPicking : &m_pipelineStateStatic;
        //SetupRenderStates( viewport, pPipelineState->m_pPixelShader, data );

        //renderContext.SetRasterPipelineState( *pPipelineState );
        //renderContext.SetShaderInputBinding( m_inputBindingStatic );
        //renderContext.SetPrimitiveTopology( Topology::TriangleList );

        //-------------------------------------------------------------------------
        constexpr uint32_t dirShadowMapWidth = 2048;
        constexpr uint32_t dirShadowMapHeight = 2048;

        auto dirShadowMapDesc = RG::TextureDesc::New2D( dirShadowMapWidth, dirShadowMapHeight, RHI::EPixelFormat::Depth32 );
        dirShadowMapDesc.m_desc.m_usage.SetFlag( RHI::ETextureUsage::DepthStencil );
        dirShadowMapDesc.m_desc.m_usage.SetFlag( RHI::ETextureUsage::Sampled );

        auto transformUniformBufferDesc = RG::BufferDesc::NewUniformBuffer( sizeof( ObjectTransforms ) );
        transformUniformBufferDesc.m_desc.m_memoryUsage = RHI::ERenderResourceMemoryUsage::CPUToGPU;
        transformUniformBufferDesc.m_desc.m_memoryFlag.SetFlag( RHI::ERenderResourceMemoryFlag::PersistentMapping );

        auto lightDataDesc = RG::BufferDesc::NewUniformBuffer( sizeof( LightData ) );
        lightDataDesc.m_desc.m_memoryUsage = RHI::ERenderResourceMemoryUsage::CPUToGPU;
        lightDataDesc.m_desc.m_memoryFlag.SetFlag( RHI::ERenderResourceMemoryFlag::PersistentMapping );

        auto materialDataDesc = RG::BufferDesc::NewUniformBuffer( sizeof( MaterialData ) );
        materialDataDesc.m_desc.m_memoryUsage = RHI::ERenderResourceMemoryUsage::CPUToGPU;
        materialDataDesc.m_desc.m_memoryFlag.SetFlag( RHI::ERenderResourceMemoryFlag::PersistentMapping );

        auto pickingDataDesc = RG::BufferDesc::NewUniformBuffer( sizeof( PickingData ) );
        pickingDataDesc.m_desc.m_memoryUsage = RHI::ERenderResourceMemoryUsage::CPUToGPU;
        pickingDataDesc.m_desc.m_memoryFlag.SetFlag( RHI::ERenderResourceMemoryFlag::PersistentMapping );

        EE_LOG_MESSAGE( "Render", "World Renderer", "Test: %p", m_pRenderData->m_pSkyboxTexture );

        {
            auto node = renderGraph.AddNode( bUsePickingPipeline ? "Static Mesh Lit Picking" : "Static Mesh Lit" );

            auto transformDataResource = renderGraph.CreateTemporaryResource( transformUniformBufferDesc );
            auto lightDataResource = renderGraph.CreateTemporaryResource( lightDataDesc );
            auto materialDataResource = renderGraph.CreateTemporaryResource( materialDataDesc );

            RG::RGResourceHandle<RG::RGResourceTagBuffer> pickingDataResource = {};
            if ( bUsePickingPipeline )
            {
                pickingDataResource = renderGraph.CreateTemporaryResource( pickingDataDesc );
            }

            auto brdfLutDesc = RG::TextureDesc::New2D( 512, 512, RHI::EPixelFormat::RG16Float );
            auto brdfLutResource = renderGraph.GetOrCreateNamedResource( "Brdf Lut", brdfLutDesc );

            auto dirShadowMapResource = renderGraph.GetOrCreateNamedResource( "Directional Light Shadow Map", dirShadowMapDesc );
            auto rtResource = renderGraph.ImportResource( renderTarget, RHI::RenderResourceBarrierState::Undefined );
            auto rtDepthResource = renderGraph.ImportResource( renderTarget.GetRHIDepthStencil(), RHI::RenderResourceBarrierState::Undefined );

            RG::RGResourceHandle<RG::RGResourceTagTexture> pickingRtResource = {};
            if ( bUsePickingPipeline )
            {
                auto pickingRtResource = renderGraph.ImportResource( renderTarget.GetRHIPickingRenderTarget(), RHI::RenderResourceBarrierState::Undefined );
            }

            //-------------------------------------------------------------------------

            RHI::RHIRasterPipelineStateCreateDesc rasterPipelineDesc = {};
            rasterPipelineDesc.AddShader( RHI::RHIPipelineShader( ResourcePath( "data://shaders/engine/StaticPrimitive.vsdr" ) ) );
            rasterPipelineDesc.AddShader( RHI::RHIPipelineShader( ResourcePath( bUsePickingPipeline ? "data://shaders/engine/LitPicking.psdr" : "data://shaders/engine/Lit.psdr" ) ) );
            rasterPipelineDesc.SetRasterizerState( RHI::RHIPipelineRasterizerState{} );
            rasterPipelineDesc.SetBlendState( RHI::RHIPipelineBlendState::NoBlend() );
            rasterPipelineDesc.SetRenderPass( bUsePickingPipeline ? m_pOpaquePickingRenderPass : m_pOpaqueRenderPass );
            rasterPipelineDesc.DepthTest( true );
            rasterPipelineDesc.DepthWrite( true );

            node.RegisterRasterPipeline( eastl::move( rasterPipelineDesc ) );

            //-------------------------------------------------------------------------

            auto transformDataBinding = node.RasterRead( transformDataResource, RHI::RenderResourceBarrierState::VertexShaderReadUniformBuffer );
            auto lightDataBinding = node.RasterRead( lightDataResource, RHI::RenderResourceBarrierState::VertexShaderReadUniformBuffer );
            auto materialDataBinding = node.RasterRead( materialDataResource, RHI::RenderResourceBarrierState::VertexShaderReadUniformBuffer );
            auto brdfLutBinding = node.RasterRead( brdfLutResource, RHI::RenderResourceBarrierState::FragmentShaderReadSampledImageOrUniformTexelBuffer );

            RG::RGNodeResourceRef<RG::RGResourceTagBuffer, RG::RGResourceViewType::SRV> pickingDataBinding = {};
            if ( bUsePickingPipeline )
            {
                pickingDataBinding = node.RasterRead( pickingDataResource, RHI::RenderResourceBarrierState::VertexShaderReadUniformBuffer );
            }

            RG::RGNodeResourceRef<RG::RGResourceTagTexture, RG::RGResourceViewType::SRV> sunShadowMapBinding = {};
            if ( m_pRenderData->m_lightData.m_lightingFlags & LIGHTING_ENABLE_SUN_SHADOW )
            {
                sunShadowMapBinding = node.RasterRead( dirShadowMapResource, RHI::RenderResourceBarrierState::VertexShaderReadSampledImageOrUniformTexelBuffer );
            }

            auto rtBinding = node.RasterWrite( rtResource, RHI::RenderResourceBarrierState::ColorAttachmentWrite );
            auto rtDepthBinding = node.RasterWrite( rtDepthResource, RHI::RenderResourceBarrierState::DepthAttachmentWriteStencilReadOnly );
            
            RG::RGNodeResourceRef<RG::RGResourceTagTexture, RG::RGResourceViewType::RT> pickingRtBinding = {};
            if ( bUsePickingPipeline )
            {
                pickingRtBinding = node.RasterWrite( pickingRtResource, RHI::RenderResourceBarrierState::ColorAttachmentWrite );
            }

            RHI::RHIRenderPass* pRenderPass = m_pOpaqueRenderPass;
            if ( bUsePickingPipeline )
            {
                pRenderPass = m_pOpaquePickingRenderPass;
            }

            uint32_t viewportWidth = (uint32_t) Math::FloorToInt( viewport.GetDimensions().m_x );
            uint32_t viewportHeight = (uint32_t) Math::FloorToInt( viewport.GetDimensions().m_y );
            int32_t viewportX = Math::FloorToInt( viewport.GetTopLeftPosition().m_x );
            int32_t viewportY = Math::FloorToInt( viewport.GetTopLeftPosition().m_y );

            node.Execute( [=] ( RG::RGRenderCommandContext& context )
            {
                auto const& rtDesc = context.GetDesc( rtBinding );

                if ( bUsePickingPipeline )
                {
                    RG::RGRenderTargetViewDesc rtViews[] = {
                        RG::RGRenderTargetViewDesc{ rtBinding, RHI::RHITextureViewCreateDesc{} },
                        RG::RGRenderTargetViewDesc{ pickingRtBinding, RHI::RHITextureViewCreateDesc{} },
                        RG::RGRenderTargetViewDesc{ rtDepthBinding, RHI::RHITextureViewCreateDesc{} }
                    };
                    EE_ASSERT( context.BeginRenderPass(
                        pRenderPass, Int2( rtDesc.m_width, rtDesc.m_height ),
                        rtViews
                    ) );
                }
                else
                {
                    RG::RGRenderTargetViewDesc rtViews[] = {
                        RG::RGRenderTargetViewDesc{ rtBinding, RHI::RHITextureViewCreateDesc{} },
                        RG::RGRenderTargetViewDesc{ rtDepthBinding, RHI::RHITextureViewCreateDesc{} }
                    };
                    EE_ASSERT( context.BeginRenderPass(
                        pRenderPass, Int2( rtDesc.m_width, rtDesc.m_height ),
                        rtViews
                    ) );
                }
                
                context.SetViewportAndScissor(
                    viewportWidth, viewportHeight,
                    viewportX, viewportY
                );
                auto boundPipeline = context.BindPipeline();

                //-------------------------------------------------------------------------

                auto* pLightDataBuffer = context.GetCompiledBufferResource( lightDataBinding );
                EE_ASSERT( pLightDataBuffer );

                {
                    auto* pMapped = pLightDataBuffer->Map( context.GetRHIDevice() );
                    memcpy( pMapped, &m_pRenderData->m_lightData, sizeof( m_pRenderData->m_lightData ) );
                    pLightDataBuffer->Unmap( context.GetRHIDevice() );
                }

                auto* pTransformBuffer = context.GetCompiledBufferResource( transformDataBinding );
                EE_ASSERT( pTransformBuffer );
                RHI::RHIBuffer* pPickingBuffer = nullptr;
                if ( bUsePickingPipeline )
                {
                    pPickingBuffer = context.GetCompiledBufferResource( pickingDataBinding );
                    EE_ASSERT( pPickingBuffer );
                }
                auto* pMaterialDataBuffer = context.GetCompiledBufferResource( materialDataBinding );
                EE_ASSERT( pMaterialDataBuffer );

                auto defaultTexBinding = RHI::RHITextureBinding{};
                defaultTexBinding.m_view = CoreResources::GetMissingTexture()->GetOrCreateView( context.GetRHIDevice(), RHI::RHITextureViewCreateDesc{} );
                defaultTexBinding.m_layout = RHI::ETextureLayout::ShaderReadOnlyOptimal;

                auto envMapBinding = RHI::RHITextureBinding{};
                envMapBinding.m_view = m_pRenderData->m_pSkyboxRadianceTexture ?
                    m_pRenderData->m_pSkyboxRadianceTexture->GetRHITexture()->GetOrCreateView( context.GetRHIDevice(), RHI::RHITextureViewCreateDesc{} ) : 
                    CoreResources::GetMissingTexture()->GetOrCreateView( context.GetRHIDevice(), RHI::RHITextureViewCreateDesc{} );
                envMapBinding.m_layout = RHI::ETextureLayout::ShaderReadOnlyOptimal;

                for ( StaticMeshComponent const* pMeshComponent : m_pRenderData->m_staticMeshComponents )
                {
                    {
                        Vector const finalScale = pMeshComponent->GetLocalScale() * pMeshComponent->GetWorldTransform().GetScale();
                        Matrix const worldTransform = Matrix( pMeshComponent->GetWorldTransform().GetRotation(), pMeshComponent->GetWorldTransform().GetTranslation(), finalScale );

                        ObjectTransforms transforms = m_pRenderData->m_transforms;
                        transforms.m_worldTransform = worldTransform;
                        transforms.m_worldTransform.SetTranslation( worldTransform.GetTranslation() );
                        transforms.m_normalTransform = transforms.m_worldTransform.GetInverse().GetTransposed();

                        auto* pMapped = pTransformBuffer->MapTo<ObjectTransforms*>( context.GetRHIDevice() );
                        *pMapped = transforms;
                        pTransformBuffer->Unmap( context.GetRHIDevice() );
                    }

                    if ( bUsePickingPipeline )
                    {
                        PickingData const pickingData( pMeshComponent->GetEntityID().m_value, pMeshComponent->GetID().m_value );

                        auto* pMapped = pPickingBuffer->MapTo<PickingData*>( context.GetRHIDevice() );
                        *pMapped = pickingData;
                        pPickingBuffer->Unmap( context.GetRHIDevice() );
                    }
                    
                    //-------------------------------------------------------------------------

                    auto* pMesh = pMeshComponent->GetMesh();
                    RHI::RHIBuffer const* vertexBuffers[] = { pMesh->GetVertexBuffer() };
                    context.BindVertexBuffer( 0, vertexBuffers );
                    context.BindIndexBuffer( pMesh->GetIndexBuffer() );

                    if ( m_pRenderData->m_lightData.m_lightingFlags & LIGHTING_ENABLE_SUN_SHADOW )
                    {
                        RG::RGPipelineBinding const bindings[] = {
                            RG::Bind( sunShadowMapBinding ),
                            RG::Bind( brdfLutBinding ),
                            RG::BindRaw( envMapBinding ),
                            RG::Bind( transformDataBinding ),
                            RG::Bind( lightDataBinding ),
                            RG::Bind( materialDataBinding ),
                            RG::Bind( pickingDataBinding )
                        };
                        boundPipeline.Bind( 0, bindings );
                    }
                    else
                    {
                        RG::RGPipelineBinding const bindings[] = {
                            RG::BindRaw( defaultTexBinding ),
                            RG::Bind( brdfLutBinding ),
                            RG::BindRaw( envMapBinding ),
                            RG::Bind( transformDataBinding ),
                            RG::Bind( lightDataBinding ),
                            RG::Bind( materialDataBinding ),
                            RG::Bind( pickingDataBinding )
                        };
                        boundPipeline.Bind( 0, bindings );
                    }

                    //-------------------------------------------------------------------------

                    TVector<Material const*> const& materials = pMeshComponent->GetMaterials();

                    auto const numSubMeshes = pMesh->GetNumSections();
                    for ( auto i = 0u; i < numSubMeshes; i++ )
                    {
                        if ( i < materials.size() && materials[i] )
                        {
                            SetMaterial( context, boundPipeline, pMaterialDataBuffer, materials[i] );
                        }
                        else // Use default material
                        {
                            SetDefaultMaterial( context, boundPipeline, pMaterialDataBuffer );
                        }

                        auto const& subMesh = pMesh->GetSection( i );
                        context.DrawIndexed( subMesh.m_numIndices, 1, subMesh.m_startIndex );
                    }
                }

                context.EndRenderPass();
            } );
        }

        //-------------------------------------------------------------------------

        //for ( StaticMeshComponent const* pMeshComponent : data.m_staticMeshComponents )
        //{
        //    auto pMesh = pMeshComponent->GetMesh();
        //    Vector const finalScale = pMeshComponent->GetLocalScale() * pMeshComponent->GetWorldTransform().GetScale();
        //    Matrix const worldTransform = Matrix( pMeshComponent->GetWorldTransform().GetRotation(), pMeshComponent->GetWorldTransform().GetTranslation(), finalScale );

        //    ObjectTransforms transforms = data.m_transforms;
        //    transforms.m_worldTransform = worldTransform;
        //    transforms.m_worldTransform.SetTranslation( worldTransform.GetTranslation() );
        //    transforms.m_normalTransform = transforms.m_worldTransform.GetInverse().Transpose();
        //    //renderContext.WriteToBuffer( m_vertexShaderStatic.GetConstBuffer( 0 ), &transforms, sizeof( transforms ) );

        //    if ( renderTarget.HasPickingRT() )
        //    {
        //        PickingData const pd( pMeshComponent->GetEntityID().m_value, pMeshComponent->GetID().m_value );
        //        //renderContext.WriteToBuffer( m_pixelShaderPicking.GetConstBuffer( 2 ), &pd, sizeof( PickingData ) );
        //    }

        //    //renderContext.SetVertexBuffer( pMesh->GetVertexBuffer() );
        //    //renderContext.SetIndexBuffer( pMesh->GetIndexBuffer() );

        //    TVector<Material const*> const& materials = pMeshComponent->GetMaterials();

        //    auto const numSubMeshes = pMesh->GetNumSections();
        //    for ( auto i = 0u; i < numSubMeshes; i++ )
        //    {
        //        //if ( i < materials.size() && materials[i] )
        //        //{
        //        //    SetMaterial( renderContext, *pPipelineState->m_pPixelShader, materials[i] );
        //        //}
        //        //else // Use default material
        //        //{
        //        //    SetDefaultMaterial( renderContext, *pPipelineState->m_pPixelShader );
        //        //}

        //        auto const& subMesh = pMesh->GetSection( i );
        //        //renderContext.DrawIndexed( subMesh.m_numIndices, subMesh.m_startIndex );
        //    }
        //}
        ////renderContext.ClearShaderResource( PipelineStage::Pixel, 10 );
    }

    void WorldRenderer::RenderSkeletalMeshes( Viewport const& viewport, RenderTarget const& renderTarget )
    {
        EE_PROFILE_FUNCTION_RENDER();

        auto const& renderContext = m_pRenderDevice->GetImmediateContext();

        // Set primary render state and clear the render buffer
        //-------------------------------------------------------------------------

        //RasterPipelineState* pPipelineState = renderTarget.HasPickingRT() ? &m_pipelineStateSkeletalPicking : &m_pipelineStateSkeletal;
        //SetupRenderStates( viewport, pPipelineState->m_pPixelShader, data );

        //renderContext.SetRasterPipelineState( *pPipelineState );
        //renderContext.SetShaderInputBinding( m_inputBindingSkeletal );
        //renderContext.SetPrimitiveTopology( Topology::TriangleList );

        //-------------------------------------------------------------------------

        SkeletalMesh const* pCurrentMesh = nullptr;

        //for ( SkeletalMeshComponent const* pMeshComponent : data.m_skeletalMeshComponents )
        //{
        //    if ( pMeshComponent->GetMesh() != pCurrentMesh )
        //    {
        //        pCurrentMesh = pMeshComponent->GetMesh();
        //        EE_ASSERT( pCurrentMesh != nullptr && pCurrentMesh->IsValid() );

        //        //renderContext.SetVertexBuffer( pCurrentMesh->GetVertexBuffer() );
        //        //renderContext.SetIndexBuffer( pCurrentMesh->GetIndexBuffer() );
        //    }

        //    // Update Bones and Transforms
        //    //-------------------------------------------------------------------------

        //    Matrix worldTransform = pMeshComponent->GetWorldTransform().ToMatrix();
        //    ObjectTransforms transforms = data.m_transforms;
        //    transforms.m_worldTransform = worldTransform;
        //    transforms.m_worldTransform.SetTranslation( worldTransform.GetTranslation() );
        //    transforms.m_normalTransform = transforms.m_worldTransform.GetInverse().Transpose();
        //    //renderContext.WriteToBuffer( m_vertexShaderSkeletal.GetConstBuffer( 0 ), &transforms, sizeof( transforms ) );

        //    //auto const& bonesConstBuffer = m_vertexShaderSkeletal.GetConstBuffer( 1 );
        //    auto const& boneTransforms = pMeshComponent->GetSkinningTransforms();
        //    EE_ASSERT( boneTransforms.size() == pCurrentMesh->GetNumBones() );
        //    //renderContext.WriteToBuffer( bonesConstBuffer, boneTransforms.data(), sizeof( Matrix ) * pCurrentMesh->GetNumBones() );

        //    if ( renderTarget.HasPickingRT() )
        //    {
        //        PickingData const pd( pMeshComponent->GetEntityID().m_value, pMeshComponent->GetID().m_value );
        //        renderContext.WriteToBuffer( m_pixelShaderPicking.GetConstBuffer( 2 ), &pd, sizeof( PickingData ) );
        //    }

        //    // Draw sub-meshes
        //    //-------------------------------------------------------------------------

        //    TVector<Material const*> const& materials = pMeshComponent->GetMaterials();

        //    auto const numSubMeshes = pCurrentMesh->GetNumSections();
        //    for ( auto i = 0u; i < numSubMeshes; i++ )
        //    {
        //        //if ( i < materials.size() && materials[i] )
        //        //{
        //        //    SetMaterial( renderContext, *pPipelineState->m_pPixelShader, materials[i] );
        //        //}
        //        //else // Use default material
        //        //{
        //        //    SetDefaultMaterial( renderContext, *pPipelineState->m_pPixelShader );
        //        //}

        //        // Draw mesh
        //        auto const& subMesh = pCurrentMesh->GetSection( i );
        //        renderContext.DrawIndexed( subMesh.m_numIndices, subMesh.m_startIndex );
        //    }
        //}
        renderContext.ClearShaderResource( PipelineStage::Pixel, 10 );
    }

    void WorldRenderer::RenderSkybox( RG::RenderGraph& renderGraph, Viewport const& viewport, RenderTarget const& renderTarget )
    {
        EE_PROFILE_FUNCTION_RENDER();

        if ( m_pRenderData->m_pSkyboxTexture )
        {
            //renderContext.SetViewport( Float2( viewport.GetDimensions() ), Float2( viewport.GetTopLeftPosition() ), Float2( 1, 1 )/*TODO: fix for inv z*/ );
            //renderContext.SetRasterPipelineState( m_pipelineSkybox );
            //renderContext.SetShaderInputBinding( ShaderInputBindingHandle() );
            //renderContext.SetPrimitiveTopology( Topology::TriangleStrip );
            //renderContext.WriteToBuffer( m_vertexShaderSkybox.GetConstBuffer( 0 ), &skyboxTransform, sizeof( Matrix ) );
            //renderContext.WriteToBuffer( m_pixelShaderSkybox.GetConstBuffer( 0 ), &data.m_lightData, sizeof( data.m_lightData ) );
            //renderContext.SetShaderResource( PipelineStage::Pixel, 0, data.m_pSkyboxTexture->GetShaderResourceView() );
            //renderContext.Draw( 14, 0 );

            auto node = renderGraph.AddNode( "Draw Skybox" );

            auto transformDataDesc = RG::BufferDesc::NewUniformBuffer( sizeof( Matrix ) );
            transformDataDesc.m_desc.m_memoryUsage = RHI::ERenderResourceMemoryUsage::CPUToGPU;
            transformDataDesc.m_desc.m_memoryFlag.SetFlag( RHI::ERenderResourceMemoryFlag::PersistentMapping );
            auto transformDataResource = renderGraph.CreateTemporaryResource( transformDataDesc );

            auto lightDataDesc = RG::BufferDesc::NewUniformBuffer( sizeof( LightData ) );
            lightDataDesc.m_desc.m_memoryUsage = RHI::ERenderResourceMemoryUsage::CPUToGPU;
            lightDataDesc.m_desc.m_memoryFlag.SetFlag( RHI::ERenderResourceMemoryFlag::PersistentMapping );
            auto lightDataResource = renderGraph.CreateTemporaryResource( lightDataDesc );

            auto skyboxResource = renderGraph.ImportResource( m_pRenderData->m_pSkyboxTexture->GetRHITexture(), RHI::RenderResourceBarrierState::Undefined );

            auto rtResource = renderGraph.ImportResource( renderTarget, RHI::RenderResourceBarrierState::Undefined );
            auto rtDepthResource = renderGraph.ImportResource( renderTarget.GetRHIDepthStencil(), RHI::RenderResourceBarrierState::Undefined );

            //-------------------------------------------------------------------------

            RHI::RHIRasterPipelineStateCreateDesc rasterPipelineDesc = {};
            rasterPipelineDesc.AddShader( RHI::RHIPipelineShader( ResourcePath( "data://shaders/engine/Cube.vsdr" ) ) );
            rasterPipelineDesc.AddShader( RHI::RHIPipelineShader( ResourcePath( "data://shaders/engine/Skybox.psdr" ) ) );
            rasterPipelineDesc.SetRasterizerState( RHI::RHIPipelineRasterizerState{} );
            rasterPipelineDesc.SetBlendState( RHI::RHIPipelineBlendState::NoBlend() );
            rasterPipelineDesc.SetRenderPass( m_pOpaqueRenderPass );
            rasterPipelineDesc.DepthTest( true );
            rasterPipelineDesc.DepthWrite( false );

            node.RegisterRasterPipeline( eastl::move( rasterPipelineDesc ) );

            //-------------------------------------------------------------------------

            auto transformDataBinding = node.RasterRead( transformDataResource, RHI::RenderResourceBarrierState::FragmentShaderReadUniformBuffer );
            auto lightDataBinding = node.RasterRead( lightDataResource, RHI::RenderResourceBarrierState::FragmentShaderReadUniformBuffer );
            auto skyboxBinding = node.RasterRead( skyboxResource, RHI::RenderResourceBarrierState::FragmentShaderReadSampledImageOrUniformTexelBuffer );

            auto rtBinding = node.RasterWrite( rtResource, RHI::RenderResourceBarrierState::ColorAttachmentWrite );
            auto rtDepthBinding = node.RasterWrite( rtDepthResource, RHI::RenderResourceBarrierState::DepthStencilAttachmentRead );

            //-------------------------------------------------------------------------

            auto* pRenderPass = m_pOpaqueRenderPass;

            uint32_t viewportWidth = (uint32_t) Math::FloorToInt( viewport.GetDimensions().m_x );
            uint32_t viewportHeight = (uint32_t) Math::FloorToInt( viewport.GetDimensions().m_y );
            int32_t viewportX = Math::FloorToInt( viewport.GetTopLeftPosition().m_x );
            int32_t viewportY = Math::FloorToInt( viewport.GetTopLeftPosition().m_y );

            Matrix const skyboxTransform = Matrix( Quaternion::Identity, viewport.GetViewPosition(), Vector::One ) * m_pRenderData->m_transforms.m_viewprojTransform;

            node.Execute( [=] ( RG::RGRenderCommandContext& context )
            {
                {
                    auto* pTransformBuffer = context.GetCompiledBufferResource( transformDataBinding );
                    EE_ASSERT( pTransformBuffer );

                    auto* pMapped = pTransformBuffer->MapTo<Matrix*>( context.GetRHIDevice() );
                    *pMapped = skyboxTransform;
                    pTransformBuffer->Unmap( context.GetRHIDevice() );
                }

                {
                    auto* pLightDataBuffer = context.GetCompiledBufferResource( lightDataBinding );
                    EE_ASSERT( pLightDataBuffer );

                    auto* pMapped = pLightDataBuffer->MapTo<LightData*>( context.GetRHIDevice() );
                    *pMapped = m_pRenderData->m_lightData;
                    pLightDataBuffer->Unmap( context.GetRHIDevice() );
                }

                //-------------------------------------------------------------------------

                auto const& rtDesc = context.GetDesc( rtBinding );

                RHI::RenderPassClearValue clearValue;
                clearValue.m_depth = 0.0f; // Note: reverse z
                RG::RGRenderTargetViewDesc rtViews[] = {
                    RG::RGRenderTargetViewDesc{ rtBinding, RHI::RHITextureViewCreateDesc{} },
                    RG::RGRenderTargetViewDesc{ rtDepthBinding, RHI::RHITextureViewCreateDesc{} }
                };
                EE_ASSERT( context.BeginRenderPassWithClearValue(
                    pRenderPass, Int2( rtDesc.m_width, rtDesc.m_height ),
                    clearValue,
                    rtViews, {}
                ) );

                context.SetViewportAndScissor(
                    viewportWidth, viewportHeight,
                    viewportX, viewportY
                );

                auto boundPipeline = context.BindPipeline();
                RG::RGPipelineBinding bindings[] = {
                    RG::Bind( transformDataBinding ),
                    RG::Bind( skyboxBinding ),
                    RG::Bind( lightDataBinding )
                };
                boundPipeline.Bind( 0, bindings );

                context.Draw( 14 );

                context.EndRenderPass();
            } );
        }
    }

    void WorldRenderer::RenderSunShadows( RG::RenderGraph& renderGraph, Viewport const& viewport, DirectionalLightComponent* pDirectionalLightComponent )
    {
        EE_PROFILE_FUNCTION_RENDER();

        if ( viewport.GetDimensions().m_x <= 0 || viewport.GetDimensions().m_y <= 0 )
        {
            return;
        }

        if ( !pDirectionalLightComponent || !pDirectionalLightComponent->GetShadowed() )
        {
            return;
        }

        //-------------------------------------------------------------------------

        // TODO: hardcode width and height, should be configurable
        constexpr uint32_t dirShadowMapWidth = 2048;
        constexpr uint32_t dirShadowMapHeight = 2048;

        auto dirShadowMapDesc = RG::TextureDesc::New2D( dirShadowMapWidth, dirShadowMapHeight, RHI::EPixelFormat::Depth32 );
        dirShadowMapDesc.m_desc.m_usage.SetFlag( RHI::ETextureUsage::DepthStencil );
        dirShadowMapDesc.m_desc.m_usage.SetFlag( RHI::ETextureUsage::Sampled );

        auto transformUniformBufferDesc = RG::BufferDesc::NewUniformBuffer( sizeof( ObjectTransforms ) );
        // TODO: extract common pattern
        transformUniformBufferDesc.m_desc.m_memoryUsage = RHI::ERenderResourceMemoryUsage::CPUToGPU;
        transformUniformBufferDesc.m_desc.m_memoryFlag.SetFlag( RHI::ERenderResourceMemoryFlag::PersistentMapping );

        EE_LOG_MESSAGE( "Render", "World Renderer", "Test: %p", m_pRenderData->m_pSkyboxTexture );

        {
            auto node = renderGraph.AddNode( "Static Mesh Sun Shadows" );

            auto dirShadowMapResource = renderGraph.GetOrCreateNamedResource( "Directional Light Shadow Map", dirShadowMapDesc );
            auto transformUniformBufferResource = renderGraph.CreateTemporaryResource( transformUniformBufferDesc );

            //-------------------------------------------------------------------------

            RHI::RHIRasterPipelineStateCreateDesc rasterPipelineDesc = {};
            rasterPipelineDesc.AddShader( RHI::RHIPipelineShader( ResourcePath( "data://shaders/engine/StaticPrimitive.vsdr" ) ) );
            rasterPipelineDesc.AddShader( RHI::RHIPipelineShader( ResourcePath( "data://shaders/engine/Empty.psdr" ) ) );
            rasterPipelineDesc.SetRasterizerState( RHI::RHIPipelineRasterizerState{} );
            rasterPipelineDesc.SetBlendState( RHI::RHIPipelineBlendState::NoBlend() );
            rasterPipelineDesc.SetRenderPass( m_pShadowRenderPass );
            rasterPipelineDesc.DepthTest( true );
            rasterPipelineDesc.DepthWrite( true );

            node.RegisterRasterPipeline( eastl::move( rasterPipelineDesc ) );

            //-------------------------------------------------------------------------
        
            auto transformBufferBinding = node.RasterRead( transformUniformBufferResource, RHI::RenderResourceBarrierState::VertexShaderReadUniformBuffer );
            auto shadowMapBinding = node.RasterWrite( dirShadowMapResource, RHI::RenderResourceBarrierState::DepthStencilAttachmentWrite );
        
            RHI::RHIRenderPass* pRenderPass = m_pShadowRenderPass;

            node.Execute( [=] ( RG::RGRenderCommandContext& context )
            {
                context.ClearDepthStencil( shadowMapBinding );
            
                //-------------------------------------------------------------------------

                auto const& shadowMapDesc = context.GetDesc( shadowMapBinding );

                RG::RGRenderTargetViewDesc rtViews[] = {
                    RG::RGRenderTargetViewDesc{ shadowMapBinding, RHI::RHITextureViewCreateDesc{} }
                };
                EE_ASSERT( context.BeginRenderPass(
                    pRenderPass, Int2( shadowMapDesc.m_width, shadowMapDesc.m_height ),
                    rtViews
                ) );

                context.SetViewportAndScissor( shadowMapDesc.m_width, shadowMapDesc.m_height );
                auto boundPipeline = context.BindPipeline();

                // Static Mesh Pass
                //-------------------------------------------------------------------------

                auto* pTransformBuffer = context.GetCompiledBufferResource( transformBufferBinding );
                EE_ASSERT( pTransformBuffer );

                ObjectTransforms transforms;
                transforms.m_viewprojTransform = m_pRenderData->m_lightData.m_sunShadowMapMatrix;

                for ( StaticMeshComponent const* pMeshComponent : m_pRenderData->m_staticMeshComponents )
                {
                    Matrix worldTransform = pMeshComponent->GetWorldTransform().ToMatrix();
                    transforms.m_worldTransform = worldTransform;

                    auto* pMapped = pTransformBuffer->MapTo<ObjectTransforms*>( context.GetRHIDevice() );
                    *pMapped = transforms;
                    pTransformBuffer->Unmap( context.GetRHIDevice() );

                    RG::RGPipelineBinding const binding[] = {
                        RG::Bind( transformBufferBinding )
                    };
                    boundPipeline.Bind( 0, binding );

                    //-------------------------------------------------------------------------

                    auto* pMesh = pMeshComponent->GetMesh();
                    RHI::RHIBuffer const* vertexBuffers[] = { pMesh->GetVertexBuffer() };
                    context.BindVertexBuffer( 0, vertexBuffers );
                    context.BindIndexBuffer( pMesh->GetIndexBuffer() );

                    auto const numSubMeshes = pMesh->GetNumSections();
                    for ( auto i = 0u; i < numSubMeshes; i++ )
                    {
                        auto const& subMesh = pMesh->GetSection( i );
                        context.DrawIndexed( subMesh.m_numIndices, 1, subMesh.m_startIndex );
                    }
                }

                context.EndRenderPass();
            });
        }

        //{
        //    auto node = renderGraph.AddNode( "Skeletal Mesh Sun Shadows" );

        //    auto boneTransformBufferDesc = RG::BufferDesc::NewUniformBuffer( sizeof( Matrix ) * 255 ); // ( 1 WVP matrix + 255 bone matrices )
        //    boneTransformBufferDesc.m_desc.m_memoryUsage = RHI::ERenderResourceMemoryUsage::CPUToGPU;
        //    boneTransformBufferDesc.m_desc.m_memoryFlag.SetFlag( RHI::ERenderResourceMemoryFlag::PersistentMapping );

        //    auto dirShadowMapResource = renderGraph.GetOrCreateNamedResource( "Directional Light Shadow Map", dirShadowMapDesc );
        //    auto transformBufferResource = renderGraph.CreateTemporaryResource( transformUniformBufferDesc );
        //    auto boneTransformBufferResource = renderGraph.CreateTemporaryResource( boneTransformBufferDesc );

        //    //-------------------------------------------------------------------------

        //    RHI::RHIRasterPipelineStateCreateDesc rasterPipelineDesc = {};
        //    rasterPipelineDesc.AddShader( RHI::RHIPipelineShader( ResourcePath( "data://shaders/engine/SkinnedPrimitive.vsdr" ) ) );
        //    rasterPipelineDesc.AddShader( RHI::RHIPipelineShader( ResourcePath( "data://shaders/engine/Empty.psdr" ) ) );
        //    rasterPipelineDesc.SetRasterizerState( RHI::RHIPipelineRasterizerState{} );
        //    rasterPipelineDesc.SetBlendState( RHI::RHIPipelineBlendState::NoBlend() );
        //    rasterPipelineDesc.SetRenderPass( m_pShadowRenderPass );
        //    rasterPipelineDesc.DepthTest( true );
        //    rasterPipelineDesc.DepthWrite( true );

        //    node.RegisterRasterPipeline( eastl::move( rasterPipelineDesc ) );

        //    //-------------------------------------------------------------------------

        //    auto transformBinding = node.RasterRead( transformBufferResource, RHI::RenderResourceBarrierState::VertexShaderReadUniformBuffer );
        //    auto boneTransformBinding = node.RasterRead( boneTransformBufferResource, RHI::RenderResourceBarrierState::VertexShaderReadUniformBuffer );
        //    auto shadowMapBinding = node.RasterWrite( dirShadowMapResource, RHI::RenderResourceBarrierState::DepthStencilAttachmentWrite );

        //    RHI::RHIRenderPass* pRenderPass = m_pShadowRenderPass;

        //    node.Execute( [=] ( RG::RGRenderCommandContext& context )
        //    {
        //        auto const& shadowMapDesc = context.GetDesc( shadowMapBinding );

        //        RG::RGRenderTargetViewDesc rtViews[] = {
        //            RG::RGRenderTargetViewDesc{ shadowMapBinding, RHI::RHITextureViewCreateDesc{} }
        //        };
        //        EE_ASSERT( context.BeginRenderPass(
        //            pRenderPass, Int2( shadowMapDesc.m_width, shadowMapDesc.m_height ),
        //            rtViews
        //        ) );

        //        context.SetViewportAndScissor( shadowMapDesc.m_width, shadowMapDesc.m_height );
        //        auto boundPipeline = context.BindPipeline();

        //        // Skeletal Mesh Pass
        //        //-------------------------------------------------------------------------

        //        auto* pTransformBuffer = context.GetCompiledBufferResource( transformBinding );
        //        auto* pBoneTransformBuffer = context.GetCompiledBufferResource( boneTransformBinding );
        //        EE_ASSERT( pTransformBuffer );
        //        EE_ASSERT( pBoneTransformBuffer );

        //        ObjectTransforms transforms;
        //        transforms.m_viewprojTransform = m_pRenderData->m_lightData.m_sunShadowMapMatrix;

        //        for ( SkeletalMeshComponent const* pMeshComponent : m_pRenderData->m_skeletalMeshComponents )
        //        {
        //            Matrix worldTransform = pMeshComponent->GetWorldTransform().ToMatrix();
        //            transforms.m_worldTransform = worldTransform;
        //            transforms.m_worldTransform.SetTranslation( worldTransform.GetTranslation() );

        //            {
        //                auto* pMapped = pTransformBuffer->MapTo<ObjectTransforms*>( context.GetRHIDevice() );
        //                *pMapped = transforms;
        //                pTransformBuffer->Unmap( context.GetRHIDevice() );
        //            }

        //            auto* pMesh = pMeshComponent->GetMesh();
        //            auto const& boneTransforms = pMeshComponent->GetSkinningTransforms();
        //            EE_ASSERT( boneTransforms.size() == pMesh->GetNumBones() );

        //            {
        //                auto* pMapped = pBoneTransformBuffer->Map( context.GetRHIDevice() );
        //                memcpy( pMapped, boneTransforms.data(), pMesh->GetNumBones() * sizeof( Matrix ) );
        //                pBoneTransformBuffer->Unmap( context.GetRHIDevice() );
        //            }

        //            RG::RGPipelineBinding const binding[] = {
        //                RG::Bind( transformBinding ),
        //                RG::Bind( boneTransformBinding )
        //            };
        //            boundPipeline.Bind( 0, binding );

        //            //-------------------------------------------------------------------------

        //            RHI::RHIBuffer const* vertexBuffers[] = { pMesh->GetVertexBuffer() };
        //            context.BindVertexBuffer( 0, vertexBuffers );
        //            context.BindIndexBuffer( pMesh->GetIndexBuffer() );

        //            auto const numSubMeshes = pMesh->GetNumSections();
        //            for ( auto i = 0u; i < numSubMeshes; i++ )
        //            {
        //                auto const& subMesh = pMesh->GetSection( i );
        //                context.DrawIndexed( subMesh.m_numIndices, 1, subMesh.m_startIndex );
        //            }
        //        }

        //        context.EndRenderPass();
        //    } );
        //}


        // Set primary render state and clear the render buffer
        //-------------------------------------------------------------------------

        //renderContext.ClearDepthStencilView( m_shadowMap.GetDepthStencilView(), 1.0f/*TODO: inverse z*/, 0 );
        //renderContext.SetRenderTarget( m_shadowMap.GetDepthStencilView() );
        //renderContext.SetViewport( Float2( (float) m_shadowMap.GetDimensions().m_x, (float) m_shadowMap.GetDimensions().m_y ), Float2( 0.0f, 0.0f ) );
        //renderContext.SetDepthTestMode( DepthTestMode::On );

        //ObjectTransforms transforms;
        //transforms.m_viewprojTransform = m_pRenderData->m_lightData.m_sunShadowMapMatrix;

        // Static Meshes
        //-------------------------------------------------------------------------

        //renderContext.SetRasterPipelineState( m_pipelineStateStaticShadow );
        //renderContext.SetShaderInputBinding( m_inputBindingStatic );
        //renderContext.SetPrimitiveTopology( Topology::TriangleList );

        //for ( StaticMeshComponent const* pMeshComponent : m_pRenderData->m_staticMeshComponents )
        //{
        //    auto pMesh = pMeshComponent->GetMesh();
        //    Matrix worldTransform = pMeshComponent->GetWorldTransform().ToMatrix();
        //    transforms.m_worldTransform = worldTransform;
        //    //renderContext.WriteToBuffer( m_vertexShaderStatic.GetConstBuffer( 0 ), &transforms, sizeof( transforms ) );

        //    //renderContext.SetVertexBuffer( pMesh->GetVertexBuffer() );
        //    //renderContext.SetIndexBuffer( pMesh->GetIndexBuffer() );

        //    auto const numSubMeshes = pMesh->GetNumSections();
        //    for ( auto i = 0u; i < numSubMeshes; i++ )
        //    {
        //        auto const& subMesh = pMesh->GetSection( i );
        //        //renderContext.DrawIndexed( subMesh.m_numIndices, subMesh.m_startIndex );
        //    }
        //}

        // Skeletal Meshes
        //-------------------------------------------------------------------------

        //renderContext.SetRasterPipelineState( m_pipelineStateSkeletalShadow );
        //renderContext.SetShaderInputBinding( m_inputBindingSkeletal );
        //renderContext.SetPrimitiveTopology( Topology::TriangleList );

        //for ( SkeletalMeshComponent const* pMeshComponent : m_pRenderData->m_skeletalMeshComponents )
        //{
        //    auto pMesh = pMeshComponent->GetMesh();

        //    // Update Bones and Transforms
        //    //-------------------------------------------------------------------------

        //    Matrix worldTransform = pMeshComponent->GetWorldTransform().ToMatrix();
        //    //transforms.m_worldTransform = worldTransform;
        //    //transforms.m_worldTransform.SetTranslation( worldTransform.GetTranslation() );
        //    //renderContext.WriteToBuffer( m_vertexShaderSkeletal.GetConstBuffer( 0 ), &transforms, sizeof( transforms ) );

        //    //auto const& bonesConstBuffer = m_vertexShaderSkeletal.GetConstBuffer( 1 );
        //    auto const& boneTransforms = pMeshComponent->GetSkinningTransforms();
        //    EE_ASSERT( boneTransforms.size() == pMesh->GetNumBones() );
        //    //renderContext.WriteToBuffer( bonesConstBuffer, boneTransforms.data(), sizeof( Matrix ) * pMesh->GetNumBones() );

        //    //renderContext.SetVertexBuffer( pMesh->GetVertexBuffer() );
        //    //renderContext.SetIndexBuffer( pMesh->GetIndexBuffer() );

        //    // Draw sub-meshes
        //    //-------------------------------------------------------------------------
        //    auto const numSubMeshes = pMesh->GetNumSections();
        //    for ( auto i = 0u; i < numSubMeshes; i++ )
        //    {
        //        // Draw mesh
        //        auto const& subMesh = pMesh->GetSection( i );
        //        //renderContext.DrawIndexed( subMesh.m_numIndices, subMesh.m_startIndex );
        //    }
        //}
    }

    //-------------------------------------------------------------------------

    void WorldRenderer::RenderWorld( Seconds const deltaTime, Viewport const& viewport, RenderTarget const& renderTarget, EntityWorld* pWorld )
    {
        EE_ASSERT( IsInitialized() && Threading::IsMainThread() );
        EE_PROFILE_FUNCTION_RENDER();

        if ( !viewport.IsValid() )
        {
            return;
        }

        //-------------------------------------------------------------------------

        auto pWorldSystem = pWorld->GetWorldSystem<RendererWorldSystem>();
        EE_ASSERT( pWorldSystem != nullptr );

        //-------------------------------------------------------------------------

        RenderData renderData
        {
            ObjectTransforms(),
            LightData(),
            nullptr,
            nullptr,
            pWorldSystem->m_visibleStaticMeshComponents,
            pWorldSystem->m_visibleSkeletalMeshComponents,
        };

        renderData.m_transforms.m_viewprojTransform = viewport.GetViewVolume().GetViewProjectionMatrix();

        //-------------------------------------------------------------------------

        uint32_t lightingFlags = 0;

        DirectionalLightComponent* pDirectionalLightComponent = nullptr;
        if ( !pWorldSystem->m_registeredDirectionLightComponents.empty() )
        {
            pDirectionalLightComponent = pWorldSystem->m_registeredDirectionLightComponents[0];
            lightingFlags |= LIGHTING_ENABLE_SUN;
            lightingFlags |= pDirectionalLightComponent->GetShadowed() ? LIGHTING_ENABLE_SUN_SHADOW : 0;
            renderData.m_lightData.m_SunDirIndirectIntensity = -pDirectionalLightComponent->GetLightDirection();
            Float4 colorIntensity = pDirectionalLightComponent->GetLightColor();
            renderData.m_lightData.m_SunColorRoughnessOneLevel = colorIntensity * pDirectionalLightComponent->GetLightIntensity();
            // TODO: conditional
            renderData.m_lightData.m_sunShadowMapMatrix = ComputeShadowMatrix( viewport, pDirectionalLightComponent->GetWorldTransform(), 50.0f/*TODO: configure*/ );
        }

        renderData.m_lightData.m_SunColorRoughnessOneLevel.SetW0();
        if ( !pWorldSystem->m_registeredGlobalEnvironmentMaps.empty() )
        {
            GlobalEnvironmentMapComponent* pGlobalEnvironmentMapComponent = pWorldSystem->m_registeredGlobalEnvironmentMaps[0];
            if ( pGlobalEnvironmentMapComponent->HasSkyboxRadianceTexture() && pGlobalEnvironmentMapComponent->HasSkyboxTexture() )
            {
                lightingFlags |= LIGHTING_ENABLE_SKYLIGHT;
                renderData.m_pSkyboxRadianceTexture = pGlobalEnvironmentMapComponent->GetSkyboxRadianceTexture();
                renderData.m_pSkyboxTexture = pGlobalEnvironmentMapComponent->GetSkyboxTexture();
                renderData.m_lightData.m_SunColorRoughnessOneLevel.SetW( Math::Max( Math::Floor( Math::Log2f( (float) renderData.m_pSkyboxRadianceTexture->GetDimensions().m_x ) ) - 1.0f, 0.0f ) );
                renderData.m_lightData.m_SunDirIndirectIntensity.SetW( pGlobalEnvironmentMapComponent->GetSkyboxIntensity() );
                renderData.m_lightData.m_manualExposure = pGlobalEnvironmentMapComponent->GetExposure();
            }
        }

        int32_t const numPointLights = Math::Min( pWorldSystem->m_registeredPointLightComponents.size(), (int32_t) s_maxPunctualLights );
        uint32_t lightIndex = 0;
        for ( int32_t i = 0; i < numPointLights; ++i )
        {
            EE_ASSERT( lightIndex < s_maxPunctualLights );
            PointLightComponent* pPointLightComponent = pWorldSystem->m_registeredPointLightComponents[i];
            renderData.m_lightData.m_punctualLights[lightIndex].m_positionInvRadiusSqr = pPointLightComponent->GetLightPosition();
            renderData.m_lightData.m_punctualLights[lightIndex].m_positionInvRadiusSqr.SetW( Math::Sqr( 1.0f / pPointLightComponent->GetLightRadius() ) );
            renderData.m_lightData.m_punctualLights[lightIndex].m_dir = Vector::Zero;
            renderData.m_lightData.m_punctualLights[lightIndex].m_color = Vector( pPointLightComponent->GetLightColor() ) * pPointLightComponent->GetLightIntensity();
            renderData.m_lightData.m_punctualLights[lightIndex].m_spotAngles = Vector( -1.0f, 1.0f, 0.0f );
            ++lightIndex;
        }

        int32_t const numSpotLights = Math::Min( pWorldSystem->m_registeredSpotLightComponents.size(), (int32_t) s_maxPunctualLights - numPointLights );
        for ( int32_t i = 0; i < numSpotLights; ++i )
        {
            EE_ASSERT( lightIndex < s_maxPunctualLights );
            SpotLightComponent* pSpotLightComponent = pWorldSystem->m_registeredSpotLightComponents[i];
            renderData.m_lightData.m_punctualLights[lightIndex].m_positionInvRadiusSqr = pSpotLightComponent->GetLightPosition();
            renderData.m_lightData.m_punctualLights[lightIndex].m_positionInvRadiusSqr.SetW( Math::Sqr( 1.0f / pSpotLightComponent->GetLightRadius() ) );
            renderData.m_lightData.m_punctualLights[lightIndex].m_dir = -pSpotLightComponent->GetLightDirection();
            renderData.m_lightData.m_punctualLights[lightIndex].m_color = Vector( pSpotLightComponent->GetLightColor() ) * pSpotLightComponent->GetLightIntensity();
            Radians innerAngle = pSpotLightComponent->GetLightInnerUmbraAngle().ToRadians();
            Radians outerAngle = pSpotLightComponent->GetLightOuterUmbraAngle().ToRadians();
            innerAngle.Clamp( 0, Math::PiDivTwo );
            outerAngle.Clamp( 0, Math::PiDivTwo );

            float cosInner = Math::Cos( (float) innerAngle );
            float cosOuter = Math::Cos( (float) outerAngle );
            renderData.m_lightData.m_punctualLights[lightIndex].m_spotAngles = Vector( cosOuter, 1.0f / Math::Max( cosInner - cosOuter, 0.001f ), 0.0f );
            ++lightIndex;
        }

        renderData.m_lightData.m_numPunctualLights = lightIndex;

        //-------------------------------------------------------------------------

        renderData.m_lightData.m_lightingFlags = lightingFlags;

        #if EE_DEVELOPMENT_TOOLS
        renderData.m_lightData.m_lightingFlags = renderData.m_lightData.m_lightingFlags | ( (int32_t) pWorldSystem->GetVisualizationMode() << (int32_t) RendererWorldSystem::VisualizationMode::BitShift );
        #endif

        //-------------------------------------------------------------------------

        //auto const& immediateContext = m_pRenderDevice->GetImmediateContext();

        //RenderSunShadows( viewport, pDirectionalLightComponent, renderData );
        //{
        //    //immediateContext.SetRenderTarget( renderTarget );
        //    RenderStaticMeshes( viewport, renderTarget, renderData );
        //    RenderSkeletalMeshes( viewport, renderTarget, renderData );
        //}
        //RenderSkybox( viewport, renderData );
    }

    void WorldRenderer::RenderWorld_Test( RG::RenderGraph& renderGraph, Seconds const deltaTime, Viewport const& viewport, RenderTarget const& renderTarget, EntityWorld* pWorld )
    {
        EE_ASSERT( IsInitialized() && Threading::IsMainThread() );
        EE_PROFILE_FUNCTION_RENDER();

        if ( !viewport.IsValid() )
        {
            return;
        }

        //-------------------------------------------------------------------------

        auto pWorldSystem = pWorld->GetWorldSystem<RendererWorldSystem>();
        EE_ASSERT( pWorldSystem != nullptr );

        //-------------------------------------------------------------------------

        // clear render data
        if ( !m_pRenderData )
        {
            m_pRenderData = EE::New<RenderData>(
                ObjectTransforms(),
                LightData(),
                nullptr,
                nullptr,
                pWorldSystem->m_visibleStaticMeshComponents,
                pWorldSystem->m_visibleSkeletalMeshComponents
            );
        }
        else
        {
            m_pRenderData->m_transforms = ObjectTransforms();
            m_pRenderData->m_lightData = LightData();
            m_pRenderData->m_pSkyboxRadianceTexture = nullptr;
            m_pRenderData->m_pSkyboxTexture = nullptr;
            m_pRenderData->m_staticMeshComponents = pWorldSystem->m_visibleStaticMeshComponents;
            m_pRenderData->m_skeletalMeshComponents = pWorldSystem->m_visibleSkeletalMeshComponents;
        }
        
        m_pRenderData->m_transforms.m_viewprojTransform = viewport.GetViewVolume().GetViewProjectionMatrix();

        //-------------------------------------------------------------------------

        uint32_t lightingFlags = 0;

        DirectionalLightComponent* pDirectionalLightComponent = nullptr;
        if ( !pWorldSystem->m_registeredDirectionLightComponents.empty() )
        {
            pDirectionalLightComponent = pWorldSystem->m_registeredDirectionLightComponents[0];
            lightingFlags |= LIGHTING_ENABLE_SUN;
            lightingFlags |= pDirectionalLightComponent->GetShadowed() ? LIGHTING_ENABLE_SUN_SHADOW : 0;
            m_pRenderData->m_lightData.m_SunDirIndirectIntensity = -pDirectionalLightComponent->GetLightDirection();
            Float4 colorIntensity = pDirectionalLightComponent->GetLightColor();
            m_pRenderData->m_lightData.m_SunColorRoughnessOneLevel = colorIntensity * pDirectionalLightComponent->GetLightIntensity();
            // TODO: conditional
            m_pRenderData->m_lightData.m_sunShadowMapMatrix = ComputeShadowMatrix( viewport, pDirectionalLightComponent->GetWorldTransform(), 50.0f/*TODO: configure*/ );
        }

        m_pRenderData->m_lightData.m_SunColorRoughnessOneLevel.SetW0();
        if ( !pWorldSystem->m_registeredGlobalEnvironmentMaps.empty() )
        {
            GlobalEnvironmentMapComponent* pGlobalEnvironmentMapComponent = pWorldSystem->m_registeredGlobalEnvironmentMaps[0];
            if ( pGlobalEnvironmentMapComponent->HasSkyboxRadianceTexture() && pGlobalEnvironmentMapComponent->HasSkyboxTexture() )
            {
                lightingFlags |= LIGHTING_ENABLE_SKYLIGHT;
                m_pRenderData->m_pSkyboxRadianceTexture = pGlobalEnvironmentMapComponent->GetSkyboxRadianceTexture();
                m_pRenderData->m_pSkyboxTexture = pGlobalEnvironmentMapComponent->GetSkyboxTexture();
                m_pRenderData->m_lightData.m_SunColorRoughnessOneLevel.SetW( Math::Max( Math::Floor( Math::Log2f( (float) m_pRenderData->m_pSkyboxRadianceTexture->GetDimensions().m_x ) ) - 1.0f, 0.0f ) );
                m_pRenderData->m_lightData.m_SunDirIndirectIntensity.SetW( pGlobalEnvironmentMapComponent->GetSkyboxIntensity() );
                m_pRenderData->m_lightData.m_manualExposure = pGlobalEnvironmentMapComponent->GetExposure();
            }
        }

        int32_t const numPointLights = Math::Min( pWorldSystem->m_registeredPointLightComponents.size(), (int32_t) s_maxPunctualLights );
        uint32_t lightIndex = 0;
        for ( int32_t i = 0; i < numPointLights; ++i )
        {
            EE_ASSERT( lightIndex < s_maxPunctualLights );
            PointLightComponent* pPointLightComponent = pWorldSystem->m_registeredPointLightComponents[i];
            m_pRenderData->m_lightData.m_punctualLights[lightIndex].m_positionInvRadiusSqr = pPointLightComponent->GetLightPosition();
            m_pRenderData->m_lightData.m_punctualLights[lightIndex].m_positionInvRadiusSqr.SetW( Math::Sqr( 1.0f / pPointLightComponent->GetLightRadius() ) );
            m_pRenderData->m_lightData.m_punctualLights[lightIndex].m_dir = Vector::Zero;
            m_pRenderData->m_lightData.m_punctualLights[lightIndex].m_color = Vector( pPointLightComponent->GetLightColor() ) * pPointLightComponent->GetLightIntensity();
            m_pRenderData->m_lightData.m_punctualLights[lightIndex].m_spotAngles = Vector( -1.0f, 1.0f, 0.0f );
            ++lightIndex;
        }

        int32_t const numSpotLights = Math::Min( pWorldSystem->m_registeredSpotLightComponents.size(), (int32_t) s_maxPunctualLights - numPointLights );
        for ( int32_t i = 0; i < numSpotLights; ++i )
        {
            EE_ASSERT( lightIndex < s_maxPunctualLights );
            SpotLightComponent* pSpotLightComponent = pWorldSystem->m_registeredSpotLightComponents[i];
            m_pRenderData->m_lightData.m_punctualLights[lightIndex].m_positionInvRadiusSqr = pSpotLightComponent->GetLightPosition();
            m_pRenderData->m_lightData.m_punctualLights[lightIndex].m_positionInvRadiusSqr.SetW( Math::Sqr( 1.0f / pSpotLightComponent->GetLightRadius() ) );
            m_pRenderData->m_lightData.m_punctualLights[lightIndex].m_dir = -pSpotLightComponent->GetLightDirection();
            m_pRenderData->m_lightData.m_punctualLights[lightIndex].m_color = Vector( pSpotLightComponent->GetLightColor() ) * pSpotLightComponent->GetLightIntensity();
            Radians innerAngle = pSpotLightComponent->GetLightInnerUmbraAngle().ToRadians();
            Radians outerAngle = pSpotLightComponent->GetLightOuterUmbraAngle().ToRadians();
            innerAngle.Clamp( 0, Math::PiDivTwo );
            outerAngle.Clamp( 0, Math::PiDivTwo );

            float cosInner = Math::Cos( (float) innerAngle );
            float cosOuter = Math::Cos( (float) outerAngle );
            m_pRenderData->m_lightData.m_punctualLights[lightIndex].m_spotAngles = Vector( cosOuter, 1.0f / Math::Max( cosInner - cosOuter, 0.001f ), 0.0f );
            ++lightIndex;
        }

        m_pRenderData->m_lightData.m_numPunctualLights = lightIndex;

        //-------------------------------------------------------------------------

        m_pRenderData->m_lightData.m_lightingFlags = lightingFlags;

        #if EE_DEVELOPMENT_TOOLS
        m_pRenderData->m_lightData.m_lightingFlags = m_pRenderData->m_lightData.m_lightingFlags | ( (int32_t) pWorldSystem->GetVisualizationMode() << (int32_t) RendererWorldSystem::VisualizationMode::BitShift );
        #endif

        //-------------------------------------------------------------------------

        if ( !m_bBrdfLutReady )
        {
            ComputeBrdfLut( renderGraph );
            m_bBrdfLutReady = true;
        }

        RenderSunShadows( renderGraph, viewport, pDirectionalLightComponent );
        {
            //immediateContext.SetRenderTarget( renderTarget );
            RenderStaticMeshes( renderGraph, viewport, renderTarget );
            //RenderSkeletalMeshes( viewport, renderTarget );
        }
        RenderSkybox( renderGraph, viewport, renderTarget );
    }
}