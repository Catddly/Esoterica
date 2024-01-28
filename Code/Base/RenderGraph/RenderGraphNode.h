#pragma once

#include "Base/_Module/API.h"
#include "RenderGraphResource.h"
//#include "RenderGraphContext.h"
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
        friend class RenderGraphResolver;

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
    
    class RGRenderCommandContext;

    class EE_BASE_API RGNode
    {
        friend class RenderGraph;
        friend class RGNodeBuilder;
        friend class RGExecutableNode;
        friend class RenderGraphResolver;

        using ExecutionCallbackFunc = TFunction<void( RGRenderCommandContext& context )>;

    public:

        RGNode();
        RGNode( String const& nodeName, NodeID id );

        RGNode( RGNode const& ) = delete;
        RGNode& operator=( RGNode const& ) = delete;

        RGNode( RGNode&& rhs ) noexcept
        {
            m_inputs = eastl::exchange( rhs.m_inputs, {} );
            m_outputs = eastl::exchange( rhs.m_outputs, {} );
            m_passName = eastl::exchange( rhs.m_passName, {} );
            m_id = eastl::exchange( rhs.m_id, {} );
            m_pipelineHandle = eastl::exchange( rhs.m_pipelineHandle, {} );
            m_executionCallback = eastl::move( rhs.m_executionCallback );
        }
        RGNode& operator=( RGNode&& rhs ) noexcept
        {
            RGNode copy( eastl::move( rhs ) );
            copy.swap( *this );
            return *this;
        }

    public:

        friend void swap( RGNode& lhs, RGNode& rhs ) noexcept
        {
            lhs.swap( rhs );
        }

        void swap( RGNode& rhs ) noexcept
        {
            eastl::swap( m_inputs, rhs.m_inputs );
            eastl::swap( m_outputs, rhs.m_outputs );
            eastl::swap( m_passName, rhs.m_passName );
            eastl::swap( m_id, rhs.m_id );
            eastl::swap( m_pipelineHandle, rhs.m_pipelineHandle );
            eastl::swap( m_executionCallback, rhs.m_executionCallback );
        }

    private:

        inline bool HadRegisteredPipeline() const { return m_bHasPipeline; }

        inline bool IsReadyToExecute( RG::RGResourceRegistry* pRGResourceRegistry ) const;
        RGExecutableNode IntoExecutableNode( RG::RGResourceRegistry* pRGResourceRegistry ) &&;

	private:

		TVector<RGNodeResource>					m_inputs;
		TVector<RGNodeResource>					m_outputs;

		String									m_passName;
		NodeID									m_id;

        Render::PipelineHandle                  m_pipelineHandle;
        ExecutionCallbackFunc                   m_executionCallback;

        bool                                    m_bHasPipeline = false;
    };

    class RGExecutableNode
    {
        friend class RGNode;
        friend class RenderGraph;
        friend class RGRenderCommandContext;

    public:

        RGExecutableNode() = default;

        RGExecutableNode( RGExecutableNode const& ) = delete;
        RGExecutableNode& operator=( RGExecutableNode const& ) = delete;

        RGExecutableNode( RGExecutableNode&& rhs ) noexcept
        {
            m_inputs = eastl::exchange( rhs.m_inputs, {} );
            m_outputs = eastl::exchange( rhs.m_outputs, {} );
            m_passName = eastl::exchange( rhs.m_passName, {} );
            m_id = eastl::exchange( rhs.m_id, {} );
            m_pPipelineState = eastl::exchange( rhs.m_pPipelineState, {} );
            m_executionCallback = eastl::exchange( rhs.m_executionCallback, {} );
        }
        RGExecutableNode& operator=( RGExecutableNode&& rhs ) noexcept
        {
            RGExecutableNode copy( eastl::move( rhs ) );
            copy.swap( *this );
            return *this;
        }

    public:

        friend void swap( RGExecutableNode& lhs, RGExecutableNode& rhs ) noexcept
        {
            lhs.swap( rhs );
        }

        void swap( RGExecutableNode& rhs ) noexcept
        {
            eastl::swap( m_inputs, rhs.m_inputs );
            eastl::swap( m_outputs, rhs.m_outputs );
            eastl::swap( m_passName, rhs.m_passName );
            eastl::swap( m_id, rhs.m_id );
            eastl::swap( m_pPipelineState, rhs.m_pPipelineState );
            eastl::swap( m_executionCallback, rhs.m_executionCallback );
        }

    private:

        TVector<RGNodeResource>					m_inputs;
        TVector<RGNodeResource>					m_outputs;

        String									m_passName;
        NodeID									m_id;

        // Note: Lifetime safety of RHI::RHIPipelineState* and
        //       other resource references used in ExecutionCallbackFunc must be guaranteed.
        RHI::RHIPipelineState*                  m_pPipelineState = nullptr;
        RGNode::ExecutionCallbackFunc           m_executionCallback;
    };
}