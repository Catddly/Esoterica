#pragma once

#include "RHITaggedType.h"
#include "Base/Render/RenderAPI.h"
#include "Base/Types/Arrays.h"
#include "Base/Memory/Pointers.h"
#include "Base/Threading/Threading.h"

namespace EE::RHI
{
    enum class CommandQueueType : uint8_t
    {
        Graphic = 0,
        Compute,
        Transfer,
        Unknown
    };

    class RHICommandBuffer;
    class RHISemaphore;

    // TODO: reference counting
    // It is now programmer's job to keep this resources alive until the command is submitted and finished.
    struct CommandQueueRenderCommand
    {
        RHI::RHICommandBuffer*              m_pCommandBuffer;
        TVector<RHI::RHISemaphore*>         m_pWaitSemaphores;
        TVector<RHI::RHISemaphore*>         m_pSignalSemaphores;
        TVector<Render::PipelineStage>      m_waitStages;
    };

    class RHICommandQueue : public RHITaggedType
    {
    public:

        RHICommandQueue( ERHIType rhiType );
        virtual ~RHICommandQueue() = default;

        RHICommandQueue( RHICommandQueue const& ) = delete;
        RHICommandQueue& operator=( RHICommandQueue const& ) = delete;

        RHICommandQueue( RHICommandQueue&& ) = default;
        RHICommandQueue& operator=( RHICommandQueue&& ) = default;

        //-------------------------------------------------------------------------

        // Note: In vulkan, this will return this queue family index of this queue.
        virtual uint32_t GetQueueIndex() const = 0;
        virtual CommandQueueType GetType() const = 0;

        virtual void WaitUntilIdle() const = 0;

        // Submit render command to this queue
        void Submit( CommandQueueRenderCommand const& task );

        // Add a transfer synchronization point.
        // This ensure all transfer commands submitted before this point must be valid and visible to commands after.
        //virtual void AddTransferCommandSyncPoint() = 0;

        virtual void FlushToGPU() = 0;

    protected:

        Threading::Mutex                            m_submitMutex;

        Threading::ThreadID                         m_threadId;
        TVector<CommandQueueRenderCommand>          m_waitToSubmitTasks;
    };
}