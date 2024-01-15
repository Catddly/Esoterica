#include "ResourceLoader_RenderShader.h"
#include "Base/Serialization/BinarySerialization.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    bool ShaderLoader::LoadInternal( ResourceID const& resID, Resource::ResourceRecord* pResourceRecord, Serialization::BinaryInputArchive& archive ) const
    {
        EE_ASSERT( m_pRenderDevice != nullptr );

        // Get shader resource
        Shader* pShaderResource = nullptr;
        auto const shaderResourceTypeID = resID.GetResourceTypeID();

        if ( shaderResourceTypeID == VertexShader::GetStaticResourceTypeID() )
        {
            auto pVertexShaderResource = EE::New<VertexShader>();
            archive << *pVertexShaderResource;
            pShaderResource = pVertexShaderResource;
        }
        else if ( shaderResourceTypeID == PixelShader::GetStaticResourceTypeID() )
        {
            auto pPixelShaderResource = EE::New<PixelShader>();
            archive << *pPixelShaderResource;
            pShaderResource = pPixelShaderResource;
        }
        else if ( shaderResourceTypeID == ComputeShader::GetStaticResourceTypeID() )
        {
            auto pComputeShaderResource = EE::New<ComputeShader>();
            archive << *pComputeShaderResource;
            pShaderResource = pComputeShaderResource;
        }
        else
        {
            return false;
        }

        EE_ASSERT( pShaderResource != nullptr );

        // Create shader
        m_pRenderDevice->LockDevice();
        m_pRenderDevice->CreateVkShader( *pShaderResource );
        m_pRenderDevice->UnlockDevice();
        pResourceRecord->SetResourceData( pShaderResource );
        return true;
    }

    void ShaderLoader::UnloadInternal( ResourceID const& resID, Resource::ResourceRecord* pResourceRecord ) const
    {
        EE_ASSERT( m_pRenderDevice != nullptr );

        auto pShaderResource = pResourceRecord->GetResourceData<Shader>();
        if ( pShaderResource != nullptr )
        {
            m_pRenderDevice->LockDevice();
            m_pRenderDevice->DestroyVkShader( *pShaderResource );
            m_pRenderDevice->UnlockDevice();
        }

        ResourceLoader::UnloadInternal( resID, pResourceRecord );
    }
}