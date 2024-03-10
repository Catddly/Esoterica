#include "RHICommandQueue.h"
#include "RHICommandBuffer.h"
#include "RHICommandBufferPool.h"

namespace EE::RHI
{
    RHICommandQueue::RHICommandQueue( ERHIType rhiType )
        : RHITaggedType( rhiType )
    {
        m_threadId = Threading::GetCurrentThreadID();
    }

    //-------------------------------------------------------------------------

    void RHICommandQueue::Submit( CommandQueueRenderCommand const& task )
    {
        Threading::ScopeLock lock( m_submitMutex );

        if ( task.m_pCommandBuffer->m_pCommandBufferPool->m_pCommandQueue == this )
        {
            m_waitToSubmitTasks.push_back( task );
        }
        else
        {
            EE_LOG_WARNING( "RHI", "RHICommandQueue", "Try to submit command buffer to inconsist queue. Command has been ignored.");
        }
    }
}