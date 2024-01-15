#include "RenderingSystem.h"
#include "Engine/Render/Renderers/WorldRenderer.h"
#include "Engine/Render/Renderers/DebugRenderer.h"
#include "Engine/Render/Renderers/ImguiRenderer.h"
#include "Engine/Entity/EntityWorldManager.h"
#include "Base/Render/RenderDevice.h"
#include "Engine/UpdateContext.h"
#include "Base/Profiling.h"
#include "Base/Math/ViewVolume.h"
#include "Base/Render/RenderPipelineRegistry.h"
#include "Engine/Entity/EntityWorld.h"
#include <eastl/sort.h>

//-------------------------------------------------------------------------

namespace EE::Render
{
    void RenderingSystem::CreateCustomRenderTargetForViewport( Viewport const* pViewport, bool requiresPickingBuffer )
    {
        EE_ASSERT( pViewport != nullptr && pViewport->IsValid() );
        EE_ASSERT( FindRenderTargetForViewport( pViewport ) == nullptr );

        m_pRenderDevice->LockDevice();
        {
            auto& vrt = m_viewportRenderTargets.emplace_back( pViewport->GetID(), EE::New<RenderTarget>() );
            m_pRenderDevice->CreateRenderTarget( *vrt.m_pRenderTarget, pViewport->GetDimensions(), requiresPickingBuffer );
            EE_ASSERT( vrt.m_pRenderTarget->IsValid() );
        }
        m_pRenderDevice->UnlockDevice();
    }

    void RenderingSystem::DestroyCustomRenderTargetForViewport( Viewport const* pViewport )
    {
        EE_ASSERT( pViewport != nullptr && pViewport->GetID().IsValid() );

        auto pViewportRenderTarget = FindRenderTargetForViewport( pViewport );
        EE_ASSERT( pViewportRenderTarget != nullptr );
        EE_ASSERT( pViewportRenderTarget->m_pRenderTarget != nullptr && pViewportRenderTarget->m_pRenderTarget->IsValid() );

        m_pRenderDevice->LockDevice();
        {
            m_pRenderDevice->DestroyRenderTarget( *pViewportRenderTarget->m_pRenderTarget );
            EE::Delete( pViewportRenderTarget->m_pRenderTarget );
        }
        m_pRenderDevice->UnlockDevice();

        m_viewportRenderTargets.erase( pViewportRenderTarget );
    }

    RenderTarget const& RenderingSystem::GetRenderTargetForViewport( Viewport const* pViewport ) const
    {
        EE_ASSERT( pViewport != nullptr && pViewport->GetID().IsValid() );

        auto* pViewportRenderTarget = FindRenderTargetForViewport( pViewport );
        EE_ASSERT( pViewportRenderTarget != nullptr );
        EE_ASSERT( pViewportRenderTarget->m_pRenderTarget != nullptr && pViewportRenderTarget->m_pRenderTarget->IsValid() );

        return *pViewportRenderTarget->m_pRenderTarget;
    }

    Render::PickingID RenderingSystem::GetViewportPickingID( Viewport const* pViewport, Int2 const& pixelCoords ) const
    {
        EE_ASSERT( pViewport != nullptr && pViewport->GetID().IsValid() );

        auto pViewportRenderTarget = FindRenderTargetForViewport( pViewport );
        EE_ASSERT( pViewportRenderTarget != nullptr );
        EE_ASSERT( pViewportRenderTarget->m_pRenderTarget != nullptr && pViewportRenderTarget->m_pRenderTarget->IsValid() );
        if ( pViewportRenderTarget->m_pRenderTarget->HasPickingRT() )
        {
            return m_pRenderDevice->ReadBackPickingID( *pViewportRenderTarget->m_pRenderTarget, pixelCoords );
        }
        else
        {
            return PickingID();
        }
    }

    RenderingSystem::ViewportRenderTarget* RenderingSystem::FindRenderTargetForViewport( Viewport const* pViewport )
    {
        auto SearchPredicate = [] ( ViewportRenderTarget const& viewportRenderTarget, UUID const& ID ) { return viewportRenderTarget.m_viewportID == ID; };
        auto viewportRenderTargetIter = eastl::find( m_viewportRenderTargets.begin(), m_viewportRenderTargets.end(), pViewport->GetID(), SearchPredicate );
        if ( viewportRenderTargetIter != m_viewportRenderTargets.end() )
        {
            return viewportRenderTargetIter;
        }

        return nullptr;
    }

    //-------------------------------------------------------------------------

    void RenderingSystem::Initialize( RenderDevice* pRenderDevice, PipelineRegistry* pRenderPipelineRegistry, Float2 primaryWindowDimensions, RendererRegistry* pRegistry, EntityWorldManager* pWorldManager )
    {
        EE_ASSERT( m_pRenderDevice == nullptr );
        EE_ASSERT( m_pRenderPipelineRegistry == nullptr );
        EE_ASSERT( pRenderDevice != nullptr && pRegistry != nullptr && pRenderPipelineRegistry != nullptr );
        EE_ASSERT( pWorldManager != nullptr );

        // Initialize render graph system
        //-------------------------------------------------------------------------

        m_pRenderPipelineRegistry = pRenderPipelineRegistry;
        m_renderGraph.AttachToPipelineRegistry( m_pRenderPipelineRegistry );

        // Set initial render device size
        //-------------------------------------------------------------------------

        m_pRenderDevice = pRenderDevice;
        m_pWorldManager = pWorldManager;

        m_pRenderDevice->LockDevice();
        m_pRenderDevice->ResizePrimaryWindowRenderTarget( primaryWindowDimensions );
        m_pRenderDevice->UnlockDevice();

        // Initialize viewports
        //-------------------------------------------------------------------------

        Math::ViewVolume const orthographicVolume( primaryWindowDimensions, FloatRange( 0.1f, 100.0f ) );

        for ( auto pWorld : m_pWorldManager->GetWorlds() )
        {
            Render::Viewport* pViewport = pWorld->GetViewport();
            *pViewport = Viewport( Int2::Zero, primaryWindowDimensions, orthographicVolume );
        }

        #if EE_DEVELOPMENT_TOOLS
        m_toolsViewport = Viewport( Int2::Zero, primaryWindowDimensions, orthographicVolume );
        #endif

        // Get renderers
        //-------------------------------------------------------------------------

        for ( auto pRenderer : pRegistry->GetRegisteredRenderers() )
        {
            if ( pRenderer->GetRendererID() == WorldRenderer::RendererID )
            {
                EE_ASSERT( m_pWorldRenderer == nullptr );
                m_pWorldRenderer = reinterpret_cast<WorldRenderer*>( pRenderer );
                continue;
            }

            //-------------------------------------------------------------------------

            #if EE_DEVELOPMENT_TOOLS
            if ( pRenderer->GetRendererID() == ImguiRenderer::RendererID )
            {
                EE_ASSERT( m_pImguiRenderer == nullptr );
                m_pImguiRenderer = reinterpret_cast<ImguiRenderer*>( pRenderer );
                continue;
            }

            if ( pRenderer->GetRendererID() == DebugRenderer::RendererID )
            {
                EE_ASSERT( m_pDebugRenderer == nullptr );
                m_pDebugRenderer = reinterpret_cast<DebugRenderer*>( pRenderer );
                continue;
            }
            #endif

            //-------------------------------------------------------------------------

            m_customRenderers.push_back( pRenderer );
        }

        EE_ASSERT( m_pWorldRenderer != nullptr );

        // Sort custom renderers
        //-------------------------------------------------------------------------

        auto comparator = [] ( IRenderer* const& pRendererA, IRenderer* const& pRendererB )
        {
            int32_t const A = pRendererA->GetPriority();
            int32_t const B = pRendererB->GetPriority();
            return A < B;
        };

        eastl::sort( m_customRenderers.begin(), m_customRenderers.end(), comparator );
    }

    void RenderingSystem::Shutdown()
    {
        m_pRenderDevice->GetRHIDevice()->WaitUntilIdle();

        // Destroy any viewport render targets created
        //-------------------------------------------------------------------------

        m_renderGraph.DestroyAllResources( m_pRenderDevice );

        m_pRenderDevice->LockDevice();
        for ( auto& viewportRenderTarget : m_viewportRenderTargets )
        {
            m_pRenderDevice->DestroyRenderTarget( *viewportRenderTarget.m_pRenderTarget );
            EE::Delete( viewportRenderTarget.m_pRenderTarget );
        }
        m_pRenderDevice->UnlockDevice();

        m_viewportRenderTargets.clear();

        // Clear ptrs
        //-------------------------------------------------------------------------

        m_pRenderPipelineRegistry = nullptr;
        m_pWorldRenderer = nullptr;

        #if EE_DEVELOPMENT_TOOLS
        m_pImguiRenderer = nullptr;
        #endif

        m_pWorldManager = nullptr;
        m_pRenderDevice = nullptr;
    }

    void RenderingSystem::ResizePrimaryRenderTarget( Int2 newMainWindowDimensions )
    {
        Float2 const newWindowDimensions = Float2( newMainWindowDimensions );
        Float2 const oldWindowDimensions = Float2( m_pRenderDevice->GetPrimaryWindowDimensions() );

        //-------------------------------------------------------------------------

        m_pRenderDevice->LockDevice();
        m_pRenderDevice->ResizePrimaryWindowRenderTarget( newMainWindowDimensions );
        m_pRenderDevice->UnlockDevice();

        //-------------------------------------------------------------------------

        // TODO: add info whether viewports are in absolute size or proportional, right now it's all proportional
        for ( auto pWorld : m_pWorldManager->GetWorlds() )
        {
            Render::Viewport* pViewport = pWorld->GetViewport();
            Float2 const viewportPosition = pViewport->GetTopLeftPosition();
            Float2 const viewportSize = pViewport->GetDimensions();
            Float2 const newViewportPosition = ( viewportPosition / oldWindowDimensions ) * newWindowDimensions;
            Float2 const newViewportSize = ( viewportSize / oldWindowDimensions ) * newWindowDimensions;
            pViewport->Resize( newViewportPosition, newViewportSize );
        }
    }

    void RenderingSystem::Update( UpdateContext const& ctx )
    {
        EE_ASSERT( m_pRenderDevice != nullptr );
        EE_ASSERT( ctx.GetUpdateStage() == UpdateStage::FrameEnd );
        EE_PROFILE_SCOPE_RENDER( "Rendering Post-Physics" );

        m_pRenderDevice->LockDevice();

        // Render into active viewports
        //-------------------------------------------------------------------------

        RenderTarget const* pPrimaryRT = m_pRenderDevice->GetPrimaryWindowRenderTarget();

        //for ( auto pWorld : m_pWorldManager->GetWorlds() )
        //{
        //    if ( pWorld->IsSuspended() )
        //    {
        //        continue;
        //    }

        //    RenderTarget const* pViewportRT = pPrimaryRT;

        //    Render::Viewport* pViewport = pWorld->GetViewport();
        //    ViewportRenderTarget const* pVRT = FindRenderTargetForViewport( pViewport );
        //    // Note: Viewport of a world must have a valid render target pair with it.
        //    //       Otherwise try render to primary render target.
        //    if ( pVRT != nullptr )
        //    {
        //        pViewportRT = pVRT->m_pRenderTarget;
        //        m_pWorldRenderer->RenderWorld_Test( m_renderGraph, ctx.GetDeltaTime(), *pViewport, *pViewportRT, pWorld );
        //    }

        //    //for ( auto const& pCustomRenderer : m_customRenderers )
        //    //{
        //    //    pCustomRenderer->RenderWorld( ctx.GetDeltaTime(), *pViewport, *pViewportRT, pWorld );
        //    //    pCustomRenderer->RenderViewport( ctx.GetDeltaTime(), *pViewport, *pViewportRT );
        //    //}

        //    #if EE_DEVELOPMENT_TOOLS
        //    //m_pDebugRenderer->RenderWorld( ctx.GetDeltaTime(), *pViewport, *pViewportRT, pWorld );
        //    //m_pDebugRenderer->RenderViewport( ctx.GetDeltaTime(), *pViewport, *pViewportRT );
        //    #endif
        //}

        // Draw development UI
        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        if ( m_pImguiRenderer != nullptr )
        {
            m_pImguiRenderer->RenderViewport_Test( m_renderGraph, ctx.GetDeltaTime(), m_toolsViewport, *pPrimaryRT );
        }
        #endif

        m_pRenderDevice->UnlockDevice();

        // render graph rendering and presenting
        //-------------------------------------------------------------------------

        auto* pRhiDevice = m_pRenderDevice->GetRHIDevice();
        pRhiDevice->BeginFrame();

        bool bCompileResult = m_renderGraph.Compile( m_pRenderDevice );
        // TODO: when pipeline registry failed to update pipelines, use old pipelines
        m_pRenderPipelineRegistry->UpdatePipelines( pRhiDevice );

        if ( bCompileResult )
        {
            m_renderGraph.Execute( pRhiDevice );
            m_renderGraph.Present( pRhiDevice, *m_pRenderDevice->GetPrimaryWindowRenderTarget() );

            m_pRenderDevice->PresentFrame();
        }

        pRhiDevice->EndFrame();

        m_renderGraph.Retire();

        // Present frame
        //-------------------------------------------------------------------------

        //m_pRenderDevice->PresentFrame();

    }

    void RenderingSystem::ResizeWorldRenderTargets()
    {
        RenderTarget* pPrimaryRT = m_pRenderDevice->GetPrimaryWindowRenderTarget();

        for ( auto pWorld : m_pWorldManager->GetWorlds() )
        {
            if ( pWorld->IsSuspended() )
            {
                continue;
            }

            Render::Viewport* pViewport = pWorld->GetViewport();

            // Set and clear render target
            //-------------------------------------------------------------------------

            RenderTarget* pViewportRT = pPrimaryRT;
            ViewportRenderTarget const* pVRT = FindRenderTargetForViewport( pViewport );
            if ( pVRT != nullptr )
            {
                pViewportRT = pVRT->m_pRenderTarget;

                // Resize render target if needed
                if ( Int2( pViewport->GetDimensions() ) != pViewportRT->GetDimensions() )
                {
                    m_pRenderDevice->ResizeRenderTarget( *pViewportRT, pViewport->GetDimensions() );
                }

                // TODO: Clear render target and depth stencil textures
            }
        }
    }
}