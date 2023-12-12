#pragma once

#include "RHIResource.h"
#include "RHIResourceCreationCommons.h"

namespace EE::RHI
{
    // Pipeline State
    //-------------------------------------------------------------------------

    class EE_BASE_API RHIPipelineState : public RHIResource
    {
    public:

        using SetDescriptorLayout = TMap<uint32_t, EBindingResourceType>;

    public:

        RHIPipelineState( ERHIType rhiType = ERHIType::Invalid )
            : RHIResource( rhiType )
        {}
        virtual ~RHIPipelineState() = default;

        virtual RHIPipelineType GetPipelineType() const = 0;

        virtual TInlineVector<SetDescriptorLayout, RHI::NumMaxResourceBindingSet> const& GetResourceSetLayouts() const = 0;
    };

    class EE_BASE_API RHIRasterPipelineState : public RHIPipelineState
    {
    public:

        RHIRasterPipelineState( ERHIType rhiType = ERHIType::Invalid )
            : RHIPipelineState( rhiType )
        {}
        virtual ~RHIRasterPipelineState() = default;

        inline virtual RHIPipelineType GetPipelineType() const override { return RHIPipelineType::Raster; }

    protected:

        RHIRasterPipelineStateCreateDesc                m_desc;
    };

    class EE_BASE_API RHIComputePipelineState : public RHIPipelineState
    {
    public:

        RHIComputePipelineState( ERHIType rhiType = ERHIType::Invalid )
            : RHIPipelineState( rhiType )
        {}
        virtual ~RHIComputePipelineState() = default;

        inline virtual RHIPipelineType GetPipelineType() const override { return RHIPipelineType::Compute; }

    protected:
    };
}