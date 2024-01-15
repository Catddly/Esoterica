#pragma once

#include "../RHITaggedType.h"

namespace EE::RHI
{
    class EE_BASE_API RHIResource : public RHITaggedType
    {
    public:

        RHIResource( ERHIType rhiType = ERHIType::Invalid )
            : RHITaggedType( rhiType )
        {}
        virtual ~RHIResource() = default;

        RHIResource( RHIResource const& ) = delete;
        RHIResource& operator=( RHIResource const& ) = delete;

        RHIResource( RHIResource&& ) = default;
        RHIResource& operator=( RHIResource&& ) = default;
    };

    class RHIDevice;

    class EE_BASE_API IRHIResourceWrapper
    {
    public:

        virtual ~IRHIResourceWrapper() = default;

    public:

        struct ResourceCreateParameters
        {
        };

    public:

        // Release resources own by this resource wrapper.
        virtual void Release( RHIDevice* pDevice ) = 0;

        inline bool IsInitialized() const { return m_isInitialized; }

    protected:

        // Initialize resources by pass unique create parameters.
        // Ensure type safety by using wrapper functions sush as Initialize( RHIDevice* pDevice, MyResourceCreateParameters const& createParams )
        // which MyResourceCreateParameters is derived from ResourceCreateParameters.
        virtual bool InitializeBase( RHIDevice* pDevice, ResourceCreateParameters const& createParams ) = 0;

    protected:

        bool                            m_isInitialized = false;
    };

}


