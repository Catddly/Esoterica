#pragma once

#include "RHIResource.h"
#include "RHIResourceCreationCommons.h"
#include "RHIDeferReleasable.h"

#include <type_traits>

namespace EE::RHI
{
    class RHIDevice;

    class EE_BASE_API RHIBuffer : public RHIResource, public RHIDynamicDeferReleasable
    {
        friend class DeferReleaseQueue;

    public:

        RHIBuffer( ERHIType rhiType = ERHIType::Invalid )
            : RHIResource( rhiType )
        {}
        virtual ~RHIBuffer() = default;

        inline RHIBufferCreateDesc const& GetDesc() const { return m_desc; }

    public:

        template <typename T>
        typename std::enable_if<std::is_pointer_v<T>, T>::type MapTo( RHIDevice* pDevice )
        {
            void* pMappedData = Map( pDevice );
            return reinterpret_cast<T>( pMappedData );
        }

        virtual void* Map( RHIDevice* pDevice ) = 0;
        virtual void  Unmap( RHIDevice* pDevice ) = 0;

    private:

        virtual void Enqueue( DeferReleaseQueue& queue ) override;

        inline virtual void Release( RHIDevice* pDevice ) override;

    protected:

        RHIBufferCreateDesc                         m_desc;
    };
}