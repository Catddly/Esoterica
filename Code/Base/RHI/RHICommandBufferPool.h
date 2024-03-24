#pragma once

#include "Base/RHI/RHITaggedType.h"
#include "Base/Types/Arrays.h"
#include "Base/Memory/Pointers.h"
#include "Base/Render/RenderAPI.h"
#include "Base/Threading/Threading.h"

namespace EE::RHI
{
    class RHICommandBuffer;
    class RHISemaphore;
    class RHICommandQueue;

    class RHICommandBufferPool : public RHITaggedType
    {
        friend class RHICommandQueue;

    public:

        RHICommandBufferPool( ERHIType rhiType );
        virtual ~RHICommandBufferPool() = default;

        RHICommandBufferPool( RHICommandBufferPool const& ) = delete;
        RHICommandBufferPool& operator=( RHICommandBufferPool const& ) = delete;

        RHICommandBufferPool( RHICommandBufferPool&& ) = default;
        RHICommandBufferPool& operator=( RHICommandBufferPool&& ) = default;

        virtual RHICommandBuffer* Allocate() = 0;
        virtual void Restore( RHICommandBuffer* ) = 0;

        // Reset whole command buffer pool for next command allocation and record.
        virtual void Reset() = 0;

        // Wait until all commands inside this command buffer pool are finished.
        virtual void WaitUntilAllCommandsFinished() = 0;

        // Submit command buffer 
        virtual void Submit( RHICommandBuffer* pCommandBuffer, TSpan<RHI::RHISemaphore*> pWaitSemaphores, TSpan<RHI::RHISemaphore*> pSignalSemaphores, TSpan<Render::PipelineStage> waitStages ) = 0;

    public:

        Threading::ThreadID GetThreadID() const { return m_threadId; }

        uint32_t GetQueueIndex() const;

    private:

        Threading::ThreadID                     m_threadId;

    protected:

        RHI::RHICommandQueue*                   m_pCommandQueue = nullptr;
    };

    //-------------------------------------------------------------------------

    using RHICommandBufferRef = TTSSharedPtr<RHICommandBuffer>;
}