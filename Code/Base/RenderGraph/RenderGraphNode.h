#pragma once

#include "Base/_Module/API.h"
#include "RenderGraphResource.h"
#include "RenderGraphContext.h"
#include "Base/Types/Function.h"
#include "Base/Types/Arrays.h"
#include "Base/Types/String.h"
// TODO: may be decouple pipeline barrier from command buffer 
#include "Base/RHI/RHICommandBuffer.h"
#include "Base/Render/RenderPipelineRegistry.h"

#include <limits>

namespace EE::RHI
{
    class RHIPipelineState;
}

namespace EE::RG
{
	class EE_BASE_API RGNodeResource
	{
        friend class RenderGraph;
        friend class RGResourceRegistry;

	public:

		RGNodeResource() = default;
		RGNodeResource( _Impl::RGResourceID slotID, RHI::RenderResourceAccessState access );

	private:

        _Impl::RGResourceID					    m_slotID;
        RHI::RenderResourceAccessState		    m_passAccess;
	};

	typedef uint32_t NodeID;

    class RGExecutableNode;
    class RGResourceRegistry;

    class EE_BASE_API RGNode
    {
        friend class RenderGraph;
        friend class RGNodeBuilder;
        friend class RGExecutableNode;

        using ExecutionCallbackFunc = TFunction<void( RGRenderCommandContext& context )>;

    public:

        RGNode();
        RGNode( String const& nodeName, NodeID id );

        RGNode( RGNode const& ) = delete;
        RGNode& operator=( RGNode const& ) = delete;

        RGNode( RGNode&& rhs )
        {
            m_pInputs = eastl::exchange( rhs.m_pInputs, {} );
            m_pOutputs = eastl::exchange( rhs.m_pOutputs, {} );
            m_passName = eastl::exchange( rhs.m_passName, {} );
            m_id = eastl::exchange( rhs.m_id, {} );
            m_pipelineHandle = eastl::exchange( rhs.m_pipelineHandle, {} );
        }
        RGNode& operator=( RGNode&& rhs )
        {
            RGNode copy( eastl::move( rhs ) );
            copy.swap( *this );
            return *this;
        }

    public:

        friend void swap( RGNode& lhs, RGNode& rhs )
        {
            lhs.swap( rhs );
        }

        void swap( RGNode& rhs )
        {
            eastl::swap( m_pInputs, rhs.m_pInputs );
            eastl::swap( m_pOutputs, rhs.m_pOutputs );
            eastl::swap( m_passName, rhs.m_passName );
            eastl::swap( m_id, rhs.m_id );
            eastl::swap( m_pipelineHandle, rhs.m_pipelineHandle );
        }

    private:

        inline bool ReadyToExecute( RG::RGResourceRegistry* pRGResourceRegistry ) const;
        RGExecutableNode IntoExecutableNode( RG::RGResourceRegistry* pRGResourceRegistry ) &&;

	private:

		TVector<RGNodeResource>					m_pInputs;
		TVector<RGNodeResource>					m_pOutputs;

		String									m_passName;
		NodeID									m_id;

        Render::PipelineHandle                  m_pipelineHandle;
        ExecutionCallbackFunc                   m_executionCallback;
	};

    class RGExecutableNode
    {
        friend class RGNode;
        friend class RenderGraph;

    private:

        TVector<RGNodeResource>					m_pInputs;
        TVector<RGNodeResource>					m_pOutputs;

        String									m_passName;
        NodeID									m_id;

        // Note: Lifetime safety of RHI::RHIPipelineState* and
        //       other resource references used in ExecutionCallbackFunc must be guaranteed.
        RHI::RHIPipelineState*                  m_pPipelineState = nullptr;
        RGNode::ExecutionCallbackFunc           m_executionCallback;
    };
}