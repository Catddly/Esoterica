#pragma once

#include "Base/_Module/API.h"
#include "RenderAPI.h"
#include "Base/Serialization/BinarySerialization.h"
#include "Base/Types/Arrays.h"
#include "Base/Math/Math.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    enum class VertexFormat
    {
        Unknown = 0,
        None,
        StaticMesh,
        SkeletalMesh,
    };

    // CPU format for the static mesh vertex - this is what the mesh compiler fills the vertex data array with
    struct StaticMeshVertex
    {
        Float4  m_position;
        Float4  m_normal;
        Float2  m_UV0;
        Float2  m_UV1;
    };

    // CPU format for the skeletal mesh vertex - this is what the mesh compiler fills the vertex data array with
    struct SkeletalMeshVertex : public StaticMeshVertex
    {
        Int4    m_boneIndices;
        Float4  m_boneWeights;
    };

    //-------------------------------------------------------------------------

    struct EE_BASE_API VertexLayoutDescriptor
    {
        EE_SERIALIZE( m_elementDescriptors, m_byteSize );

        enum class VertexDataFormat : uint8_t
        {
            Unknown = 0,
            R8UInt,
            RG8UInt,
            RGBA8UInt,

            R8Unorm,
            RG8Unorm,
            RGBA8Unorm,

            R32UInt,
            RG32UInt,
            RGB32UInt,
            RGBA32UInt,

            R32SInt,
            RG32SInt,
            RGB32SInt,
            RGBA32SInt,
            
            R16Float,
            RG16Float,
            RGBA16Float,
            
            R32Float,
            RG32Float,
            RGB32Float,
            RGBA32Float,

            X32Float,

            Count
        };

        struct ElementDescriptor
        {
            EE_SERIALIZE( m_semantic, m_format, m_semanticIndex, m_offset );

            ElementDescriptor() = default;

            ElementDescriptor( DataSemantic semantic, VertexDataFormat format, uint16_t semanticIndex, uint16_t offset ) : m_semantic( semantic )
                , m_format( format )
                , m_semanticIndex( semanticIndex )
                , m_offset( offset )
            {}

            DataSemantic          m_semantic = DataSemantic::None;
            // TODO: have its own format type
            VertexDataFormat      m_format = VertexDataFormat::Unknown;
            uint16_t              m_semanticIndex = 0;
            uint16_t              m_offset = 0;
        };

    public:

        inline bool IsValid() const { return m_byteSize > 0 && !m_elementDescriptors.empty(); }

        void CalculateElementOffsets();
        void CalculateByteSize();

    public:

        TInlineVector<ElementDescriptor, 6>     m_elementDescriptors;
        uint32_t                                m_byteSize = 0;             // The total byte size per vertex
    };

    EE_BASE_API uint32_t GetDataTypeFormatByteSize( VertexLayoutDescriptor::VertexDataFormat format );

    //-------------------------------------------------------------------------

    namespace VertexLayoutRegistry
    {
        EE_BASE_API VertexLayoutDescriptor GetDescriptorForFormat( VertexFormat format );
    }
}