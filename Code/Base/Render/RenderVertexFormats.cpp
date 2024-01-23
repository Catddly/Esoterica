#include "RenderVertexFormats.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    static uint32_t const g_dataTypeSizes[] =
    {
        0,

        1,
        2,
        4,

        1,
        2,
        4,

        4,
        8,
        12,
        16,

        4,
        8,
        12,
        16,

        2,
        4,
        8,

        4,
        8,
        12,
        16,

        4
    };

    static_assert( sizeof( g_dataTypeSizes ) / sizeof( uint32_t ) == (uint32_t) VertexLayoutDescriptor::VertexDataFormat::Count, "Mismatched data type and size arrays" );

    uint32_t GetDataTypeFormatByteSize( VertexLayoutDescriptor::VertexDataFormat format )
    {
        uint32_t const formatIdx = (uint32_t) format;
        EE_ASSERT( formatIdx < (uint32_t) VertexLayoutDescriptor::VertexDataFormat::Count );
        uint32_t const size = g_dataTypeSizes[formatIdx];
        return size;
    }

    //-------------------------------------------------------------------------

    void VertexLayoutDescriptor::CalculateElementOffsets()
    {
        uint16_t currentOffset = 0;
        for ( auto& vertexElementDesc : m_elementDescriptors )
        {
            vertexElementDesc.m_offset = currentOffset;
            currentOffset += static_cast<uint16_t>( GetDataTypeFormatByteSize( vertexElementDesc.m_format ) );

            EE_ASSERT( Math::IsAlignTo( currentOffset, static_cast<uint16_t>( sizeof( float ) ) ) );
        }
    }

    void VertexLayoutDescriptor::CalculateByteSize()
    {
        m_byteSize = 0;
        for ( auto const& vertexElementDesc : m_elementDescriptors )
        {
            m_byteSize += GetDataTypeFormatByteSize( vertexElementDesc.m_format );
        }
    }

    //-------------------------------------------------------------------------

    namespace VertexLayoutRegistry
    {
        VertexLayoutDescriptor GetDescriptorForFormat( VertexFormat format )
        {
            EE_ASSERT( format != VertexFormat::Unknown );

            VertexLayoutDescriptor layoutDesc;

            if ( format == VertexFormat::StaticMesh )
            {
                layoutDesc.m_elementDescriptors.push_back( VertexLayoutDescriptor::ElementDescriptor( DataSemantic::Position, VertexLayoutDescriptor::VertexDataFormat::RGBA32Float, 0, 0 ) );
                layoutDesc.m_elementDescriptors.push_back( VertexLayoutDescriptor::ElementDescriptor( DataSemantic::Normal, VertexLayoutDescriptor::VertexDataFormat::RGBA32Float, 0, 16 ) );
                layoutDesc.m_elementDescriptors.push_back( VertexLayoutDescriptor::ElementDescriptor( DataSemantic::TexCoord, VertexLayoutDescriptor::VertexDataFormat::RG32Float, 0, 32 ) );
                layoutDesc.m_elementDescriptors.push_back( VertexLayoutDescriptor::ElementDescriptor( DataSemantic::TexCoord, VertexLayoutDescriptor::VertexDataFormat::RG32Float, 1, 40 ) );

            }
            else if ( format == VertexFormat::SkeletalMesh )
            {
                layoutDesc.m_elementDescriptors.push_back( VertexLayoutDescriptor::ElementDescriptor( DataSemantic::Position, VertexLayoutDescriptor::VertexDataFormat::RGBA32Float, 0, 0 ) );
                layoutDesc.m_elementDescriptors.push_back( VertexLayoutDescriptor::ElementDescriptor( DataSemantic::Normal, VertexLayoutDescriptor::VertexDataFormat::RGBA32Float, 0, 16 ) );
                layoutDesc.m_elementDescriptors.push_back( VertexLayoutDescriptor::ElementDescriptor( DataSemantic::TexCoord, VertexLayoutDescriptor::VertexDataFormat::RG32Float, 0, 32 ) );
                layoutDesc.m_elementDescriptors.push_back( VertexLayoutDescriptor::ElementDescriptor( DataSemantic::TexCoord, VertexLayoutDescriptor::VertexDataFormat::RG32Float, 1, 40 ) );

                layoutDesc.m_elementDescriptors.push_back( VertexLayoutDescriptor::ElementDescriptor( DataSemantic::BlendIndex, VertexLayoutDescriptor::VertexDataFormat::RGBA32SInt, 0, 48 ) );
                layoutDesc.m_elementDescriptors.push_back( VertexLayoutDescriptor::ElementDescriptor( DataSemantic::BlendWeight, VertexLayoutDescriptor::VertexDataFormat::RGBA32Float, 0, 64 ) );
            }

            //-------------------------------------------------------------------------

            layoutDesc.CalculateByteSize();
            return layoutDesc;
        }
    }
}