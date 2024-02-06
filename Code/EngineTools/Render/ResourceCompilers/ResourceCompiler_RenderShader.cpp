#include "ResourceCompiler_RenderShader.h"
#include "EngineTools/Render/ResourceDescriptors/ResourceDescriptor_RenderShader.h"

#include "Base/FileSystem/FileSystem.h"
#include "Base/Serialization/BinarySerialization.h"
#include "Base/Memory/Pointers.h"
#include "Base/Types/String.h"
#include "Base/Types/Set.h"
#include "Base/Utils/Sort.h"
#include "Base/RHI/Resource/RHIResourceCreationCommons.h"

#include "EngineTools/Render/ResourceCompilers/Platform/DxcShaderCompiler.h"

// DX11 compile header
#include <d3dcompiler.h>
// Spirv Reflection
#include <spirv_cross/spirv_cross.hpp>

//-------------------------------------------------------------------------

namespace EE::Render
{
    namespace
    {
        static char const* const g_semanticNames[] = { "POSITION", "NORMAL", "TANGENT", "BINORMAL", "COLOR", "TEXCOORD", "BLENDINDICES", "BLENDWEIGHTS" };

        EE_FORCE_INLINE static char const* GetNameForSemantic( DataSemantic semantic )
        {
            EE_ASSERT( semantic < DataSemantic::None );
            return g_semanticNames[(uint8_t) semantic];
        }

        EE_FORCE_INLINE static DataSemantic GetSemanticForName( std::string const& semantic )
        {
            size_t const numSemantics = sizeof( g_semanticNames ) / sizeof( g_semanticNames[0] );
            for ( size_t i = 0; i < numSemantics; ++i )
            {
                if ( semantic == g_semanticNames[i] )
                {
                    return DataSemantic( static_cast<uint8_t>( i ) );
                }

                // specialization
                if ( DataSemantic( static_cast<uint8_t>( i ) ) == DataSemantic::Color )
                {
                    if ( semantic == "COLOR0" || semantic == "COLOR1" )
                    {
                        return DataSemantic( static_cast<uint8_t>( i ) );
                    }
                }
                else if ( DataSemantic( static_cast<uint8_t>( i ) ) == DataSemantic::TexCoord )
                {
                    if ( semantic == "TEXCOORD0" || semantic == "TEXCOORD1" )
                    {
                        return DataSemantic( static_cast<uint8_t>( i ) );
                    }
                }
            }

            EE_UNREACHABLE_CODE();
            return {};
        }
    }

    static bool GetCBufferDescs( ID3D11ShaderReflection* pShaderReflection, TVector<RenderBuffer>& cbuffers )
    {
        EE_ASSERT( pShaderReflection != nullptr );

        D3D11_SHADER_DESC shaderDesc;
        auto result = pShaderReflection->GetDesc( &shaderDesc );

        if ( FAILED( result ) )
        {
            return false;
        }

        for ( UINT i = 0; i < shaderDesc.ConstantBuffers; i++ )
        {
            ID3D11ShaderReflectionConstantBuffer* pConstBufferDesc = pShaderReflection->GetConstantBufferByIndex( i );
            D3D11_SHADER_BUFFER_DESC desc;
            result = pConstBufferDesc->GetDesc( &desc );
            EE_ASSERT( SUCCEEDED( result ) );

            // Get constant buffer desc
            RenderBuffer buffer;
            buffer.m_ID = Hash::GetHash32( desc.Name );
            buffer.m_byteSize = desc.Size;
            buffer.m_byteStride = 16; // Vector4 aligned
            buffer.m_usage = RenderBuffer::Usage::CPU_and_GPU;
            buffer.m_type = RenderBuffer::Type::Constant;
            buffer.m_slot = i;
            cbuffers.push_back( buffer );
        }

        return true;
    }

    //-------------------------------------------------------------------------

    static bool GetResourceBindingDescs( ID3D11ShaderReflection* pShaderReflection, TVector<TVector<Shader::ResourceBinding>>& resourceBindings )
    {
        EE_ASSERT( pShaderReflection != nullptr );

        D3D11_SHADER_DESC shaderDesc;
        HRESULT result = pShaderReflection->GetDesc( &shaderDesc );

        if ( FAILED( result ) )
        {
            return false;
        }

        // default use only one set
        resourceBindings.resize( 1 );

        for ( UINT i = 0; i < shaderDesc.BoundResources; i++ )
        {
            D3D11_SHADER_INPUT_BIND_DESC desc;
            result = pShaderReflection->GetResourceBindingDesc( i, &desc );
            if ( SUCCEEDED( result ) )
            {
                Shader::ResourceBinding binding = { desc.Name, desc.BindPoint };
                resourceBindings[0].push_back( binding );
            }
            else
            {
                return false;
            }
        }

        return true;
    }

    //-------------------------------------------------------------------------

    //static DataSemantic GetSemanticForName( char const* pName )
    //{
    //    if ( strcmp( pName, "POSITION" ) == 0 )
    //    {
    //        return DataSemantic::Position;
    //    }

    //    if ( strcmp( pName, "NORMAL" ) == 0 )
    //    {
    //        return DataSemantic::Normal;
    //    }

    //    if ( strcmp( pName, "TANGENT" ) == 0 )
    //    {
    //        return DataSemantic::Tangent;
    //    }

    //    if ( strcmp( pName, "BINORMAL" ) == 0 )
    //    {
    //        return DataSemantic::BiTangent;
    //    }

    //    if ( strcmp( pName, "COLOR" ) == 0 )
    //    {
    //        return DataSemantic::Color;
    //    }

    //    if ( strcmp( pName, "TEXCOORD" ) == 0 )
    //    {
    //        return DataSemantic::TexCoord;
    //    }

    //    return DataSemantic::None;
    //}

    static bool GetInputLayoutDesc( ID3D11ShaderReflection* pShaderReflection, VertexLayoutDescriptor& vertexLayoutDesc )
    {
        EE_ASSERT( pShaderReflection != nullptr );

        D3D11_SHADER_DESC shaderDesc;
        pShaderReflection->GetDesc( &shaderDesc );

        // Read input layout description from shader info
        for ( UINT i = 0; i < shaderDesc.InputParameters; i++ )
        {
            D3D11_SIGNATURE_PARAMETER_DESC paramDesc;
            if ( FAILED( pShaderReflection->GetInputParameterDesc( i, &paramDesc ) ) )
            {
                return false;
            }

            VertexLayoutDescriptor::ElementDescriptor elementDesc;
            elementDesc.m_semantic = GetSemanticForName( paramDesc.SemanticName );
            elementDesc.m_semanticIndex = (uint16_t) paramDesc.SemanticIndex;

            // Determine DXGI format
            if ( paramDesc.Mask == 1 )
            {
                if ( paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32 ) elementDesc.m_format = VertexLayoutDescriptor::VertexDataFormat::R32UInt;
                else if ( paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32 ) elementDesc.m_format = VertexLayoutDescriptor::VertexDataFormat::R32SInt;
                else if ( paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32 ) elementDesc.m_format = VertexLayoutDescriptor::VertexDataFormat::R32Float;
            }
            else if ( paramDesc.Mask <= 3 )
            {
                if ( paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32 ) elementDesc.m_format = VertexLayoutDescriptor::VertexDataFormat::RG32UInt;
                else if ( paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32 ) elementDesc.m_format = VertexLayoutDescriptor::VertexDataFormat::RG32SInt;
                else if ( paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32 ) elementDesc.m_format = VertexLayoutDescriptor::VertexDataFormat::RG32Float;
            }
            else if ( paramDesc.Mask <= 7 )
            {
                if ( paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32 ) elementDesc.m_format = VertexLayoutDescriptor::VertexDataFormat::RGB32UInt;
                else if ( paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32 ) elementDesc.m_format = VertexLayoutDescriptor::VertexDataFormat::RGB32SInt;
                else if ( paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32 ) elementDesc.m_format = VertexLayoutDescriptor::VertexDataFormat::RGB32Float;
            }
            else if ( paramDesc.Mask <= 15 )
            {
                if ( paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32 ) elementDesc.m_format = VertexLayoutDescriptor::VertexDataFormat::RGBA32UInt;
                else if ( paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32 ) elementDesc.m_format = VertexLayoutDescriptor::VertexDataFormat::RGBA32SInt;
                else if ( paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32 ) elementDesc.m_format = VertexLayoutDescriptor::VertexDataFormat::RGBA32Float;
            }

            vertexLayoutDesc.m_elementDescriptors.push_back( elementDesc );
        }

        vertexLayoutDesc.CalculateByteSize();
        return true;
    }

    //-------------------------------------------------------------------------

    //static uint32_t SpirvTypeToShaderDataType( const spirv_cross::SPIRType& type )
    //{
    //    if ( type.basetype == spirv_cross::SPIRType::Float )
    //    {
    //        if ( type.columns == 1 )
    //        {
    //            if ( type.vecsize == 1 )
    //                return sizeof( float );
    //            else if ( type.vecsize == 2 )
    //                return sizeof( float ) * 2;
    //            else if ( type.vecsize == 3 )
    //                return sizeof( float ) * 3;
    //            else if ( type.vecsize == 4 )
    //                return sizeof( float ) * 4;
    //        }
    //        else if ( type.columns == 3 && type.vecsize == 3 )
    //            return sizeof( float ) * 3 * 3;
    //        else if ( type.columns == 4 && type.vecsize == 4 )
    //            return sizeof( float ) * 4 * 4;
    //    }
    //    else if ( type.basetype == spirv_cross::SPIRType::Int )
    //    {
    //        if ( type.vecsize == 1 )
    //            return sizeof( int );
    //        else if ( type.vecsize == 2 )
    //            return sizeof( int ) * 2;
    //        else if ( type.vecsize == 3 )
    //            return sizeof( int ) * 3;
    //        else if ( type.vecsize == 4 )
    //            return sizeof( int ) * 4;
    //    }
    //    else if ( type.basetype == spirv_cross::SPIRType::UInt )
    //        if ( type.vecsize == 1 )
    //            return sizeof( int ) * 4;
    //        else if ( type.basetype == spirv_cross::SPIRType::Boolean )
    //            return sizeof( bool );

    //    EE_UNREACHABLE_CODE();
    //    return 0;
    //}

    //static bool GetCBufferDescsSpirv( spirv_cross::Compiler& spirvCompiler, TVector<RenderBuffer>& cbuffers )
    //{
    //    auto shaderResources = spirvCompiler.get_shader_resources();

    //    for ( auto& ubo : shaderResources.uniform_buffers )
    //    {
    //        auto const& type = spirvCompiler.get_type( ubo.type_id );
    //        char const* name = spirvCompiler.get_name( ubo.id ).c_str();
    //        auto const set = spirvCompiler.get_decoration( ubo.id, spv::DecorationDescriptorSet );
    //        auto const binding = spirvCompiler.get_decoration( ubo.id, spv::DecorationBinding );

    //        RenderBuffer buffer;
    //        buffer.m_ID = Hash::GetHash32( name );
    //        buffer.m_byteSize = static_cast<uint32_t>( spirvCompiler.get_declared_struct_size( type ) );
    //        buffer.m_byteStride = 16; // Vector4 aligned
    //        buffer.m_usage = RenderBuffer::Usage::CPU_and_GPU;
    //        buffer.m_type = RenderBuffer::Type::Constant;
    //        buffer.m_slot = binding;
    //        cbuffers.push_back( buffer );
    //    }

    //    return true;
    //}

    static Shader::ReflectedBindingCount GetBindingCount( spirv_cross::SPIRType const& spirvType )
    {
        Shader::ReflectedBindingCount bindingCount;
        if ( spirvType.array.empty() )
        {
            bindingCount.m_type = Shader::EBindingCountType::One;
            bindingCount.m_count = 1;
        }
        else
        {
            size_t const count = static_cast<size_t>( spirvType.array[0] );

            if ( count != 0 )
            {
                bindingCount.m_type = Shader::EBindingCountType::Static;
                bindingCount.m_count = count;
            }
            else
            {
                bindingCount.m_type = Shader::EBindingCountType::Dynamic;
                bindingCount.m_count = std::numeric_limits<size_t>::max();
            }
        }
        return bindingCount;
    }

    static DataSemantic GetVertexLayoutElementDataSemanticFromName( std::string const& name )
    {
        size_t semanticStartPos = name.find_last_of( '.' ) + 1;
        EE_ASSERT( semanticStartPos < name.size() );

        std::string semanticStr = name.substr( semanticStartPos, name.size() - semanticStartPos );
        return GetSemanticForName( semanticStr );
    }

    static VertexLayoutDescriptor::VertexDataFormat GetVertexLayoutElementDataFormat( DataSemantic semantic, spirv_cross::Compiler& spirvCompiler, spirv_cross::SPIRType const& spirvType )
    {
        if ( spirvType.basetype == spirv_cross::SPIRType::UByte )
        {
            if ( spirvType.columns == 1 ) // This is a vector type
            {
                if ( spirvType.vecsize == 1 )
                {
                    return VertexLayoutDescriptor::VertexDataFormat::R8UInt;
                }
                else if ( spirvType.vecsize == 2 )
                {
                    return VertexLayoutDescriptor::VertexDataFormat::RG8UInt;
                }
                else if ( spirvType.vecsize == 4 )
                {
                    return VertexLayoutDescriptor::VertexDataFormat::RGBA8UInt;
                }

                EE_UNIMPLEMENTED_FUNCTION();
            }
        }
        else if ( spirvType.basetype == spirv_cross::SPIRType::UInt )
        {
            if ( spirvType.columns == 1 ) // This is a vector type
            {
                if ( spirvType.vecsize == 1 )
                {
                    return VertexLayoutDescriptor::VertexDataFormat::R32UInt;
                }
                else if ( spirvType.vecsize == 2 )
                {
                    return VertexLayoutDescriptor::VertexDataFormat::RG32UInt;
                }
                else if ( spirvType.vecsize == 3 )
                {
                    return VertexLayoutDescriptor::VertexDataFormat::RGB32UInt;
                }
                else if ( spirvType.vecsize == 4 )
                {
                    return VertexLayoutDescriptor::VertexDataFormat::RGBA32UInt;
                }

                EE_UNIMPLEMENTED_FUNCTION();
            }
        }
        else if ( spirvType.basetype == spirv_cross::SPIRType::Int )
        {
            if ( spirvType.columns == 1 ) // This is a vector type
            {
                if ( spirvType.vecsize == 1 )
                {
                    return VertexLayoutDescriptor::VertexDataFormat::R32SInt;
                }
                else if ( spirvType.vecsize == 2 )
                {
                    return VertexLayoutDescriptor::VertexDataFormat::RG32SInt;
                }
                else if ( spirvType.vecsize == 3 )
                {
                    return VertexLayoutDescriptor::VertexDataFormat::RGB32SInt;
                }
                else if ( spirvType.vecsize == 4 )
                {
                    EE_UNIMPLEMENTED_FUNCTION();
                    return VertexLayoutDescriptor::VertexDataFormat::RGBA32SInt;
                }

                EE_UNIMPLEMENTED_FUNCTION();
            }
        }
        else if ( spirvType.basetype == spirv_cross::SPIRType::Half )
        {
            if ( spirvType.columns == 1 ) // This is a vector type
            {
                if ( spirvType.vecsize == 1 )
                {
                    return VertexLayoutDescriptor::VertexDataFormat::R16Float;
                }
                else if ( spirvType.vecsize == 2 )
                {
                    return VertexLayoutDescriptor::VertexDataFormat::RG16Float;
                }
                else if ( spirvType.vecsize == 4 )
                {
                    return VertexLayoutDescriptor::VertexDataFormat::RGBA16Float;
                }

                EE_UNIMPLEMENTED_FUNCTION();
            }
        }
        else if ( spirvType.basetype == spirv_cross::SPIRType::Float )
        {
            if ( spirvType.columns == 1 ) // This is a vector type
            {
                if ( spirvType.vecsize == 1 )
                {
                    // Note: It is robust? what can we do if we want unorm float?
                    //       We just assume that all these value should be in range [0, 1].
                    if ( semantic == DataSemantic::Normal ||
                         semantic == DataSemantic::Tangent || 
                         semantic == DataSemantic::BiTangent ) 
                    {
                        return VertexLayoutDescriptor::VertexDataFormat::R8Unorm;
                    }

                    return VertexLayoutDescriptor::VertexDataFormat::R32Float;
                }
                else if ( spirvType.vecsize == 2 )
                {
                    // Note: It is robust? what can we do if we want unorm float?
                    //       We just assume that all these value should be in range [0, 1].
                    if ( semantic == DataSemantic::Normal ||
                         semantic == DataSemantic::Tangent ||
                         semantic == DataSemantic::BiTangent )
                    {
                        return VertexLayoutDescriptor::VertexDataFormat::RG8Unorm;
                    }

                    return VertexLayoutDescriptor::VertexDataFormat::RG32Float;
                }
                else if ( spirvType.vecsize == 3 )
                {
                    // Note: It is robust? what can we do if we want unorm float?
                    //       We just assume that all these value should be in range [0, 1].
                    if ( semantic == DataSemantic::Color ||
                         semantic == DataSemantic::Normal ||
                         semantic == DataSemantic::Tangent ||
                         semantic == DataSemantic::BiTangent )
                    {
                        EE_UNREACHABLE_CODE();
                        return VertexLayoutDescriptor::VertexDataFormat::Unknown;
                    }

                    return VertexLayoutDescriptor::VertexDataFormat::RGB32Float;
                }
                else if ( spirvType.vecsize == 4 )
                {
                    // Note: It is robust? what can we do if we want unorm float?
                    //       We just assume that all these value should be in range [0, 1].
                    if ( semantic == DataSemantic::Color )
                    {
                        return VertexLayoutDescriptor::VertexDataFormat::RGBA8Unorm;
                    }

                    return VertexLayoutDescriptor::VertexDataFormat::RGBA32Float;
                }

                EE_UNIMPLEMENTED_FUNCTION();
            }
        }

        EE_UNREACHABLE_CODE();
        return VertexLayoutDescriptor::VertexDataFormat::Unknown;
    }

    static bool ReflectVertexLayoutSpirv( spirv_cross::Compiler& spirvCompiler, VertexLayoutDescriptor& vertexLayoutDesc )
    {
        vertexLayoutDesc.m_elementDescriptors.clear();

        auto shaderResources = spirvCompiler.get_shader_resources();

        for ( auto const& stageInput : shaderResources.stage_inputs )
        {
            //auto const set = spirvCompiler.get_decoration( ubo.id, spv::DecorationDescriptorSet );
            //auto const binding = spirvCompiler.get_decoration( ubo.id, spv::DecorationBinding );
            auto const& spirvType = spirvCompiler.get_type( stageInput.type_id );

            VertexLayoutDescriptor::ElementDescriptor currentElement = {};
            currentElement.m_semantic = GetVertexLayoutElementDataSemanticFromName( stageInput.name );
            currentElement.m_format = GetVertexLayoutElementDataFormat( currentElement.m_semantic, spirvCompiler, spirvType );
            currentElement.m_semanticIndex = 0;
            currentElement.m_offset = 0;

            vertexLayoutDesc.m_elementDescriptors.push_back( currentElement );
        }

        vertexLayoutDesc.CalculateElementOffsets();
        vertexLayoutDesc.CalculateByteSize();

        return true;
    }

    static bool ReflectComputeThreadGroupSize( spirv_cross::Compiler& spirvCompiler, uint32_t threadGroupSize[3] )
    {
        auto executionModel = spirvCompiler.get_execution_model();
        if ( executionModel == spv::ExecutionModelGLCompute )
        {
            threadGroupSize[0] = spirvCompiler.get_execution_mode_argument( spv::ExecutionMode::ExecutionModeLocalSize, 0 );
            threadGroupSize[1] = spirvCompiler.get_execution_mode_argument( spv::ExecutionMode::ExecutionModeLocalSize, 1 );
            threadGroupSize[2] = spirvCompiler.get_execution_mode_argument( spv::ExecutionMode::ExecutionModeLocalSize, 2 );
            //spirvCompiler.get_execution_mode_argument( spv::ExecutionMode::ExecutionModeLocalSizeHint );
        
            return true;
        }

        return false;
    }

    static bool ReflectResourceBindingDescsSpirv( spirv_cross::Compiler& spirvCompiler, TVector<TVector<Shader::ResourceBinding>>& resourceBindings )
     {
        auto shaderResources = spirvCompiler.get_shader_resources();
        
        TSet<String> bindingResourceNames;
        TVector<TPair<uint32_t, Shader::ResourceBinding>> bindings;

        for ( auto& ubo : shaderResources.uniform_buffers )
        {
            char const* name = spirvCompiler.get_name( ubo.id ).c_str();
         
            auto iterator = bindingResourceNames.find( name );
            if ( iterator != bindingResourceNames.end() )
            {
                // Name collision
                return false;
            }

            bindingResourceNames.emplace( name );

            auto const set = spirvCompiler.get_decoration( ubo.id, spv::DecorationDescriptorSet );
            auto const binding = spirvCompiler.get_decoration( ubo.id, spv::DecorationBinding );
            auto const& spirvType = spirvCompiler.get_type( ubo.type_id );

            auto bindingCount = GetBindingCount( spirvType );
            Shader::EReflectedBindingResourceType resourceType = Shader::EReflectedBindingResourceType::UniformBuffer;

            bindings.emplace_back( set, Shader::ResourceBinding{ name, binding, bindingCount, resourceType, "" });
        }

        for ( auto& sbo : shaderResources.storage_buffers )
        {
            char const* name = spirvCompiler.get_name( sbo.id ).c_str();

            auto iterator = bindingResourceNames.find( name );
            if ( iterator != bindingResourceNames.end() )
            {
                // Name collision
                return false;
            }

            bindingResourceNames.emplace( name );

            auto const set = spirvCompiler.get_decoration( sbo.id, spv::DecorationDescriptorSet );
            auto const binding = spirvCompiler.get_decoration( sbo.id, spv::DecorationBinding );
            auto const& spirvType = spirvCompiler.get_type( sbo.type_id );

            auto bindingCount = GetBindingCount( spirvType );
            Shader::EReflectedBindingResourceType resourceType = Shader::EReflectedBindingResourceType::StorageBuffer;

            bindings.emplace_back( set, Shader::ResourceBinding{ name, binding, bindingCount, resourceType, "" } );
        }

        for ( auto& img : shaderResources.sampled_images )
        {
            char const* name = spirvCompiler.get_name( img.id ).c_str();

            auto iterator = bindingResourceNames.find( name );
            if ( iterator != bindingResourceNames.end() )
            {
                // Name collision
                return false;
            }

            bindingResourceNames.emplace( name );

            auto const set = spirvCompiler.get_decoration( img.id, spv::DecorationDescriptorSet );
            auto const binding = spirvCompiler.get_decoration( img.id, spv::DecorationBinding );
            auto const& spirvType = spirvCompiler.get_type( img.type_id );

            auto bindingCount = GetBindingCount( spirvType );
            Shader::EReflectedBindingResourceType resourceType = Shader::EReflectedBindingResourceType::CombinedTextureSampler;

            bindings.emplace_back( set, Shader::ResourceBinding{ name, binding, bindingCount, resourceType, "" } );
        }

        for ( auto& img : shaderResources.separate_images )
        {
            char const* name = spirvCompiler.get_name( img.id ).c_str();

            auto iterator = bindingResourceNames.find( name );
            if ( iterator != bindingResourceNames.end() )
            {
                // Name collision
                return false;
            }

            bindingResourceNames.emplace( name );

            auto const set = spirvCompiler.get_decoration( img.id, spv::DecorationDescriptorSet );
            auto const binding = spirvCompiler.get_decoration( img.id, spv::DecorationBinding );
            auto const& spirvType = spirvCompiler.get_type( img.type_id );

            auto bindingCount = GetBindingCount( spirvType );

            Shader::EReflectedBindingResourceType resourceType;
            if ( spirvType.image.dim == spv::DimBuffer )
            {
                resourceType = Shader::EReflectedBindingResourceType::UniformTexelBuffer;
            }
            else
            {
                resourceType = Shader::EReflectedBindingResourceType::SampleTexture;
            }
            
            bindings.emplace_back( set, Shader::ResourceBinding{ name, binding, bindingCount, resourceType, "" } );
        }

        for ( auto& img : shaderResources.storage_images )
        {
            char const* name = spirvCompiler.get_name( img.id ).c_str();

            auto iterator = bindingResourceNames.find( name );
            if ( iterator != bindingResourceNames.end() )
            {
                // Name collision
                return false;
            }

            bindingResourceNames.emplace( name );

            auto const set = spirvCompiler.get_decoration( img.id, spv::DecorationDescriptorSet );
            auto const binding = spirvCompiler.get_decoration( img.id, spv::DecorationBinding );
            auto const& spirvType = spirvCompiler.get_type( img.type_id );

            Shader::EReflectedBindingResourceType resourceType;
            auto bindingCount = GetBindingCount( spirvType );
            if ( spirvType.image.dim == spv::DimBuffer )
            {
                resourceType = Shader::EReflectedBindingResourceType::StorageTexelBuffer;
            }
            else
            {
                resourceType = Shader::EReflectedBindingResourceType::StorageTexture;
            }

            bindings.emplace_back( set, Shader::ResourceBinding{ name, binding, bindingCount, resourceType, "" } );
        }

        for ( auto& sampler : shaderResources.separate_samplers )
        {
            String const name = spirvCompiler.get_name( sampler.id ).c_str();

            auto iterator = bindingResourceNames.find( name );
            if ( iterator != bindingResourceNames.end() )
            {
                // Name collision
                return false;
            }

            bindingResourceNames.emplace( name );

            auto const set = spirvCompiler.get_decoration( sampler.id, spv::DecorationDescriptorSet );
            auto const binding = spirvCompiler.get_decoration( sampler.id, spv::DecorationBinding );
            auto const& spirvType = spirvCompiler.get_type( sampler.type_id );

            auto bindingCount = GetBindingCount( spirvType );
            Shader::EReflectedBindingResourceType resourceType = Shader::EReflectedBindingResourceType::Sampler;

            size_t const underscorePos = name.find_first_of( '_' ) + 1;
            String const nameSuffix = name.substr( underscorePos, name.size() - underscorePos );
            bindings.emplace_back( set, Shader::ResourceBinding{ name, binding, bindingCount, resourceType, nameSuffix } );
        }

        //for ( auto& acc : shaderResources.acceleration_structures )
        //{
        //    char const* name = spirvCompiler.get_name( acc.id ).c_str();
        //    auto const set = spirvCompiler.get_decoration( acc.id, spv::DecorationDescriptorSet );
        //    auto const binding = spirvCompiler.get_decoration( acc.id, spv::DecorationBinding );

        //    bindings.emplace_back( set, Shader::ResourceBinding{ Hash::GetHash32( name ), binding } );
        //}

        VectorSort( bindings, [] ( auto const& lhs, auto const& rhs )
        {
            return lhs < rhs;
        } );

        int currSetId = -1;
        for ( auto const& binding : bindings )
        {
            if ( static_cast<int>( binding.first ) != currSetId )
            {
                int intervalCount = binding.first - currSetId;
                resourceBindings.resize( resourceBindings.size() + intervalCount );
                currSetId = binding.first;
            }

            resourceBindings[binding.first].emplace_back( binding.second );
        }

        return true;
    }

    static bool ReflectPushConstantsSpirv( spirv_cross::Compiler& spirvCompiler, Shader::PushConstant& pushConstants  )
    {
        auto shaderResources = spirvCompiler.get_shader_resources();

        for ( auto& pc : shaderResources.push_constant_buffers )
        {
            char const* name = spirvCompiler.get_name( pc.id ).c_str();
            auto const& spirvType = spirvCompiler.get_type( pc.type_id );

            pushConstants.m_ID = Hash::GetHash32(name);
            pushConstants.m_size = static_cast<uint32_t>( spirvCompiler.get_declared_struct_size( spirvType ) );
            // TODO: offset?
            pushConstants.m_offset = 0;

            // Note: only take the first one for now.
            break;
        }

        return true;
    }

    //-------------------------------------------------------------------------

    Resource::CompilationResult ShaderCompiler::CompileShader( Resource::CompileContext const& ctx, int32_t compilerVersion ) const
    {
        ShaderResourceDescriptor resourceDescriptor;
        if ( !Resource::ResourceDescriptor::TryReadFromFile( *m_pTypeRegistry, ctx.m_inputFilePath, resourceDescriptor ) )
        {
            return Error( "Failed to read resource descriptor from input file: %s", ctx.m_inputFilePath.c_str() );
        }

        if ( resourceDescriptor.m_shaderBackendLanguage == ShaderBackendLanguage::DX11 )
        {
            return CompileDX11Shader( ctx, resourceDescriptor, compilerVersion );
        }
        else if ( resourceDescriptor.m_shaderBackendLanguage == ShaderBackendLanguage::Vulkan )
        {
            TSharedPtr<Shader> pShader = nullptr;
            Resource::CompilationResult result = CompileVulkanShader( ctx, resourceDescriptor, pShader );
            if ( result == Resource::CompilationResult::Failure )
            {
                return result;
            }

            result = ReflectVulkanShader( ctx, pShader );
            if ( result == Resource::CompilationResult::Failure )
            {
                return result;
            }

            return WriteShaderToFile( ctx, pShader, compilerVersion );
        }
        else
        {
            return Error( "Unknown shader graphic backend." );
        }
    }

    Resource::CompilationResult ShaderCompiler::CompileDX11Shader( Resource::CompileContext const& ctx, ShaderResourceDescriptor const& desc, int32_t compilerVersion ) const
    {
        uint32_t shaderCompileFlags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_WARNINGS_ARE_ERRORS;
        #ifdef EE_DEBUG
        shaderCompileFlags |= D3DCOMPILE_DEBUG;
        #endif

        TSharedPtr<Shader> pShader = nullptr;

        // Set compile options
        //-------------------------------------------------------------------------

        String compileTarget;
        if ( desc.m_shaderType == ShaderType::Vertex )
        {
            compileTarget = "vs_5_0";
            pShader = MakeShared<VertexShader>();
        }
        else if ( desc.m_shaderType == ShaderType::Pixel )
        {
            compileTarget = "ps_5_0";
            pShader = MakeShared<PixelShader>();
        }
        else if ( desc.m_shaderType == ShaderType::Compute )
        {
            compileTarget = "cs_5_0";
            pShader = MakeShared<ComputeShader>();
        }
        else
        {
            return Error( "Unknown shader type" );
        }

        // Load shader file
        //-------------------------------------------------------------------------

        FileSystem::Path shaderFilePath;
        if ( !ConvertResourcePathToFilePath( desc.m_shaderPath, shaderFilePath ) )
        {
            return Error( "Invalid texture data path: %s", desc.m_shaderPath.c_str() );
        }

        Blob fileData;
        if ( !FileSystem::LoadFile( shaderFilePath, fileData ) )
        {
            return Error( "Failed to load specified shader file: %s", shaderFilePath.c_str() );
        }

        // Compile shader file
        //-------------------------------------------------------------------------

        ID3DBlob* pCompiledShaderBlob = nullptr;
        ID3DBlob* pErrorMessagesBlob = nullptr;
        auto result = D3DCompile( fileData.data(), fileData.size(), nullptr, nullptr, nullptr, "main", compileTarget.c_str(), shaderCompileFlags, 0, &pCompiledShaderBlob, &pErrorMessagesBlob );

        if ( FAILED( result ) )
        {
            char const* pErrorMessage = (const char*)pErrorMessagesBlob->GetBufferPointer();
            //printf( "Failed to compiler with: %s", pErrorMessage );
            Error( "Failed to compile specified shader file, error: %u", pErrorMessage );
            pErrorMessagesBlob->Release();
            return Resource::CompilationResult::Failure;
        }

        EE_ASSERT( pErrorMessagesBlob == nullptr );

        // Store compiled shader byte code 
        pShader->m_byteCode.resize( pCompiledShaderBlob->GetBufferSize() );
        memcpy( &pShader->m_byteCode[0], pCompiledShaderBlob->GetBufferPointer(), pCompiledShaderBlob->GetBufferSize() );

        // Reflect shader info
        //-------------------------------------------------------------------------

        ID3D11ShaderReflection* pShaderReflection = nullptr;
        if ( FAILED( D3DReflect( pCompiledShaderBlob->GetBufferPointer(), pCompiledShaderBlob->GetBufferSize(), IID_ID3D11ShaderReflection, (void**)&pShaderReflection ) ) )
        {
            return Error( "Failed to reflect compiled shader file" );
        }

        if ( !GetCBufferDescs( pShaderReflection, pShader->m_cbuffers ) )
        {
            return Error( "Failed to get cbuffer descs" );
        }

        if ( !GetResourceBindingDescs( pShaderReflection, pShader->m_resourceBindings ) )
        {
            return Error( "Failed to get resource binding descs" );
        }

        // If vertex shader
        if ( pShader->GetPipelineStage() == PipelineStage::Vertex )
        {
            auto pVertexShader = reinterpret_cast<VertexShader*>( pShader.get() );
            // Get vertex buffer input element descs
            if ( !GetInputLayoutDesc( pShaderReflection, pVertexShader->m_vertexLayoutDesc ) )
            {
                return Error( "Failed to get input element descs for vertex buffer" );
            }
        }

        // Release reflected and compiled shader data - semantic string names were contained in this data
        pShaderReflection->Release();
        pCompiledShaderBlob->Release();

        // Output shader resource
        //-------------------------------------------------------------------------

        Serialization::BinaryOutputArchive archive;

        if ( pShader->GetPipelineStage() == PipelineStage::Pixel )
        {
            Resource::ResourceHeader hdr( s_version, PixelShader::GetStaticResourceTypeID(), ctx.m_sourceResourceHash );
            archive << hdr << *static_cast<PixelShader*>( pShader.get() );
        }
        if ( pShader->GetPipelineStage() == PipelineStage::Vertex )
        {
            Resource::ResourceHeader hdr( s_version, VertexShader::GetStaticResourceTypeID(), ctx.m_sourceResourceHash );
            archive << hdr << *static_cast<VertexShader*>( pShader.get() );
        }

        if ( archive.WriteToFile( ctx.m_outputFilePath ) )
        {
            return CompilationSucceeded( ctx );
        }
        else
        {
            return CompilationFailed( ctx );
        }
    }

    Resource::CompilationResult ShaderCompiler::CompileVulkanShader( Resource::CompileContext const& ctx, ShaderResourceDescriptor const& desc, TSharedPtr<Shader>& OutShader ) const
    {
        uint32_t shaderCompileFlags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_WARNINGS_ARE_ERRORS;
        #ifdef EE_DEBUG
        shaderCompileFlags |= D3DCOMPILE_DEBUG;
        #endif

        TSharedPtr<Shader> pShader = nullptr;

        // Set compile options
        //-------------------------------------------------------------------------

        DxcShaderTargetProfile profile;
        if ( desc.m_shaderType == ShaderType::Vertex )
        {
            profile = DxcShaderTargetProfile::Vertex;
            pShader = MakeShared<VertexShader>();
        }
        else if ( desc.m_shaderType == ShaderType::Pixel )
        {
            profile = DxcShaderTargetProfile::Pixel;
            pShader = MakeShared<PixelShader>();
        }
        else if ( desc.m_shaderType == ShaderType::Compute )
        {
            profile = DxcShaderTargetProfile::Compute;
            pShader = MakeShared<ComputeShader>();
        }
        else
        {
            return Error( "Unknown shader type" );
        }

        // Load shader file
        //-------------------------------------------------------------------------

        FileSystem::Path shaderFilePath;
        if ( !ConvertResourcePathToFilePath( desc.m_shaderPath, shaderFilePath ) )
        {
            return Error( "Invalid texture data path: %s", desc.m_shaderPath.c_str() );
        }

        // Compile shader file
        //-------------------------------------------------------------------------

        pShader->m_shaderEntryName = desc.m_shaderEntryName;

        DxcShaderCompiler& compiler = DxcShaderCompiler::Get();
        if ( !compiler.Compile( shaderFilePath, pShader->m_byteCode, profile, desc.m_shaderEntryName.c_str() ) )
        {
            String errorMessage = compiler.GetLastErrorMessage();
            Error( "Failed to compile specified shader file with dxc compiler, error: %s", errorMessage.c_str() );
            return Resource::CompilationResult::Failure;
        }

        OutShader = pShader;
        return Resource::CompilationResult::Success;
    }

    Resource::CompilationResult ShaderCompiler::ReflectVulkanShader( Resource::CompileContext const& ctx, TSharedPtr<Shader>& pShader ) const
    {
        EE_ASSERT( !pShader->m_byteCode.empty() );
        spirv_cross::Compiler spirvCompiler( reinterpret_cast<uint32_t const*>( pShader->m_byteCode.data() ), pShader->m_byteCode.size() / sizeof( uint32_t ) );

        if ( pShader->GetPipelineStage() == PipelineStage::Vertex )
        {
            auto* pVertexShader = static_cast<VertexShader*>(pShader.get());

            if ( !ReflectVertexLayoutSpirv( spirvCompiler, pVertexShader->m_vertexLayoutDesc ) )
            {
                return Error( "Failed to reflect vertex shader vertex layout!" );
            }
        }

        if ( pShader->GetPipelineStage() == PipelineStage::Compute )
        {
            auto* pComputeShader = static_cast<ComputeShader*>( pShader.get() );

            if ( !ReflectComputeThreadGroupSize( spirvCompiler, pComputeShader->m_threadGroupSize ) )
            {
                return Error( "Failed to reflect compute shader thread group size!" );
            }
        }

        if ( !ReflectResourceBindingDescsSpirv( spirvCompiler, pShader->m_resourceBindings ) )
        {
            return Error( "Failed to reflect shader resource bindings!" );
        }

        if ( !ReflectPushConstantsSpirv( spirvCompiler, pShader->m_pushConstants ) )
        {
            return Error( "Failed to reflect shader push constants!" );
        }
        
        return Resource::CompilationResult::Success;
    }

    Resource::CompilationResult ShaderCompiler::WriteShaderToFile( Resource::CompileContext const& ctx, TSharedPtr<Shader> const& pShader, int32_t compilerVersion ) const
    {
        Serialization::BinaryOutputArchive archive;

        if ( pShader->GetPipelineStage() == PipelineStage::Vertex )
        {
            Resource::ResourceHeader hdr( compilerVersion, VertexShader::GetStaticResourceTypeID(), ctx.m_sourceResourceHash );
            archive << hdr << *static_cast<VertexShader*>( pShader.get() );
        }
        if ( pShader->GetPipelineStage() == PipelineStage::Pixel )
        {
            Resource::ResourceHeader hdr( compilerVersion, PixelShader::GetStaticResourceTypeID(), ctx.m_sourceResourceHash );
            archive << hdr << *static_cast<PixelShader*>( pShader.get() );
        }
        if ( pShader->GetPipelineStage() == PipelineStage::Compute )
        {
            Resource::ResourceHeader hdr( compilerVersion, ComputeShader::GetStaticResourceTypeID(), ctx.m_sourceResourceHash );
            archive << hdr << *static_cast<ComputeShader*>( pShader.get() );
        }

        if ( archive.WriteToFile( ctx.m_outputFilePath ) )
        {
            return CompilationSucceeded( ctx );
        }
        else
        {
            return CompilationFailed( ctx );
        }

        return Resource::CompilationResult::Success;
    }

    //-------------------------------------------------------------------------

    VertexShaderCompiler::VertexShaderCompiler()
        : ShaderCompiler( "VertexShaderCompiler", ShaderCompiler::s_version )
    {
        m_outputTypes.push_back( VertexShader::GetStaticResourceTypeID() );
    }

    Resource::CompilationResult VertexShaderCompiler::Compile( Resource::CompileContext const& ctx ) const
    {
        return CompileShader( ctx, ShaderCompiler::s_version );
    }

    //-------------------------------------------------------------------------

    PixelShaderCompiler::PixelShaderCompiler()
        : ShaderCompiler( "PixelShaderCompiler", ShaderCompiler::s_version )
    {
        m_outputTypes.push_back( PixelShader::GetStaticResourceTypeID() );
    }

    Resource::CompilationResult PixelShaderCompiler::Compile( Resource::CompileContext const& ctx ) const
    {
        return CompileShader( ctx, ShaderCompiler::s_version );
    }

    //-------------------------------------------------------------------------

    ComputeShaderCompiler::ComputeShaderCompiler()
        : ShaderCompiler( "ComputeShaderCompiler", ShaderCompiler::s_version )
    {
        m_outputTypes.push_back( ComputeShader::GetStaticResourceTypeID() );
    }

    Resource::CompilationResult ComputeShaderCompiler::Compile( Resource::CompileContext const& ctx ) const
    {
        return CompileShader( ctx, ShaderCompiler::s_version );
    }
}