#pragma once

#include "Base/FileSystem/FileSystemPath.h"
#include "Base/Types/Arrays.h"
#include "Base/Types/Function.h"
//#include "Base/Types/WString.h"

#include <atlbase.h> // ComPtr
#include <dxc/dxcapi.h>

namespace EE::Render
{
	enum class DxcCompileTarget
	{
		Spirv
	};

	enum class DxcShaderTargetProfile
	{
		Vertex,
		Pixel,
		Compute,
	};

    struct DxcShaderMacroDefine
    {
        String              m_macroName;
        String              m_value;
    };

	class DxcShaderCompiler
	{
	public:

		DxcShaderCompiler();
		~DxcShaderCompiler();

		DxcShaderCompiler( DxcShaderCompiler const& ) = delete;
		DxcShaderCompiler& operator=( DxcShaderCompiler const& ) = delete;

		DxcShaderCompiler( DxcShaderCompiler&& ) = delete;
		DxcShaderCompiler& operator=( DxcShaderCompiler&& ) = delete;

		static DxcShaderCompiler& Get();

	public:

        bool Compile( FileSystem::Path const& shaderSourcePath, Blob& outBlob, DxcShaderTargetProfile profile, char const* entryName = "main", TSpan<DxcShaderMacroDefine> defines = {}, DxcCompileTarget target = DxcCompileTarget::Spirv );

		inline String GetLastErrorMessage() const { return m_errorMessage; }

    private:

        //void ProcessShaderMacroDefines( TSpan<DxcShaderMacroDefine> defines, TVector<TVector<wchar_t>>& outWDefines, TVector<TVector<wchar_t>>& outWValues, TVector<DxcDefine>& outDxcDefine );

	private:

		CComPtr<IDxcLibrary>					m_pLibrary = nullptr;
		CComPtr<IDxcCompiler3>					m_pCompiler = nullptr;
		CComPtr<IDxcUtils>						m_pUtils = nullptr;
		CComPtr<IDxcIncludeHandler>				m_pIncludeHandler = nullptr;
	
		String									m_errorMessage;
	};
}