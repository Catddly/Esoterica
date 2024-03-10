#include "RHICommandBufferPool.h"
#include "RHICommandQueue.h"

namespace EE::RHI
{
    RHICommandBufferPool::RHICommandBufferPool( ERHIType rhiType )
        : RHITaggedType( rhiType )
    {
        m_threadId = Threading::GetCurrentThreadID();
    }

    uint32_t RHICommandBufferPool::GetQueueIndex() const
    {
        return m_pCommandQueue->GetQueueIndex();
    }

}