#pragma once

#include "Base/_Module/API.h"
#include "RenderGraphResource.h"
#include "RenderGraphNode.h"
#include "RenderGraphResourceRegistry.h"
#include "Base/RHI/Resource/RHIResourceCreationCommons.h"

namespace EE::RG
{
    // Helper class to build a render graph node.
    // User can register pipeline and define the resource usage in a certain pass.
    // TODO: [Safety Consideration] It is thread safe?
    // This is NOT thread safe and it is not multi-instances safe.
    // User can only use one RGNodeBuilder at a time, destroy it and then create a new one to continue.
    // It is forbidden to create a RGNodeBuilder, and then create another RGNodeBuilder when the previous instance is not Destructed yet.
    class EE_BASE_API RGNodeBuilder
    {
    public:

        RGNodeBuilder( RGResourceRegistry const& graphResourceRegistry, RGNode& node )
            : m_graphResourceRegistry( graphResourceRegistry ), m_node( node )
        {}

    public:

        // Node Render Pipeline Registration
        //-------------------------------------------------------------------------

        void RegisterRasterPipeline( RHI::RHIRasterPipelineStateCreateDesc pipelineDesc );
        void RegisterComputePipeline( Render::ComputePipelineDesc pipelineDesc );

        // Node Render Resource Read And Write Operations
        //-------------------------------------------------------------------------

        template <typename Tag>
        RGNodeResourceRef<Tag, RGResourceViewType::SRV> CommonRead( RGResourceHandle<Tag> const& pResource, RHI::RenderResourceBarrierState access );

        template <typename Tag>
        RGNodeResourceRef<Tag, RGResourceViewType::UAV> CommonWrite( RGResourceHandle<Tag>& pResource, RHI::RenderResourceBarrierState access );

        template <typename Tag>
        RGNodeResourceRef<Tag, RGResourceViewType::SRV> RasterRead( RGResourceHandle<Tag> const& pResource, RHI::RenderResourceBarrierState access );

        template <typename Tag>
        RGNodeResourceRef<Tag, RGResourceViewType::RT>  RasterWrite( RGResourceHandle<Tag>& pResource, RHI::RenderResourceBarrierState access );

        // Node Render Command
        //-------------------------------------------------------------------------

        void Execute( RGNode::ExecutionCallbackFunc const& executionCallback );

    private:

        // Notify resource registry that a resource is read by this node with specified barrier state.
        template <typename Tag, RGResourceViewType RVT>
        RGNodeResourceRef<Tag, RVT> ReadImpl( RGResourceHandle<Tag> const& pResource, RHI::RenderResourceBarrierState access );

        // Notify resource registry that a resource is written by this node with specified barrier state.
        template <typename Tag, RGResourceViewType RVT>
        RGNodeResourceRef<Tag, RVT> WriteImpl( RGResourceHandle<Tag>& pResource, RHI::RenderResourceBarrierState access );

    private:

        // Safety: All reference holds during the life time of RGNodeBuilder.
        RGResourceRegistry const&				m_graphResourceRegistry;
        // Warning: In multi-thread, If the TVector in RenderGraph is reallocated, this will be a dangling reference!!
        RGNode&									m_node;
    };

    template <typename Tag>
    RGNodeResourceRef<Tag, RGResourceViewType::SRV> RGNodeBuilder::CommonRead( RGResourceHandle<Tag> const& pResource, RHI::RenderResourceBarrierState access )
    {
        // TODO: We should make a runtime check.
        EE_ASSERT( RHI::Barrier::IsCommonReadOnlyAccess( access ) );
        EE_ASSERT( Threading::IsMainThread() );
        return ReadImpl<Tag, RGResourceViewType::SRV>( pResource, access );
    }

    template <typename Tag>
    RGNodeResourceRef<Tag, RGResourceViewType::UAV> RGNodeBuilder::CommonWrite( RGResourceHandle<Tag>& pResource, RHI::RenderResourceBarrierState access )
    {
        // TODO: We should make a runtime check.
        EE_ASSERT( RHI::Barrier::IsCommonWriteAccess( access ) );
        EE_ASSERT( Threading::IsMainThread() );
        return WriteImpl<Tag, RGResourceViewType::UAV>( pResource, access );
    }

    template <typename Tag>
    RGNodeResourceRef<Tag, RGResourceViewType::SRV> RGNodeBuilder::RasterRead( RGResourceHandle<Tag> const& pResource, RHI::RenderResourceBarrierState access )
    {
        // TODO: We should make a runtime check.
        EE_ASSERT( RHI::Barrier::IsRasterReadOnlyAccess( access ) );
        EE_ASSERT( Threading::IsMainThread() );
        return ReadImpl<Tag, RGResourceViewType::SRV>( pResource, access );
    }

    template <typename Tag>
    RGNodeResourceRef<Tag, RGResourceViewType::RT> RGNodeBuilder::RasterWrite( RGResourceHandle<Tag>& pResource, RHI::RenderResourceBarrierState access )
    {
        // TODO: We should make a runtime check.
        EE_ASSERT( RHI::Barrier::IsRasterWriteAccess( access ) );
        EE_ASSERT( Threading::IsMainThread() );
        return WriteImpl<Tag, RGResourceViewType::RT>( pResource, access );
    }

    //-------------------------------------------------------------------------

    template <typename Tag, RGResourceViewType RVT>
    RGNodeResourceRef<Tag, RVT> RGNodeBuilder::ReadImpl( RGResourceHandle<Tag> const& pResource, RHI::RenderResourceBarrierState access )
    {
        EE_STATIC_ASSERT( ( std::is_base_of<RGResourceTagTypeBase<Tag>, Tag>::value ), "Invalid render graph resource tag!" );

        typedef typename Tag::DescType DescType;
        typedef typename std::add_lvalue_reference_t<std::add_const_t<DescType>> DescCVType;

        EE_ASSERT( pResource.m_slotID.IsValid() );

        // Note: only true to skip sync for read access
        EE::RHI::RenderResourceAccessState accessState( access, true );
        m_node.m_inputs.emplace_back( pResource.m_slotID, std::move( accessState ) );

        // fetch graph resource from render graph
        DescCVType desc = m_graphResourceRegistry.GetRegisteredResource( pResource.m_slotID ).GetDesc<Tag>();
        // return a lifetime limited reference to it.
        // Note: here we take a const reference description of a resource.
        // The whole system should make sure the lifetime of RGNodeResourceRef should be less than the resources inside RGResourceRegistry.
        return RGNodeResourceRef<Tag, RVT>( desc, pResource.m_slotID );
    }

    template <typename Tag, RGResourceViewType RVT>
    RGNodeResourceRef<Tag, RVT> RGNodeBuilder::WriteImpl( RGResourceHandle<Tag>& pResource, RHI::RenderResourceBarrierState access )
    {
        EE_STATIC_ASSERT( ( std::is_base_of<RGResourceTagTypeBase<Tag>, Tag>::value ), "Invalid render graph resource tag!" );

        typedef typename Tag::DescType DescType;
        typedef typename std::add_lvalue_reference_t<std::add_const_t<DescType>> DescCVType;

        EE_ASSERT( pResource.m_slotID.IsValid() );

        // Note: only true to skip sync for read access
        EE::RHI::RenderResourceAccessState accessState( access, false );
        m_node.m_outputs.emplace_back( pResource.m_slotID, std::move( accessState ) );

        // fetch graph resource from render graph
        DescCVType desc = m_graphResourceRegistry.GetRegisteredResource( pResource.m_slotID ).GetDesc<Tag>();

        // after write operation, this resource is considered a new resource
        _Impl::RGResourceID newSlotID = pResource.m_slotID;
        newSlotID.Expire();

        // return a lifetime limited reference to it.
        // Note: here we take a const reference description of a resource.
        // The whole system should make sure the lifetime of RGNodeResourceRef should be less than the resources inside RGResourceRegistry.
        return RGNodeResourceRef<Tag, RVT>( desc, newSlotID );
    }
}