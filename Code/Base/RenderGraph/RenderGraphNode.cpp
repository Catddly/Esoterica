#include "RenderGraphNode.h"
#include "RenderGraphResourceRegistry.h"
#include "Base/Render/RenderPipelineRegistry.h"
#include "Base/RHI/Resource/RHIPipelineState.h"

namespace EE
{
	namespace RG
	{
		RGNode::RGNode()
			: RGNode( "Unknown Pass", std::numeric_limits<NodeID>::max() )
		{}

		RGNode::RGNode( String const& nodeName, NodeID id )
			: m_passName( nodeName ), m_id( id )
		{}

        //-------------------------------------------------------------------------

        bool RGNode::ReadyToExecute( RG::RGResourceRegistry* pRGResourceRegistry ) const
        {
            EE_ASSERT( pRGResourceRegistry );
            EE_ASSERT( m_pipelineHandle.IsValid() );

            auto* pPipelineRegistry = pRGResourceRegistry->GetPipelineRegistry();
            EE_ASSERT( pPipelineRegistry );

            return pPipelineRegistry->IsPipelineReady( m_pipelineHandle );
        }

        RGExecutableNode RGNode::IntoExecutableNode( RG::RGResourceRegistry* pRGResourceRegistry ) &&
        {
            EE_ASSERT( pRGResourceRegistry );

            auto* pPipelineRegistry = pRGResourceRegistry->GetPipelineRegistry();
            EE_ASSERT( pPipelineRegistry );

            auto* pPipelineState = pPipelineRegistry->GetPipeline( m_pipelineHandle );
            //EE_ASSERT( pPipelineState->GetPipelineType() == m_pipelineHandle.GetPipelineType() );

            RGExecutableNode executableNode;
            executableNode.m_pInputs = eastl::exchange( m_pInputs, {} );
            executableNode.m_pOutputs = eastl::exchange( m_pOutputs, {} );
            executableNode.m_passName = eastl::exchange( m_passName, {} );
            executableNode.m_id = eastl::exchange( m_id, {} );
            executableNode.m_pPipelineState = pPipelineState;
            executableNode.m_executionCallback = eastl::exchange( m_executionCallback, {} );

            return executableNode;
        }

		//-------------------------------------------------------------------------

		RGNodeResource::RGNodeResource( _Impl::RGResourceID slotID, RHI::RenderResourceAccessState access )
			: m_slotID( slotID ), m_passAccess( access )
		{}

		//-------------------------------------------------------------------------
	}
}