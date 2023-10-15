#pragma once

#include "Base/Systems.h"
#include "Base/Resource/ResourcePtr.h"
#include "Base/Render/RenderShader.h"
#include "Base/Render/RenderPipeline.h"
#include "Base/Render/RenderPipelineState.h"
#include "Base/Types/IDVector.h"
#include "Base/RHI/Resource/RHIResourceCreationCommons.h"

#include <numeric>

//-------------------------------------------------------------------------

namespace EE
{
	class TaskSystem;
	namespace { class ResourceSystem; }
}

namespace EE::RHI
{
    class RHIDevice;
    class RHIPipelineState;
}

//-------------------------------------------------------------------------

namespace EE::Render
{
	class PipelineHandle
	{
		friend class PipelineRegistry;

	public:

        PipelineHandle() = default;

		inline bool IsValid() const
		{
			return m_ID != 0 && m_ID != std::numeric_limits<uint32_t>::max();
		}

		inline uint32_t RawValue() const
		{
			return m_ID;
		}

        inline PipelineType GetPipelineType() const
        {
            return m_type;
        }

	public:

		inline friend bool operator<( PipelineHandle const& lhs, PipelineHandle const& rhs )
		{
			return lhs.m_ID < rhs.m_ID;
		}

		inline friend bool operator==( PipelineHandle const& lhs, PipelineHandle const& rhs )
		{
			return lhs.m_ID == rhs.m_ID;
		}

	private:

		PipelineHandle( PipelineType type, uint32_t id );

	private:

		// this id will be used as ResourceRequestID, so it can not be 0.
		uint32_t				m_ID = std::numeric_limits<uint32_t>::max();
		PipelineType			m_type = PipelineType::Raster;
	};
}

//-------------------------------------------------------------------------

namespace eastl
{
	template <>
	struct hash<EE::Render::PipelineHandle>
	{
		EE_FORCE_INLINE eastl_size_t operator()( EE::Render::PipelineHandle const& handle ) const
		{
			return static_cast<eastl_size_t>( handle.RawValue() );
		}
	};
}

//-------------------------------------------------------------------------

namespace EE::Render
{
	class RasterPipelineEntry
	{
		friend class PipelineRegistry;

	public:

		inline PipelineHandle GetID() const { return m_handle; }
		inline bool IsReadyToCreatePipelineLayout() const { return m_vertexShader.IsLoaded() && m_pixelShader.IsLoaded(); }
        // A pipeline entry is visible means that it is ready to be used outside.
        // If a pipeline is NOT fully ready, it is invisible to outside, as if it doesn't exist.
        // This function should only be called with PipelineRegistry.
        inline bool IsVisible() const { return m_pPipelineState != nullptr; }

	private:

		// TODO: support more shader type
		TResourcePtr<VertexShader>			    m_vertexShader;
		TResourcePtr<PixelShader>			    m_pixelShader;

        RHI::RHIRasterPipelineStateCreateDesc   m_desc;

        RHI::RHIPipelineState*                  m_pPipelineState = nullptr;
        PipelineHandle                          m_handle;
	};

	class ComputePipelineEntry
	{
		friend class PipelineRegistry;

	public:

		inline PipelineHandle GetID() const { return m_handle; }

	private:

		//ComputeShader*					m_pComputeShader = nullptr;

		PipelineHandle					m_handle;
	};

	//-------------------------------------------------------------------------

	class EE_BASE_API PipelineRegistry
	{

	public:

		PipelineRegistry() = default;
		~PipelineRegistry();

		PipelineRegistry( PipelineRegistry const& ) = delete;
		PipelineRegistry& operator=( PipelineRegistry const& ) = delete;

		PipelineRegistry( PipelineRegistry&& ) = delete;
		PipelineRegistry& operator=( PipelineRegistry&& ) = delete;

	public:

		void Initialize( SystemRegistry const& systemRegistry );
		void Shutdown();

		[[nodiscard]] inline PipelineHandle RegisterRasterPipeline( RHI::RHIRasterPipelineStateCreateDesc const& rasterPipelineDesc );
		[[nodiscard]] inline PipelineHandle RegisterComputePipeline( ComputePipelineDesc const& computePipelineDesc );

        bool IsPipelineReady( PipelineHandle const& pipelineHandle ) const;

        RHI::RHIPipelineState* GetPipeline( PipelineHandle const& pipelineHandle ) const;

		void LoadAndUpdatePipelines( RHI::RHIDevice* pDevice );
        void DestroyAllPipelineState( RHI::RHIDevice* pDevice );

	private:

        // Call once to update all the loading status of a shader.
		void UpdateLoadPipelineShaders();

        // Create RHI raster pipeline state for certain RasterPipelineEntry.
        // This function can't have any side-effects, since it may be call for the same entry multiple time.
        bool TryCreateRHIRasterPipelineStateForEntry( TSharedPtr<RasterPipelineEntry>& rasterEntry, RHI::RHIDevice* pDevice );

        // Unload all pipeline shaders inside all registries.
		void UnloadAllPipelineShaders();

	private:

		bool															    m_isInitialized = false;

		TaskSystem*														    m_pTaskSystem = nullptr;
		Resource::ResourceSystem*										    m_pResourceSystem = nullptr;

        // TODO: may be extract to single pipeline cache class
		mutable TIDVector<PipelineHandle, TSharedPtr<RasterPipelineEntry>>  m_rasterPipelineStatesCache;
		THashMap<RHI::RHIRasterPipelineStateCreateDesc, PipelineHandle>     m_rasterPipelineHandlesCache;

        TIDVector<PipelineHandle, ComputePipelineEntry>					    m_computePipelineStates;
        //THashMap<ComputePipelineDesc, PipelineHandle>					    m_computePipelineHandles;

        // TODO: use state update pattern
        TVector<TSharedPtr<RasterPipelineEntry>>						    m_waitToSubmitRasterPipelines;
        TVector<TSharedPtr<RasterPipelineEntry>>						    m_waitToLoadRasterPipelines;
        TVector<TSharedPtr<RasterPipelineEntry>>						    m_waitToRegisteredRasterPipelines;
        TVector<TSharedPtr<RasterPipelineEntry>>                            m_retryRasterPipelineCaches;
	};
}