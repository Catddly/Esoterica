#pragma once

#include "Base/_Module/API.h"

#include "RenderGraphContext.h"
#include "RenderGraphResourceRegistry.h"
#include "RenderGraphNode.h"
#include "RenderGraphNodeBuilder.h"
#include "Base/Types/Arrays.h"
#include "Base/Types/Tuple.h"
#include "Base/Types/String.h"
#include "Base/RHI/RHIDevice.h"
// TODO: may be decouple pipeline barrier from command buffer 
#include "Base/RHI/RHICommandBuffer.h"

namespace EE::Render
{
    class RenderTarget;
    class SwapchainRenderTarget;
}

namespace EE::RHI
{
    class RHIDevice;
    class RHISwapchain;
    class RHIResource;
    class RHITexture;
    class RHIBuffer;
}

namespace EE::RG
{
    class RGCompiledResource;

	class EE_BASE_API RenderGraph
	{
		friend class RGNodeBuilder;
        friend class RGRenderCommandContext;
        friend class RGBoundPipeline;

	public:

		RenderGraph();
		RenderGraph( String const& graphName );
            
        inline void AttachToPipelineRegistry( Render::PipelineRegistry& pipelineRegistry ) { m_resourceRegistry.AttachToPipelineRegistry( pipelineRegistry ); }

	public:

        // Build Stage
        //-------------------------------------------------------------------------

		template <typename DescType, typename RTTag = typename DescType::RGResourceTypeTag>
		RGResourceHandle<RTTag> CreateResource( DescType const& desc );

        RGResourceHandle<RGResourceTagBuffer> ImportResource( RHI::RHIBuffer* pBuffer, RHI::RenderResourceBarrierState access );
        RGResourceHandle<RGResourceTagTexture> ImportResource( RHI::RHITexture* pTexture, RHI::RenderResourceBarrierState access );

        RGResourceHandle<RGResourceTagTexture> ImportResource( Render::RenderTarget const& renderTarget, RHI::RenderResourceBarrierState access );

		[[nodiscard]] RGNodeBuilder AddNode( String const& nodeName );

		#if EE_DEVELOPMENT_TOOLS
		void LogGraphNodes() const;
		#endif

        // Compilation Stage
        //-------------------------------------------------------------------------

        bool Compile( RHI::RHIDevice* pRhiDevice );

        // Execution Stage
        //-------------------------------------------------------------------------

        void Execute( RHI::RHIDevice* pRhiDevice );

        void Present( RHI::RHIDevice* pRhiDevice, Render::SwapchainRenderTarget& swapchainRt );

        // Cleanup Stage
        //-------------------------------------------------------------------------

        void Retire();
        void DestroyAllResources( RHI::RHIDevice* pRhiDevice );

    private:

        RGResourceHandle<RGResourceTagTexture> ImportSwapchainTextureResource( Render::RenderTarget const& swapchainRenderTarget );

        inline RGResourceRegistry&       GetResourceRegistry() { return m_resourceRegistry; };
        inline RGResourceRegistry const& GetResourceRegistry() const { return m_resourceRegistry; };

        void AllocateCommandContext( RHI::RHIDevice* pRhiDevice );
        void FlushCommandContext( RHI::RHIDevice* pRhiDevice );

        // Return -1 if failed to find the presentable node.
        int32_t FindPresentNodeIndex( TVector<RGExecutableNode> const& executionSequence ) const;

        void TransitionResource( RGCompiledResource& compiledResource, RHI::RenderResourceAccessState const& access );
        void TransitionResourceBatched( TSpan<TPair<RGCompiledResource&, RHI::RenderResourceAccessState>> transitionResources );

        // All resources used in this node should be in positioned. (i.e. theirs resource barrier states is correct)
        // This function will call the callback function of current node.
        // User should know that any resources used in this callback should alive, (i.e. theirs lifetime should longer than the call time point
        // of Execute() and Present() ) otherwise this will cause crashs.
        void ExecuteNode( RGExecutableNode& node );

        void PresentNode( RGExecutableNode& node, RHI::RHITexture* pSwapchainTexture );

	private:

		String									m_name;

		// TODO: use a real graph
		TVector<RGNode>							m_renderGraph;
        RGResourceRegistry                      m_resourceRegistry;

        // TODO: pack this two into a separate class.
        TVector<RGExecutableNode>               m_executeNodesSequence;
        TVector<RGExecutableNode>               m_presentNodesSequence;

        // Note: this render command context will match exact the device frame index.
        RGRenderCommandContext                  m_renderCommandContexts[RHI::RHIDevice::NumDeviceFramebufferCount];
        size_t                                  m_currentDeviceFrameIndex;
	};

	//-------------------------------------------------------------------------

	template <typename DescType, typename RTTag>
	RGResourceHandle<RTTag> RenderGraph::CreateResource( DescType const& desc )
	{
		static_assert( std::is_base_of<RGResourceTagTypeBase<RTTag>, RTTag>::value, "Invalid render graph resource tag!" );
		typedef typename RTTag::RGDescType RGDescType;

		EE_ASSERT( Threading::IsMainThread() );

		RGDescType rgDesc = {};
		rgDesc.m_desc = desc;

		_Impl::RGResourceID const id = m_resourceRegistry.RegisterResource<RGDescType>( rgDesc );
		RGResourceHandle<RTTag> handle;
		handle.m_slotID = id;
		handle.m_desc = desc;
		return handle;
	}

    //template <typename RTTag>
    //RGResourceHandle<RTTag> RenderGraph::ImportResource( RHI::RHIResource* pResource, RHI::RenderResourceBarrierState access )
    //{
    //    EE_ASSERT( Threading::IsMainThread() );

    //    if ( pResource->IsBuffer() )
    //    {
    //        RHI::RHIBuffer* pBuffer = static_cast<RHI::RHIBuffer*>( pResource );

    //        _Impl::RGBufferDesc rgDesc = {};
    //        EE::RG::BufferDesc bufferDesc;
    //        bufferDesc.m_desc = pBuffer->GetDesc();
    //        rgDesc.m_desc = bufferDesc;

    //        RGImportedResource importResource;
    //        importResource.m_pImportedResource = pResource;
    //        importResource.m_currentAccess = access;

    //        _Impl::RGResourceID const id = m_resourceRegistry.ImportResource<_Impl::RGBufferDesc>( std::move( rgDesc ), std::move( importResource ) );
    //        RGResourceHandle<RGResourceTagBuffer> handle;
    //        handle.m_slotID = id;
    //        handle.m_desc = bufferDesc;
    //        return handle;
    //    }
    //    else if ( pResource->IsTexture() )
    //    {
    //        RHI::RHITexture* pTexture = static_cast<RHI::RHITexture*>( pResource );

    //        _Impl::RGTextureDesc rgDesc = {};
    //        EE::RG::TextureDesc textureDesc;
    //        textureDesc.m_desc = pTexture->GetDesc();
    //        rgDesc.m_desc = textureDesc;

    //        RGImportedResource importResource;
    //        importResource.m_pImportedResource = pResource;
    //        importResource.m_currentAccess = access;

    //        _Impl::RGResourceID const id = m_resourceRegistry.ImportResource<_Impl::RGTextureDesc>( std::move( rgDesc ), std::move( importResource ) );
    //        RGResourceHandle<RGResourceTagTexture> handle;
    //        handle.m_slotID = id;
    //        handle.m_desc = textureDesc;
    //        return handle;
    //    }

    //    EE_UNIMPLEMENTED_FUNCTION();
    //    return {};
    //}
}