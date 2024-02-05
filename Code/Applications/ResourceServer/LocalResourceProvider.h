#pragma once

#include "Base/Esoterica.h"
#include "Base/Resource/ResourceProvider.h"

#if EE_DEVELOPMENT_TOOLS
namespace EE::Resource
{
    class ResourceServer;
    class ResourceSystem;
    class ResourceRequest;

    // Resource provider to pass resource requests directly by resource server itself.
    class LocalResourceProvider final : public ResourceProvider 
    {
    public:

        LocalResourceProvider( ResourceServer& resourceServer );

        virtual bool IsReady() const override;

    private:

        virtual bool Initialize() override;
        virtual void Shutdown() override;
        virtual void Update() override;

        virtual void RequestRawResource( ResourceRequest* pRequest ) override;
        virtual void CancelRequest( ResourceRequest* pRequest ) override;

        TVector<ResourceID> const& GetExternallyUpdatedResources() const override;

    private:

        ResourceServer&                         m_resourceServer;

        TVector<ResourceRequest*>               m_pendingRequests; // Requests we need to sent
        TVector<ResourceRequest*>               m_sentRequests; // Request that were sent but we're still waiting for a response
        TVector<ResourceRequest*>               m_waitToCompleteRequests; // Request that were sent but we're still waiting for a response
    };
}
#endif