#include "RenderGraphNodeBuilder.h"
#include "Base/Threading/Threading.h"

namespace EE::RG
{
    void RGNodeBuilder::RegisterRasterPipeline( RHI::RHIRasterPipelineStateCreateDesc pipelineDesc )
    {
        EE_ASSERT( Threading::IsMainThread() );
        EE_ASSERT( pipelineDesc.IsValid() );

        // Immediately register pipeline into graph, next frame it can start the loading process
        m_node.m_pipelineHandle = m_graphResourceRegistry.RegisterRasterPipeline( pipelineDesc );
    }

    void RGNodeBuilder::RegisterComputePipeline( RHI::RHIComputePipelineStateCreateDesc pipelineDesc )
    {
        EE_ASSERT( Threading::IsMainThread() );
        EE_ASSERT( pipelineDesc.IsValid() );

        // Immediately register pipeline into graph, next frame it can start the loading process
        m_node.m_pipelineHandle = m_graphResourceRegistry.RegisterComputePipeline( pipelineDesc );
    }

    void RGNodeBuilder::Execute( RGNode::ExecutionCallbackFunc const& executionCallback )
    {
        m_node.m_executionCallback = executionCallback;
    }
}