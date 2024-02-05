#include "RenderGraph.h"
#include "RenderGraphResolver.h"
#include "Base/Types/Arrays.h"
#include "Base/Logging/Log.h"
#include "Base/Threading/Threading.h"
#include "Base/Render/RenderTarget.h"
#include "Base/Render/RenderDevice.h"
#include "Base/RHI/RHIDevice.h"
#include "Base/RHI/RHISwapchain.h"
#include "Base/RHI/RHICommandQueue.h"
#include "Base/RHI/Resource/RHIBuffer.h"
#include "Base/RHI/Resource/RHITexture.h"

namespace EE
{
	namespace RG
	{
		RenderGraph::RenderGraph()
			: RenderGraph( "Default RG" )
		{}

		RenderGraph::RenderGraph( String const& graphName )
            : m_name( graphName ), m_currentDeviceFrameIndex( 0 )   
        {}

        // Build Stage
		//-------------------------------------------------------------------------
		
        RGRenderCommandContext& RenderGraph::ResetCommandContext( RHI::RHIDevice* pRhiDevice )
        {
            EE_ASSERT( pRhiDevice );

            m_currentDeviceFrameIndex = pRhiDevice->GetDeviceFrameIndex();

            auto& commandContext = m_renderCommandContexts[m_currentDeviceFrameIndex];
            commandContext.SetCommandContext( this, pRhiDevice, pRhiDevice->AllocateCommandBuffer() );

            if ( !commandContext.m_pCommandBuffer )
            {
                EE_LOG_ERROR( "RenderGraph", "BeginFrame", "Failed to begin render graph frame!" );
                EE_ASSERT( false );
            }

            pRhiDevice->BeginCommandBuffer( commandContext.m_pCommandBuffer );
            return commandContext;
        }

        void RenderGraph::FlushCommandContext( RHI::RHIDevice* pRhiDevice )
        {
            auto& commandContext = m_renderCommandContexts[m_currentDeviceFrameIndex];
            EE_ASSERT( commandContext.m_pCommandBuffer );

            pRhiDevice->EndCommandBuffer( commandContext.m_pCommandBuffer );

            commandContext.SubmitAndReset( pRhiDevice );
        }

        RGResourceHandle<RGResourceTagBuffer> RenderGraph::ImportResource( RHI::RHIBuffer* pBuffer, RHI::RenderResourceBarrierState access )
        {
            EE_ASSERT( Threading::IsMainThread() );

            _Impl::RGBufferDesc rgDesc = {};
            EE::RG::BufferDesc bufferDesc;
            bufferDesc.m_desc = pBuffer->GetDesc();
            rgDesc.m_desc = bufferDesc;

            RGImportedResource importResource;
            importResource.m_pImportedResource = pBuffer;
            importResource.m_currentAccess = access;

            _Impl::RGResourceID const id = m_resourceRegistry.ImportResource<_Impl::RGBufferDesc>( std::move( rgDesc ), std::move( importResource ) );
            RGResourceHandle<RGResourceTagBuffer> handle;
            handle.m_slotID = id;
            handle.m_desc = bufferDesc;
            return handle;
        }

        RGResourceHandle<RGResourceTagTexture> RenderGraph::ImportResource( RHI::RHITexture* pTexture, RHI::RenderResourceBarrierState access )
        {
            EE_ASSERT( Threading::IsMainThread() );

            _Impl::RGTextureDesc rgDesc = {};
            EE::RG::TextureDesc textureDesc;
            textureDesc.m_desc = pTexture->GetDesc();
            rgDesc.m_desc = textureDesc;

            RGImportedResource importResource;
            importResource.m_pImportedResource = pTexture;
            importResource.m_currentAccess = access;
            EE_ASSERT( importResource.m_pImportedResource );

            _Impl::RGResourceID const id = m_resourceRegistry.ImportResource<_Impl::RGTextureDesc>( std::move( rgDesc ), std::move( importResource ) );
            RGResourceHandle<RGResourceTagTexture> handle;
            handle.m_slotID = id;
            handle.m_desc = textureDesc;
            return handle;
        }

        RGResourceHandle<RGResourceTagBuffer> RenderGraph::ImportResource( RHI::RHIBuffer const* pBuffer, RHI::RenderResourceBarrierState access )
        {
            EE_ASSERT( Threading::IsMainThread() );

            _Impl::RGBufferDesc rgDesc = {};
            EE::RG::BufferDesc bufferDesc;
            bufferDesc.m_desc = pBuffer->GetDesc();
            rgDesc.m_desc = bufferDesc;

            // TODO: break of the const consistency?
            RGImportedResource importResource;
            importResource.m_pImportedResource = const_cast<RHI::RHIBuffer*>( pBuffer );
            importResource.m_currentAccess = access;

            _Impl::RGResourceID const id = m_resourceRegistry.ImportResource<_Impl::RGBufferDesc>( std::move( rgDesc ), std::move( importResource ) );
            RGResourceHandle<RGResourceTagBuffer> handle;
            handle.m_slotID = id;
            handle.m_desc = bufferDesc;
            return handle;
        }
        
        RGResourceHandle<RGResourceTagTexture> RenderGraph::ImportResource( RHI::RHITexture const* pTexture, RHI::RenderResourceBarrierState access )
        {
            EE_ASSERT( Threading::IsMainThread() );

            _Impl::RGTextureDesc rgDesc = {};
            EE::RG::TextureDesc textureDesc;
            textureDesc.m_desc = pTexture->GetDesc();
            rgDesc.m_desc = textureDesc;

            // TODO: break of the const consistency?
            RGImportedResource importResource;
            importResource.m_pImportedResource = const_cast<RHI::RHITexture*>( pTexture );
            importResource.m_currentAccess = access;
            EE_ASSERT( importResource.m_pImportedResource );

            _Impl::RGResourceID const id = m_resourceRegistry.ImportResource<_Impl::RGTextureDesc>( std::move( rgDesc ), std::move( importResource ) );
            RGResourceHandle<RGResourceTagTexture> handle;
            handle.m_slotID = id;
            handle.m_desc = textureDesc;
            return handle;
        }

        RGResourceHandle<RGResourceTagTexture> RenderGraph::ImportResource( Render::RenderTarget const& renderTarget, RHI::RenderResourceBarrierState access )
        {
            EE_ASSERT( Threading::IsMainThread() );
            // Note: renderTarget can be invalid, since it can be lazy swapchain present render target
            EE_ASSERT( renderTarget.IsInitialized() );

            if ( renderTarget.IsSwapchainRenderTarget() )
            {
                return ImportSwapchainTextureResource( renderTarget );
            }
            else
            {
                // Because it is NOT a swapchain render target, so it must be valid to continue
                EE_ASSERT( renderTarget.IsValid() );
                return ImportResource( renderTarget.GetRHIRenderTarget(), access );
            }
        }

        RGNodeBuilder RenderGraph::AddNode( String const& nodeName )
		{
			EE_ASSERT( Threading::IsMainThread() );

            auto nextID = static_cast<uint32_t>( m_renderGraph.size() );
			auto& newNode = m_renderGraph.emplace_back( nodeName, nextID );

			return RGNodeBuilder( m_resourceRegistry, newNode );
		}

		#if EE_DEVELOPMENT_TOOLS
		void RenderGraph::LogGraphNodes() const
		{
			EE_ASSERT( Threading::IsMainThread() );

			size_t const count = m_renderGraph.size();

			EE_LOG_INFO( "Render Graph", "Graph", "Node Count: %u", count );

			for ( size_t i = 0; i < count; ++i )
			{
				EE_LOG_INFO( "Render Graph", "Graph", "\tNode (%s)", m_renderGraph[i].m_passName.c_str() );
			}
		}
        #endif

        // Compilation Stage
        //-------------------------------------------------------------------------

        bool RenderGraph::Compile( Render::RenderDevice* pDevice )
        {
            EE_ASSERT( Threading::IsMainThread() );

            if ( pDevice == nullptr )
            {
                EE_LOG_WARNING( "RenderGraph", "RenderGraph::Compile()", "RHI Device missing! Cannot compile render graph!" );
                return false;
            }

            // Graph dependency analysis
            //-------------------------------------------------------------------------

            RenderGraphResolver resolver( m_renderGraph, m_resourceRegistry );
            RGResolveResult resolvedResult = resolver.Resolve();

            // Create actual RHI Resources
            //-------------------------------------------------------------------------

            bool result = m_resourceRegistry.Compile( pDevice, resolvedResult );

            if ( !result )
            {
                return false;
            }

            // Compile and Analyze graph nodes to populate an execution sequence
            //-------------------------------------------------------------------------

            bool allNodesAreReady = true;
            
            for ( auto& node : m_renderGraph )
            {
                if ( !node.HadRegisteredPipeline() )
                {
                    continue;
                }

                if ( node.m_pipelineHandle.IsValid() && !node.IsReadyToExecute( &m_resourceRegistry ) )
                {
                    allNodesAreReady = false;
                    break;
                }
            }

            if ( allNodesAreReady )
            {
                // Compile render graph into executable render graph
                //-------------------------------------------------------------------------

                TVector<RGExecutableNode> executeSequences;
                executeSequences.reserve( m_renderGraph.size() );
                for ( auto& node : m_renderGraph )
                {
                    executeSequences.emplace_back( eastl::move( node ).IntoExecutableNode( &m_resourceRegistry ) );
                }

                // Split execute nodes and present nodes
                //-------------------------------------------------------------------------

                int32_t firstPresentNodeIndex = FindPresentNodeIndex( executeSequences );

                if ( firstPresentNodeIndex < 0 )
                {
                    //EE_LOG_WARNING( "RenderGraph", "RenderGraph::Compile()", "RenderGraph has no presentable node!" );
                    return false;
                }

                size_t const presentNodeCount = executeSequences.size() - firstPresentNodeIndex;
                m_presentNodesSequence.resize( presentNodeCount );
                eastl::move( executeSequences.rbegin(), executeSequences.rbegin() + presentNodeCount, m_presentNodesSequence.begin() );

                executeSequences.erase( executeSequences.rbegin(), executeSequences.rbegin() + presentNodeCount );
                m_executeNodesSequence = eastl::move( executeSequences );
            }
            else
            {
                return false;
            }
        
            return true;
        }

        // Execution Stage
        //-------------------------------------------------------------------------

        void RenderGraph::Execute( RHI::RHIDevice* pRhiDevice )
        {
            EE_ASSERT( Threading::IsMainThread() );
            EE_ASSERT( pRhiDevice );
            EE_ASSERT( m_resourceRegistry.GetCurrentResourceState() == RGResourceRegistry::ResourceState::Compiled );

            if ( !m_executeNodesSequence.empty() )
            {
                ResetCommandContext( pRhiDevice );

                // Transition all resources used in execution nodes ahead to reduce some pipeline bubbles.
                //-------------------------------------------------------------------------
            
                // TODO: Count for total transition resource ahead, to avoid frequently memory allocation.
                TVector<TPair<RGCompiledResource&, RHI::RenderResourceAccessState>> transitionResources;

                for ( RGExecutableNode& node : m_executeNodesSequence )
                {
                    for ( RGNodeResource& input : node.m_inputs )
                    {
                        transitionResources.emplace_back(
                            m_resourceRegistry.GetCompiledResource( input ),
                            // Note: force skipping synchronization.
                            RHI::RenderResourceAccessState{ input.m_passAccess.GetCurrentAccess(), true }
                            );

                        // Note: we already transition the barrier state once, skip synchronization next time encounter.
                        input.m_passAccess.SetSkipSyncIfContinuous( true );
                    }

                    for ( RGNodeResource& output : node.m_outputs )
                    {
                        transitionResources.emplace_back(
                            m_resourceRegistry.GetCompiledResource( output ),
                            // Note: force skipping synchronization.
                            RHI::RenderResourceAccessState{ output.m_passAccess.GetCurrentAccess(), true }
                        );

                        // Note: we already transition the barrier state once, skip synchronization next time encounter.
                        output.m_passAccess.SetSkipSyncIfContinuous( true );
                    }
                }

                for ( auto& transitionResource : transitionResources )
                {
                    TransitionResource( transitionResource.first, transitionResource.second );
                }

                // Execute render graph
                //-------------------------------------------------------------------------

                for ( RGExecutableNode& executionNode : m_executeNodesSequence )
                {
                    ExecuteNode( executionNode );
                }

                FlushCommandContext( pRhiDevice );
            }
        }

        void RenderGraph::Present( RHI::RHIDevice* pRhiDevice, Render::SwapchainRenderTarget& swapchainRt )
        {
            EE_ASSERT( Threading::IsMainThread() );
            EE_ASSERT( pRhiDevice );
            EE_ASSERT( m_resourceRegistry.GetCurrentResourceState() == RGResourceRegistry::ResourceState::Compiled );

            if ( !m_presentNodesSequence.empty() )
            {
                // Acquire next frame before presenting
                //-------------------------------------------------------------------------

                if ( swapchainRt.AcquireNextFrame() )
                {
                    // Execute render graph
                    //-------------------------------------------------------------------------

                    auto& commandContext = ResetCommandContext( pRhiDevice );
                    commandContext.AddWaitSyncPoint( swapchainRt.GetWaitSemaphore(), Render::PipelineStage::Pixel );
                    commandContext.AddSignalSyncPoint( swapchainRt.GetSignalSemaphore() );

                    for ( RGExecutableNode& presentNode : m_presentNodesSequence )
                    {
                        PresentNode( presentNode, swapchainRt.GetRHIRenderTarget() );
                    }

                    FlushCommandContext( pRhiDevice );
                }
            }
        }

        void RenderGraph::Retire()
        {
            // clear up previous frame resources
            m_renderGraph.clear();
            m_executeNodesSequence.clear();
            m_presentNodesSequence.clear();

            m_resourceRegistry.Retire();
        }

        // Cleanup Stage
        //-------------------------------------------------------------------------

        void RenderGraph::DestroyAllResources( Render::RenderDevice* pDevice )
        {
            m_resourceRegistry.Shutdown( pDevice );
        }

        //-------------------------------------------------------------------------

        RGResourceHandle<RGResourceTagTexture> RenderGraph::ImportSwapchainTextureResource( Render::RenderTarget const& swapchainRenderTarget )
        {
            EE_ASSERT( Threading::IsMainThread() );
            EE_ASSERT( swapchainRenderTarget.IsInitialized() && swapchainRenderTarget.IsSwapchainRenderTarget() );

            auto& rt = static_cast<Render::SwapchainRenderTarget const&>( swapchainRenderTarget );
            EE_ASSERT( rt.GetRHISwapchain() );

            // Note: register swapchain texture resource as regular imported render graph texture resource.
            _Impl::RGTextureDesc rgDesc = {};
            EE::RG::TextureDesc textureDesc;
            textureDesc.m_desc = rt.GetRHISwapchain()->GetPresentTextureDesc();
            rgDesc.m_desc = textureDesc;

            RGImportedResource importResource;
            importResource.m_pImportedResource = nullptr;
            importResource.m_currentAccess = RHI::RenderResourceBarrierState::Undefined;

            _Impl::RGResourceID const id = m_resourceRegistry.ImportResource<_Impl::RGTextureDesc>( rgDesc, importResource );
            RGResourceHandle<RGResourceTagTexture> handle;
            handle.m_slotID = id;
            handle.m_desc = textureDesc;
            return handle;
        }

        int32_t RenderGraph::FindPresentNodeIndex( TVector<RGExecutableNode> const& executionSequence ) const
        {
            EE_ASSERT( Threading::IsMainThread() );

            if ( executionSequence.empty() )
            {
                return -1;
            }

            int32_t result = -1;
            int32_t currentNode = static_cast<int32_t>( executionSequence.size() - 1 );
            // For now, this node.m_id is the index inside m_executionSequence.
            for ( auto beg = executionSequence.rbegin(); beg != executionSequence.rend(); ++beg )
            {
                auto& node = *beg;
                for ( auto const& output : node.m_outputs )
                {
                    auto& compiledResource = m_resourceRegistry.GetCompiledResource( output );
                    if ( compiledResource.IsImportedResource() && compiledResource.GetResourceType() == RGResourceType::Texture )
                    {
                        if ( compiledResource.IsSwapchainImportedResource() )
                        {
                            result = static_cast<int32_t>(currentNode);
                            break;
                        }
                    }
                }

                --currentNode;
            }

            return result;
        }

        void RenderGraph::TransitionResource( RGCompiledResource& compiledResource, RHI::RenderResourceAccessState const& access )
        {
            // TODO: Batched barrier transition.
            //       It is more efficient to transition barrier all at once than one by one.

            if ( access.GetSkipSyncIfContinuous() && access.GetCurrentAccess() == compiledResource.GetCurrentAccessState().GetCurrentAccess() )
            {
                return;
            }

            auto& commandContext = m_renderCommandContexts[m_currentDeviceFrameIndex];
            auto* pSubmitQueue = commandContext.m_pCommandQueue;

            switch ( compiledResource.GetResourceType() )
            {
                case RGResourceType::Buffer:
                {
                    RHI::RenderResourceBarrierState prevBarrier[1] = { compiledResource.GetCurrentAccessState().GetCurrentAccess() };
                    RHI::RenderResourceBarrierState nextBarrier[1] = { access.GetCurrentAccess() };

                    RHI::BufferBarrier barrier;
                    barrier.m_pRhiBuffer = compiledResource.GetResource<RGResourceTagBuffer>();
                    barrier.m_size = compiledResource.GetDesc<RGResourceTagBuffer>().m_desc.m_desireSize;
                    barrier.m_offset = 0;
                    barrier.m_previousAccessesCount = 1;
                    barrier.m_pPreviousAccesses = prevBarrier;
                    barrier.m_nextAccessesCount = 1;
                    barrier.m_pNextAccesses = nextBarrier;
                    barrier.m_srcQueueFamilyIndex = pSubmitQueue->GetDeviceIndex();
                    barrier.m_dstQueueFamilyIndex = pSubmitQueue->GetDeviceIndex();

                    commandContext.GetRHICommandBuffer()->PipelineBarrier(
                        nullptr,
                        1, &barrier,
                        0, nullptr
                    );

                    compiledResource.GetCurrentAccessState().TransiteTo( nextBarrier[0] );
                    break;
                }
                case RGResourceType::Texture:
                {
                    RHI::RenderResourceBarrierState prevBarrier[1] = { compiledResource.GetCurrentAccessState().GetCurrentAccess() };
                    RHI::RenderResourceBarrierState nextBarrier[1] = { access.GetCurrentAccess() };

                    RHI::TextureBarrier barrier;
                    barrier.m_pRhiTexture = compiledResource.GetResource<RGResourceTagTexture>();
                    barrier.m_previousAccessesCount = 1;
                    barrier.m_pPreviousAccesses = prevBarrier;
                    barrier.m_nextAccessesCount = 1;
                    barrier.m_pNextAccesses = nextBarrier;
                    barrier.m_previousLayout = RHI::TextureMemoryLayout::Optimal;
                    barrier.m_nextLayout = RHI::TextureMemoryLayout::Optimal;
                    barrier.m_srcQueueFamilyIndex = pSubmitQueue->GetDeviceIndex();
                    barrier.m_dstQueueFamilyIndex = pSubmitQueue->GetDeviceIndex();
                    // TODO: for now, always keep contents
                    barrier.m_discardContents = false;

                    auto& desc = compiledResource.GetDesc<RGResourceTagTexture>().m_desc;
                    barrier.m_subresourceRange = RHI::TextureSubresourceRange::AllSubresources( PixelFormatToAspectFlags( desc.m_format ) );

                    commandContext.GetRHICommandBuffer()->PipelineBarrier(
                        nullptr,
                        0, nullptr,
                        1, &barrier
                    );

                    compiledResource.GetCurrentAccessState().TransiteTo( nextBarrier[0] );
                    break;
                }
                default:
                EE_UNIMPLEMENTED_FUNCTION();
                break;
            }
        }

        void RenderGraph::TransitionResourceBatched( TSpan<TPair<RGCompiledResource&, RHI::RenderResourceAccessState>> transitionResources )
        {
            if ( transitionResources.empty() )
            {
                return;
            }

            TInlineVector<TPair<RGCompiledResource&, RHI::RenderResourceAccessState>, 32> filteredTransitionResources;
            filteredTransitionResources.reserve( transitionResources.size() );

            eastl::copy_if( transitionResources.begin(), transitionResources.end(), eastl::back_inserter(filteredTransitionResources), [] ( TPair<RGCompiledResource&, RHI::RenderResourceAccessState>& pair )
            { 
                auto& [transitionResource, access] = pair;
                return !( access.GetSkipSyncIfContinuous() &&
                          access.GetCurrentAccess() == transitionResource.GetCurrentAccessState().GetCurrentAccess() );
            });

            TInlineVector<RHI::BufferBarrier, 32> bufferBarriers;
            TInlineVector<RHI::TextureBarrier, 32> textureBarriers;

            TInlineVector<TPair<RHI::RenderResourceBarrierState, RHI::RenderResourceBarrierState>, 64> barrierTransitions;

            for ( auto& [transitionResource, access] : filteredTransitionResources )
            {
                barrierTransitions.emplace_back( transitionResource.GetCurrentAccessState().GetCurrentAccess(), access.GetCurrentAccess() );
            }

            auto& commandContext = m_renderCommandContexts[m_currentDeviceFrameIndex];
            auto* pSubmitQueue = commandContext.m_pCommandQueue;

            // Safety: After this time point, barrierTransitions should NOT be reallocated.
            //         This will cause m_pPreviousAccesses and m_pNextAccesses points to dangling memory.
            size_t currentTransition = 0;
            for ( auto& [transitionResource, access] : filteredTransitionResources )
            {
                switch ( transitionResource.GetResourceType() )
                {
                    case RGResourceType::Buffer:
                    {
                        RHI::BufferBarrier barrier;
                        barrier.m_pRhiBuffer = transitionResource.GetResource<RGResourceTagBuffer>();
                        barrier.m_size = transitionResource.GetDesc<RGResourceTagBuffer>().m_desc.m_desireSize;
                        barrier.m_offset = 0;
                        barrier.m_previousAccessesCount = 1;
                        barrier.m_pPreviousAccesses = &barrierTransitions[currentTransition].first;
                        barrier.m_nextAccessesCount = 1;
                        barrier.m_pNextAccesses = &barrierTransitions[currentTransition].second;
                        barrier.m_srcQueueFamilyIndex = pSubmitQueue->GetDeviceIndex();
                        barrier.m_dstQueueFamilyIndex = pSubmitQueue->GetDeviceIndex();

                        bufferBarriers.push_back( barrier );

                        transitionResource.GetCurrentAccessState().TransiteTo( barrierTransitions[currentTransition].second );
                        break;
                    }
                    case RGResourceType::Texture:
                    {
                        RHI::TextureBarrier barrier;
                        barrier.m_pRhiTexture = transitionResource.GetResource<RGResourceTagTexture>();
                        barrier.m_previousAccessesCount = 1;
                        barrier.m_pPreviousAccesses = &barrierTransitions[currentTransition].first;
                        barrier.m_nextAccessesCount = 1;
                        barrier.m_pNextAccesses = &barrierTransitions[currentTransition].second;
                        barrier.m_previousLayout = RHI::TextureMemoryLayout::Optimal;
                        barrier.m_nextLayout = RHI::TextureMemoryLayout::Optimal;
                        barrier.m_srcQueueFamilyIndex = pSubmitQueue->GetDeviceIndex();
                        barrier.m_dstQueueFamilyIndex = pSubmitQueue->GetDeviceIndex();
                        // TODO: for now, always keep contents
                        barrier.m_discardContents = false;

                        auto& desc = transitionResource.GetDesc<RGResourceTagTexture>().m_desc;
                        barrier.m_subresourceRange = RHI::TextureSubresourceRange::AllSubresources( PixelFormatToAspectFlags( desc.m_format ) );

                        textureBarriers.push_back( barrier );

                        transitionResource.GetCurrentAccessState().TransiteTo( barrierTransitions[currentTransition].second );
                        break;
                    }
                    default:
                    EE_UNIMPLEMENTED_FUNCTION();
                    break;
                }

                ++currentTransition;
            }


            if ( !bufferBarriers.empty() )
            {
                commandContext.GetRHICommandBuffer()->PipelineBarrier(
                    nullptr,
                    static_cast<uint32_t>(bufferBarriers.size()), bufferBarriers.data(),
                    0, nullptr
                );
            }
            if ( !textureBarriers.empty() )
            {
                commandContext.GetRHICommandBuffer()->PipelineBarrier(
                    nullptr,
                    0, nullptr,
                    static_cast<uint32_t>( textureBarriers.size() ), textureBarriers.data()
                );
            }
        }

        void RenderGraph::ExecuteNode( RGExecutableNode& node )
        {
            // Transite resource to target pipeline barrier states
            //-------------------------------------------------------------------------

            size_t totalResourceCount = node.m_inputs.size() + node.m_outputs.size();
            TVector<TPair<RGCompiledResource&, RHI::RenderResourceAccessState>> transitionResources;
            transitionResources.reserve( totalResourceCount );

            for ( RGNodeResource& input : node.m_inputs )
            {
                transitionResources.emplace_back(
                    m_resourceRegistry.GetCompiledResource( input ),
                    input.m_passAccess
                );
            }

            for ( RGNodeResource& output : node.m_outputs )
            {
                transitionResources.emplace_back(
                    m_resourceRegistry.GetCompiledResource( output ),
                    output.m_passAccess
                );   
            }

            TransitionResourceBatched( transitionResources );

            // execute node callback
            //-------------------------------------------------------------------------

            auto& commandContext = m_renderCommandContexts[m_currentDeviceFrameIndex];
            commandContext.m_pExecutingNode = &node;
            node.m_executionCallback( commandContext );
            commandContext.m_pExecutingNode = nullptr;
        }

        void RenderGraph::PresentNode( RGExecutableNode& node, RHI::RHITexture* pSwapchainTexture )
        {
            EE_ASSERT( pSwapchainTexture != nullptr );

            // Transite resource to target pipeline barrier states
            //-------------------------------------------------------------------------

            size_t totalResourceCount = node.m_inputs.size() + node.m_outputs.size();
            TVector<TPair<RGCompiledResource&, RHI::RenderResourceAccessState>> transitionResources;
            transitionResources.reserve( totalResourceCount );

            for ( RGNodeResource& input : node.m_inputs )
            {
                transitionResources.emplace_back(
                    m_resourceRegistry.GetCompiledResource( input ),
                    input.m_passAccess
                );
            }

            for ( RGNodeResource& output : node.m_outputs )
            {
                // swapchain texture resource importation
                auto& compiledOutput = m_resourceRegistry.GetCompiledResource( output );
                if ( compiledOutput.IsSwapchainImportedResource() )
                {
                    compiledOutput.m_importedResource->m_pImportedResource = pSwapchainTexture;
                    compiledOutput.GetResource<RGResourceTagTexture>() = pSwapchainTexture;
                }

                transitionResources.emplace_back(
                    compiledOutput,
                    output.m_passAccess
                );
            }

            TransitionResourceBatched( transitionResources );

            // execute node callback
            //-------------------------------------------------------------------------
            
            auto& commandContext = m_renderCommandContexts[m_currentDeviceFrameIndex];
            commandContext.m_pExecutingNode = &node;
            node.m_executionCallback( commandContext );
            commandContext.m_pExecutingNode = nullptr;

            // manually transition final present texture to present barrier state
            //-------------------------------------------------------------------------

            RHI::RenderResourceAccessState finalAccessState;
            finalAccessState.SetSkipSyncIfContinuous( false );
            finalAccessState.TransiteTo( RHI::RenderResourceBarrierState::Present );
            TransitionResource( m_resourceRegistry.GetCompiledResource( node.m_outputs[0] ), eastl::move( finalAccessState ) );
        }
    }
}