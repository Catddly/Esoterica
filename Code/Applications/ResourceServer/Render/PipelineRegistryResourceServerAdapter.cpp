#include "PipelineRegistryResourceServerAdapter.h"
#include "../ResourceServer.h"
#include "Base/Resource/ResourceRequesterID.h"

namespace EE::Render
{
    bool PipelineRegistryResourceServerAdapter::Initialize()
    {
        return PipelineRegistry::Initialize( nullptr );
    }

    void PipelineRegistryResourceServerAdapter::Shutdown()
    {
        PipelineRegistry::Shutdown();
    }

    void PipelineRegistryResourceServerAdapter::UpdateLoadPipelineShaders()
    {
        //if ( m_pResourceServer )
        //{
        //    if ( !m_waitToSubmitRasterPipelines.empty() )
        //    {
        //        for ( uint32_t i = 0; i < m_waitToSubmitRasterPipelines.size(); ++i )
        //        {
        //            auto const& pEntry = m_waitToSubmitRasterPipelines[i];

        //            if ( pEntry->m_vertexShader.IsSet() )
        //            {
        //                Resource::ResourcePtr& resourecPtr = pEntry->m_vertexShader;
        //                m_pResourceServer->CompileResource( resourecPtr.GetResourceID(), false );
        //            }

        //            if ( pEntry->m_pixelShader.IsSet() )
        //            {
        //                Resource::ResourcePtr& resourecPtr = pEntry->m_pixelShader;
        //                m_pResourceServer->CompileResource( resourecPtr.GetResourceID(), false );
        //            }

        //            MarkRasterPipelineEntryLoading( pEntry );
        //        }

        //        m_waitToSubmitRasterPipelines.clear();
        //    }
        //}
    }
}
