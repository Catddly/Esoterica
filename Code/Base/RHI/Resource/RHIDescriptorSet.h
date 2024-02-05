#pragma once

#include "RHIResource.h"
#include "RHIDeferReleasable.h"
#include "RHIResourceCreationCommons.h"

namespace EE::RHI
{
    class RHIDevice;

    // RHI implementations (pure function interface)
    struct IRHIDescriptorPoolReleaseImpl
    {
        virtual ~IRHIDescriptorPoolReleaseImpl() = default;

        virtual void Release( RHIDevice* pDevice, void* pSetPool ) = 0;
    };

    class DeferReleaseQueue;

    class RHIDescriptorPool : public RHIStaticDeferReleasable<RHIDescriptorPool>
    {
        friend class DeferReleaseQueue;
        friend class RHIDevice;

    public:

        void*                   m_pSetPoolHandle = nullptr;

    public:

        inline void SetRHIReleaseImpl( IRHIDescriptorPoolReleaseImpl* pImpl ) { m_pImpl = pImpl; }

    private:

        inline bool IsValid() const { return m_pSetPoolHandle && m_pImpl; }
        
        EE_BASE_API void Enqueue( DeferReleaseQueue& queue );

        inline void Release( RHIDevice* pDevice )
        {
            EE_ASSERT( pDevice && m_pImpl );
            m_pImpl->Release( pDevice, m_pSetPoolHandle );
        }

    private:

        IRHIDescriptorPoolReleaseImpl*               m_pImpl = nullptr;
    };
}