#include "LocalResourceProvider.h"
#include "ResourceServer.h"
#include "Base/Profiling.h"
#include "Base/Resource/ResourceRequest.h"

#if EE_DEVELOPMENT_TOOLS
namespace EE::Resource
{
    LocalResourceProvider::LocalResourceProvider( ResourceServer& resourceServer )
        : m_resourceServer( resourceServer ), ResourceProvider( *resourceServer.m_pSettings )
    {
    }

    //-------------------------------------------------------------------------

    bool LocalResourceProvider::IsReady() const
    {
        return true;
    }

    //-------------------------------------------------------------------------

    bool LocalResourceProvider::Initialize()
    {
        return true;
    }

    void LocalResourceProvider::Shutdown()
    {

    }

    void LocalResourceProvider::Update()
    {
        EE_PROFILE_FUNCTION_RESOURCE();

        if ( !m_pendingRequests.empty() )
        {
            // Process all pending requests
            for ( uint32_t i = 0; i < m_pendingRequests.size(); ++i )
            {
                auto* pRequest = m_pendingRequests[i];
                m_resourceServer.CompileResource( pRequest->GetResourceID() );

                m_sentRequests.emplace_back( pRequest );
            }
            m_pendingRequests.clear();
        }

        //-------------------------------------------------------------------------

        for ( ResourceRequest* pRequest : m_sentRequests )
        {
            if ( pRequest->GetLoadingStatus() != LoadingStatus::Loading )
            {
                continue;
            }

            auto& compilationRequests = m_resourceServer.GetRequests();
            
            auto predicate = [] ( CompilationRequest const* pRequest, ResourceID const& resourceID ) { return pRequest->GetResourceID() == resourceID; };
            auto iterator = VectorFind( compilationRequests, pRequest->GetResourceID(), predicate );
            EE_ASSERT( iterator != compilationRequests.end() );
         
            auto& foundCompilationRequest = *iterator;
            if ( foundCompilationRequest->IsComplete() )
            {
                if ( foundCompilationRequest->HasSucceeded() )
                {
                    pRequest->OnRawResourceRequestComplete( foundCompilationRequest->GetDestinationFilePath().ToString(), foundCompilationRequest->GetLog() );
                }
                else // failed
                {
                    pRequest->OnRawResourceRequestComplete( "", foundCompilationRequest->GetLog() );
                }
            }
            else
            {
                m_waitToCompleteRequests.emplace_back( pRequest );
            }
        }

        m_sentRequests.swap( m_waitToCompleteRequests );
        m_waitToCompleteRequests.clear();

        //#if EE_DEVELOPMENT_TOOLS
        //m_externallyUpdatedResources.clear();
        //#endif

        //// Check connection to resource server
        ////-------------------------------------------------------------------------

        //if ( m_networkClient.IsConnecting() )
        //{
        //    return;
        //}

        //if ( m_networkClient.HasConnectionFailed() )
        //{
        //    if ( !m_networkFailureDetected )
        //    {
        //        EE_LOG_FATAL_ERROR( "Resource", "Network Resource Provider", "Lost connection to resource server" );
        //        m_networkFailureDetected = true;
        //    }
        //    return;
        //}

        //// Check for any network messages
        ////-------------------------------------------------------------------------

        //auto ProcessMessageFunction = [this] ( Network::IPC::Message const& message )
        //{
        //    switch ( (NetworkMessageID) message.GetMessageID() )
        //    {
        //        case NetworkMessageID::ResourceRequestComplete:
        //        {
        //            NetworkResourceResponse response = message.GetData<NetworkResourceResponse>();
        //            m_serverResults.insert( m_serverResults.end(), response.m_results.begin(), response.m_results.end() );
        //        }
        //        break;

        //        case NetworkMessageID::ResourceUpdated:
        //        {
        //            #if EE_DEVELOPMENT_TOOLS
        //            NetworkResourceResponse response = message.GetData<NetworkResourceResponse>();
        //            for ( auto const& result : response.m_results )
        //            {
        //                m_externallyUpdatedResources.emplace_back( result.m_resourceID );
        //            }
        //            #endif
        //        }
        //        break;

        //        default:
        //        break;
        //    };
        //};

        //m_networkClient.ProcessIncomingMessages( ProcessMessageFunction );

        //// Send all requests and keep-alive messages
        ////-------------------------------------------------------------------------

        //if ( !m_pendingRequests.empty() )
        //{
        //    NetworkResourceRequest request;

        //    // Process all pending requests
        //    for ( auto pRequest : m_pendingRequests )
        //    {
        //        request.m_resourceIDs.emplace_back( pRequest->GetResourceID() );
        //        m_sentRequests.emplace_back( pRequest );

        //        // Try to limit the size of the network messages so we limit each message to 128 request
        //        if ( request.m_resourceIDs.size() == 128 )
        //        {
        //            Network::IPC::Message requestResourceMessage( (int32_t) NetworkMessageID::RequestResource, request );
        //            m_networkClient.SendMessageToServer( eastl::move( requestResourceMessage ) );
        //            request.m_resourceIDs.clear();
        //        }
        //    }
        //    m_pendingRequests.clear();

        //    // Send any remaining requests
        //    if ( request.m_resourceIDs.size() > 0 )
        //    {
        //        Network::IPC::Message requestResourceMessage( (int32_t) NetworkMessageID::RequestResource, request );
        //        m_networkClient.SendMessageToServer( eastl::move( requestResourceMessage ) );
        //    }
        //}

        //// Process all server results
        ////-------------------------------------------------------------------------

        //for ( auto& result : m_serverResults )
        //{
        //    auto predicate = [] ( ResourceRequest* pRequest, ResourceID const& resourceID ) { return pRequest->GetResourceID() == resourceID; };
        //    auto foundIter = VectorFind( m_sentRequests, result.m_resourceID, predicate );

        //    // This might have been a canceled request
        //    if ( foundIter == m_sentRequests.end() )
        //    {
        //        continue;
        //    }

        //    ResourceRequest* pFoundRequest = static_cast<ResourceRequest*>( *foundIter );
        //    EE_ASSERT( pFoundRequest->IsValid() );

        //    // Ignore any responses for requests that may have been canceled or are unloading
        //    if ( pFoundRequest->GetLoadingStatus() != LoadingStatus::Loading )
        //    {
        //        continue;
        //    }

        //    // If the request has a filepath set, the compilation was a success
        //    pFoundRequest->OnRawResourceRequestComplete( result.m_filePath );

        //    // Remove from request list
        //    m_sentRequests.erase_unsorted( foundIter );
        //}

        //m_serverResults.clear();
    }

    void LocalResourceProvider::RequestRawResource( ResourceRequest* pRequest )
    {
        EE_ASSERT( pRequest != nullptr && pRequest->IsValid() && pRequest->GetLoadingStatus() == LoadingStatus::Loading );

        #if EE_DEVELOPMENT_TOOLS
        auto predicate = [] ( ResourceRequest* pRequest, ResourceID const& resourceID ) { return pRequest->GetResourceID() == resourceID; };
        auto foundIter = VectorFind( m_sentRequests, pRequest->GetResourceID(), predicate );
        EE_ASSERT( foundIter == m_sentRequests.end() );
        #endif

        //-------------------------------------------------------------------------

        m_pendingRequests.emplace_back( pRequest );
    }

    void LocalResourceProvider::CancelRequest( ResourceRequest* pRequest )
    {
        EE_ASSERT( pRequest != nullptr && pRequest->IsValid() );

        auto foundIter = VectorFind( m_sentRequests, pRequest );
        EE_ASSERT( foundIter != m_sentRequests.end() );

        //-------------------------------------------------------------------------

        m_sentRequests.erase_unsorted( foundIter );
    }

    TVector<ResourceID> const& LocalResourceProvider::GetExternallyUpdatedResources() const
    {
        static TVector<ResourceID> emptyVector;
        return emptyVector;
    }


}
#endif