#pragma once

#include "RHIResource.h"
#include "RHIResourceCreationCommons.h"

namespace EE::RHI
{
    // Pipeline State
    //-------------------------------------------------------------------------

    using SetDescriptorLayout = TMap<uint32_t, EBindingResourceType>;

    class EE_BASE_API RHIPipelineState : public RHIResource
    {
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

    public:

        RHIRasterPipelineStateCreateDesc const& GetDesc() const { return m_desc; }

        //-------------------------------------------------------------------------

        inline virtual RHI::RHIPipelineType GetPipelineType() const override { return RHI::RHIPipelineType::Raster; }

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

    public:

        RHIComputePipelineStateCreateDesc const& GetDesc() const { return m_desc; }

        //-------------------------------------------------------------------------

        inline virtual RHI::RHIPipelineType GetPipelineType() const override { return RHI::RHIPipelineType::Compute; }

        virtual uint32_t GetThreadGroupSizeX() const = 0;
        virtual uint32_t GetThreadGroupSizeY() const = 0;
        virtual uint32_t GetThreadGroupSizeZ() const = 0;

    protected:

        RHIComputePipelineStateCreateDesc                m_desc;
    };
}