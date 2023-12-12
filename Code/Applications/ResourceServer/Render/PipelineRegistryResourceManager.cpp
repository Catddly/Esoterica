#include "PipelineRegistryResourceManager.h"

#include "Base/Threading/Threading.h"

namespace EE::Resource
{
    void PipelineRegistryResourceManager::LoadResource( ResourcePtr& resourcePtr, ResourceRequesterID const& requesterID )
    {
        //EE_ASSERT( Threading::IsMainThread() );

        //// Immediately update the resource ptr
        //auto pRecord = FindOrCreateResourceRecord( resourcePtr.GetResourceID() );
        //resourcePtr.m_pResourceRecord = pRecord;

        ////-------------------------------------------------------------------------

        //if ( !pRecord->HasReferences() )
        //{
        //    AddPendingRequest( PendingRequest( PendingRequest::Type::Load, pRecord, requesterID ) );
        //}

        //pRecord->AddReference( requesterID );
    }

    void PipelineRegistryResourceManager::UnloadResource( ResourcePtr& resourcePtr, ResourceRequesterID const& requesterID )
    {

    }

    //-------------------------------------------------------------------------

    ResourceRecord* PipelineRegistryResourceManager::FindOrCreateResourceRecord( ResourceID const& resourceID )
    {
        EE_ASSERT( Threading::IsMainThread() );
        EE_ASSERT( resourceID.IsValid() );
        //Threading::RecursiveScopeLock lock( m_accessLock );

        Resource::ResourceRecord* pRecord = nullptr;
        auto const recordIter = m_resourceRecords.find( resourceID );
        if ( recordIter == m_resourceRecords.end() )
        {
            pRecord = EE::New<Resource::ResourceRecord>( resourceID );
            m_resourceRecords[resourceID] = pRecord;
        }
        else
        {
            pRecord = recordIter->second;
        }

        return pRecord;
    }

    ResourceRecord* PipelineRegistryResourceManager::FindExistingResourceRecord( ResourceID const& resourceID )
    {
        EE_ASSERT( Threading::IsMainThread() );
        EE_ASSERT( resourceID.IsValid() );
        //Threading::RecursiveScopeLock lock( m_accessLock );

        auto const recordIter = m_resourceRecords.find( resourceID );
        EE_ASSERT( recordIter != m_resourceRecords.end() );
        return recordIter->second;
    }
}