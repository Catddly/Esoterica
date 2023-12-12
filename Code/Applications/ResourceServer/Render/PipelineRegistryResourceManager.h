#pragma once

#include "Base/Types/HashMap.h"
#include "Base/Resource/ResourceID.h"
#include "Base/Resource/ResourcePtr.h"

namespace EE::Resource
{
    class ResourceRecord;

    class PipelineRegistryResourceManager
    {
    public:

        // Request a load of a resource, can optionally provide a ResourceRequesterID for identification of the request source
        void LoadResource( ResourcePtr& resourcePtr, ResourceRequesterID const& requesterID = ResourceRequesterID() );

        // Request an unload of a resource, can optionally provide a ResourceRequesterID for identification of the request source
        void UnloadResource( ResourcePtr& resourcePtr, ResourceRequesterID const& requesterID = ResourceRequesterID() );

        template<typename T>
        inline void LoadResource( TResourcePtr<T>& resourcePtr, ResourceRequesterID const& requesterID = ResourceRequesterID() ) { LoadResource( (ResourcePtr&) resourcePtr, requesterID ); }

        template<typename T>
        inline void UnloadResource( TResourcePtr<T>& resourcePtr, ResourceRequesterID const& requesterID = ResourceRequesterID() ) { UnloadResource( (ResourcePtr&) resourcePtr, requesterID ); }

    private:

        ResourceRecord* FindOrCreateResourceRecord( ResourceID const& resourceID );
        ResourceRecord* FindExistingResourceRecord( ResourceID const& resourceID );

    private:

        THashMap<ResourceID, ResourceRecord*>           m_resourceRecords;
    };
}