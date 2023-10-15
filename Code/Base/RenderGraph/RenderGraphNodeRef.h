#pragma once

#include "RenderGraphResource.h"

#include <type_traits>

namespace EE::RG
{
    template <typename Tag, RGResourceViewType RVT>
    class RGNodeResourceRef
    {
        friend class RGResourceRegistry;

        static_assert( std::is_base_of<RGResourceTagTypeBase<Tag>, Tag>::value, "Invalid render graph resource tag!" );

        typedef typename Tag::DescType DescType;
        typedef typename std::add_lvalue_reference_t<std::add_const_t<DescType>> DescCVType;

    public:

        RGNodeResourceRef( DescCVType desc, _Impl::RGResourceID slotID );

        inline DescCVType GetDesc() const { return m_desc; }

    private:
        // Safety: This reference is always held by RenderGraph during the life time of RGNodeResourceRef.
        DescCVType						m_desc;
        _Impl::RGResourceID				m_slotID;
    };

    //-------------------------------------------------------------------------

    template<typename Tag, RGResourceViewType RVT>
    RGNodeResourceRef<Tag, RVT>::RGNodeResourceRef( DescCVType desc, _Impl::RGResourceID slotID )
        : m_desc( desc ), m_slotID( slotID )
    {}
}