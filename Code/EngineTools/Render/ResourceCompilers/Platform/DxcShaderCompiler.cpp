#include "DxcShaderCompiler.h"
#include "Base/Logging/Log.h"
#include "Base/Types/String.h"
#include "Base/Types/WString.h"

#include <spirv_cross/spirv_cross.hpp>

namespace EE::Render
{
	DxcShaderCompiler::DxcShaderCompiler()
	{
        EE_ASSERT( !m_pLibrary && !m_pCompiler && !m_pUtils && !m_pIncludeHandler );

        HRESULT hres { S_OK };

        hres = DxcCreateInstance( CLSID_DxcLibrary, IID_PPV_ARGS( &m_pLibrary ) );
        if ( FAILED( hres ) )
        {
            EE_LOG_ERROR( "Render", "DxcShaderCompiler", "Failed to create dxc instance!" );
            EE_HALT();
            return;
        }
        
        hres = DxcCreateInstance( CLSID_DxcCompiler, IID_PPV_ARGS( &m_pCompiler ) );
        if ( FAILED( hres ) )
        {
            EE_LOG_ERROR( "Render", "DxcShaderCompiler", "Failed to create dxc compiler!" );
            EE_HALT();
            return;
        }

        hres = DxcCreateInstance( CLSID_DxcUtils, IID_PPV_ARGS( &m_pUtils ) );
        if ( FAILED( hres ) )
        {
            EE_LOG_ERROR( "Render", "DxcShaderCompiler", "Failed to create dxc utils!" );
            EE_HALT();
            return;
        }

        hres = m_pUtils->CreateDefaultIncludeHandler( &m_pIncludeHandler );
        if ( FAILED( hres ) )
        {
            EE_LOG_ERROR( "Render", "DxcShaderCompiler", "Failed to create default include handler from dxc utils!" );
            EE_HALT();
            return;
        }

        EE_ASSERT( m_pLibrary && m_pCompiler && m_pUtils && m_pIncludeHandler );
	}

	DxcShaderCompiler::~DxcShaderCompiler()
	{
        // TODO: need to destroy dxc instance?
	}

    //-------------------------------------------------------------------------

    bool DxcShaderCompiler::Compile( FileSystem::Path const& shaderSourcePath, Blob& outBlob, DxcShaderTargetProfile profile, char const* entryName, TSpan<DxcShaderMacroDefine> defines, DxcCompileTarget target )
    {
        EE_ASSERT( m_pCompiler && m_pUtils && m_pIncludeHandler );
        EE_ASSERT( shaderSourcePath.Exists() );
        String extension = shaderSourcePath.GetExtension();
        EE_ASSERT( extension == "hlsl" || extension == "glsl" );

        WString wShaderSourcePath;
        StringUtils::StringToWString( shaderSourcePath.c_str(), wShaderSourcePath );

        WString wShaderEntryName;
        StringUtils::StringToWString( entryName, wShaderEntryName );

        HRESULT hres;

        // Load the hlsl text shader from disk
        //-------------------------------------------------------------------------
        
        uint32_t codePage = DXC_CP_ACP; // code page auto-detection
        CComPtr<IDxcBlobEncoding> sourceBlob;
        hres = m_pUtils->LoadFile( wShaderSourcePath.c_str(), &codePage, &sourceBlob);
        if ( FAILED( hres ) )
        {
            m_errorMessage.sprintf( "Failed to load shader source: %s", shaderSourcePath.c_str() );
            return false;
        }

        // Select target profile based on shader file extension
        //-------------------------------------------------------------------------

        LPCWSTR targetProfile = {};
        switch ( profile )
        {
            case EE::Render::DxcShaderTargetProfile::Vertex:
            targetProfile = L"vs_6_4";
            break;
            case EE::Render::DxcShaderTargetProfile::Pixel:
            targetProfile = L"ps_6_4";
            break;
            case EE::Render::DxcShaderTargetProfile::Compute:
            targetProfile = L"cs_6_4";
            break;
            default:
            EE_UNREACHABLE_CODE();
            break;
        }

        // Compiler arguments
        //-------------------------------------------------------------------------

        TVector<LPCWSTR> arguments = {
            // (Optional) name of the shader file to be displayed e.g. in an error message
            wShaderSourcePath.c_str(),
            DXC_ARG_ENABLE_STRICTNESS,
            DXC_ARG_WARNINGS_ARE_ERRORS,
            // TODO: custom entry point
            L"-E", wShaderEntryName.c_str(),
            L"-T", targetProfile,

            #if EE_DEBUG
            DXC_ARG_DEBUG,
            DXC_ARG_SKIP_OPTIMIZATIONS,
            #else
            DXC_ARG_OPTIMIZATION_LEVEL3,
            #endif

            //L"Qstrip_reflect", // strip reflection into separate blob
            //L"Qstrip_debug",   // strip debug infos into separate blob
        };

        //DxcDefine

        if ( target == DxcCompileTarget::Spirv )
        {
            arguments.push_back( L"-spirv" );
            arguments.push_back( L"-fspv-target-env=vulkan1.3" );
        }

        // Compile shader
        //-------------------------------------------------------------------------

        //TVector<TVector<wchar_t>> wideDefines;
        //TVector<TVector<wchar_t>> wideValues;

        //TVector<DxcDefine> wideDxcDefines;

        //ProcessShaderMacroDefines( defines, wideDefines, wideValues, wideDxcDefines );

        DxcBuffer buffer = {};
        buffer.Encoding = DXC_CP_ACP;
        buffer.Ptr = sourceBlob->GetBufferPointer();
        buffer.Size = sourceBlob->GetBufferSize();

        CComPtr<IDxcResult> result = nullptr;
        hres = m_pCompiler->Compile (
            &buffer,
            arguments.data(),
            static_cast<uint32_t>( arguments.size() ),
            m_pIncludeHandler,
            IID_PPV_ARGS( &result )
        );

        //m_pCompiler->Compile(
        //    pBlobEncoding,
        //    wShaderSourceName.data(), wShaderEntryName.data(), targetProfile,
        //    arguments.data(), static_cast<uint32_t>( arguments.size() ),
        //    wideDxcDefines.data(), static_cast<uint32_t>( wideDxcDefines.size() ),
        //    m_pIncludeHandler,
        //    result._PtrClass
        //);

        if ( FAILED( hres ) )
        {
            m_errorMessage.sprintf( "Failed to start compilation task compiling shader (%s)!", shaderSourcePath.c_str() );
            return false;
        }

        // Get compilation error outputs (if any)
        //-------------------------------------------------------------------------

        CComPtr<IDxcBlobUtf8> errorBlob = nullptr;
        hres = result->GetOutput( DXC_OUT_ERRORS, IID_PPV_ARGS( &errorBlob ), nullptr );

        if ( FAILED( hres ) )
        {
            m_errorMessage.sprintf( "Failed to fetch compile error outputs compiling shader (%s)!", shaderSourcePath.c_str() );
            return false;
        }

        if ( SUCCEEDED( hres ) && errorBlob && errorBlob->GetBufferSize() != 0 )
        {
            m_errorMessage.sprintf( "Failed to compile shader (%s) with error:\t%s", shaderSourcePath.c_str(), errorBlob->GetStringPointer() );
            return false;
        }

        // Get other error outputs (if any)
        //-------------------------------------------------------------------------

        HRESULT statusHres = S_OK;
        statusHres = result->GetStatus( &hres );

        if ( FAILED( statusHres ) )
        {
            m_errorMessage.sprintf( "Failed to fetch compile result status compiling shader (%s)!", shaderSourcePath.c_str() );
            return false;
        }

        if ( FAILED( hres ) && ( result ) )
        {
            CComPtr<IDxcBlobEncoding> errorBlob = nullptr;
            hres = result->GetErrorBuffer( &errorBlob );
            if ( SUCCEEDED( hres ) && errorBlob && errorBlob->GetBufferSize() != 0 )
            {
                m_errorMessage.sprintf( "Failed to compile shader source with: %s", reinterpret_cast<char const*>( errorBlob->GetBufferPointer() ) );
                return false;
            }
        }

        // Get compilation result
        //-------------------------------------------------------------------------

        CComPtr<IDxcBlob> code = nullptr;
        result->GetResult( &code );

        outBlob.resize( code->GetBufferSize() );
        memcpy( outBlob.data(), code->GetBufferPointer(), code->GetBufferSize() );

        return true;
    }

    //void DxcShaderCompiler::ProcessShaderMacroDefines( TSpan<DxcShaderMacroDefine> defines, TVector<TVector<wchar_t>>& outWDefines, TVector<TVector<wchar_t>>& outWValues, TVector<DxcDefine>& outDxcDefine )
    //{
    //    for ( DxcShaderMacroDefine const& define : defines )
    //    {
    //        if ( define.m_value.empty() )
    //        {
    //            WString outWString;
    //            EE_ASSERT( StringUtils::StringToWString( define.m_macroName, outWString ) );
    //            outWDefines.emplace_back( eastl::move( outWString ) );
    //            outWValues.emplace_back( L"1" );

    //            DxcDefine dxcDefine;
    //            dxcDefine.Name = outWDefines.back().data();
    //            dxcDefine.Value = outWValues.back().data();
    //            outDxcDefine.push_back( dxcDefine );
    //        }
    //        else
    //        {
    //            WString outWString;
    //            EE_ASSERT( StringUtils::StringToWString( define.m_macroName, outWString ) );
    //            WString outWValue;
    //            EE_ASSERT( StringUtils::StringToWString( define.m_value, outWValue ) );

    //            outWDefines.emplace_back( eastl::move( outWString ) );
    //            outWValues.emplace_back( eastl::move( outWValue ) );

    //            DxcDefine dxcDefine;
    //            dxcDefine.Name = outWDefines.back().data();
    //            dxcDefine.Value = outWValues.back().data();
    //            outDxcDefine.push_back( dxcDefine );
    //        }
    //    }
    //}

    //-------------------------------------------------------------------------

    DxcShaderCompiler& DxcShaderCompiler::Get()
    {
        static DxcShaderCompiler sDxcCompiler;
        return sDxcCompiler;
    }
}