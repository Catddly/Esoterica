#pragma once

#include "Base/Types/Atomic.h"

#include <type_traits>
#include <limits>

//-------------------------------------------------------------------------
// Reference Counting Object
//-------------------------------------------------------------------------
// Simple implementation of reference count object.
//

#define EE_REF_COUNT_POINTER_DO_CHECK 1

namespace EE
{
	template <bool THREAD_SAFE>
    class RefCountObject;

    // RefCountObject -- Non-thread safe
    //-------------------------------------------------------------------------

    template <>
    class RefCountObject<false>
    {
    public:

        RefCountObject() = default;
        RefCountObject( uint32_t count )
            : m_count( count )
        {}

    public:

        uint32_t IncreaseRef()
        {
            #if EE_REF_COUNT_POINTER_DO_CHECK
            EE_ASSERT( m_count <= std::numeric_limits<uint32_t>::max() - 1 );
            #endif

            return ++m_count;
        }

        uint32_t DecreaseRef()
        {
            #if EE_REF_COUNT_POINTER_DO_CHECK
            EE_ASSERT( m_count > 0 );
            #endif

            return --m_count;
        }

    private:

        uint32_t						m_count = 0;
    };

    // RefCountObject -- Thread safe
    //-------------------------------------------------------------------------

    template <>
    class RefCountObject<true>
    {
    public:

        RefCountObject() = default;
        RefCountObject( uint32_t count )
        {
            m_count.store( count );
        }

    protected:

        template <typename T>
        friend class RefPtr;

        uint32_t IncreaseRef()
        {
            #if EE_REF_COUNT_POINTER_DO_CHECK
            EE_ASSERT( m_count.load( eastl::memory_order_relaxed ) <= std::numeric_limits<uint32_t>::max() - 1 );
            #endif

            return m_count.add_fetch( 1, eastl::memory_order_seq_cst );
        }

        uint32_t DecreaseRef()
        {
            #if EE_REF_COUNT_POINTER_DO_CHECK
            EE_ASSERT( m_count.load( eastl::memory_order_relaxed ) > 0 );
            #endif

            return m_count.sub_fetch( 1, eastl::memory_order_seq_cst );
        }

    private:

        AtomicU32					m_count = 0;
    };

    //-------------------------------------------------------------------------

    typedef RefCountObject<false> RcObject;
    typedef RefCountObject<true>  ThreadSafeRcObject;

	template <typename T>
	class RefPtr
	{
		EE_STATIC_ASSERT( ( eastl::disjunction_v<
							eastl::is_base_of< RcObject, T >,
							eastl::is_base_of< ThreadSafeRcObject, T >
		> ), "Invalid RefPtr object!" );

        template <typename T>
        friend class RefPtr;

	public:

		template <typename Ret = T, typename... Args>
		inline static RefPtr<T> New( Args&&... args );

	public:

		RefPtr() = default;
		RefPtr( std::nullptr_t ) {};

		RefPtr( T* ptr )
			: m_pData( ptr )
		{
			this->IncreaseRef();
		}

        template <typename Derived>
        RefPtr( Derived* ptr )
            : m_pData( ptr )
        {
            this->IncreaseRef();
        }

		~RefPtr()
		{
			this->DecreaseRef();
			m_pData = nullptr;
		}

		RefPtr( RefPtr const& rhs )
			: m_pData( rhs.m_pData )
		{
			this->IncreaseRef();
		}

        template <typename Derived>
        RefPtr( Derived const& rhs )
            : m_pData( rhs.m_pData )
        {
            this->IncreaseRef();
        }

		RefPtr& operator=( RefPtr const& rhs )
		{
			// Note: it is ok that rhs is itself.
			rhs.IncreaseRef();
			this->DecreaseRef();
			m_pData = rhs.m_pData;
			return *this;
		}

        template <typename Derived>
        RefPtr& operator=( RefPtr<Derived> const& rhs )
        {
            // Note: it is ok that rhs is itself.
            rhs.IncreaseRef();
            this->DecreaseRef();
            m_pData = rhs.m_pData;
            return *this;
        }

		RefPtr( RefPtr&& rhs )
			: m_pData( rhs.m_pData )
		{
			rhs.m_pData = nullptr;
		}

        template <typename Derived>
        RefPtr( RefPtr<Derived>&& rhs )
            : m_pData( rhs.m_pData )
        {
            rhs.m_pData = nullptr;
        }

		RefPtr& operator=( RefPtr&& rhs )
		{
			if ( this != &rhs )
			{
				this->DecreaseRef();
				m_pData = rhs.m_pData;
				rhs.m_pData = nullptr;
			}
			return *this;
		}

        template <typename Derived>
        RefPtr& operator=( RefPtr<Derived>&& rhs )
        {
            if ( this != &rhs )
            {
                this->DecreaseRef();
                m_pData = rhs.m_pData;
                rhs.m_pData = nullptr;
            }
            return *this;
        }

		RefPtr& operator=( nullptr_t )
		{
            if ( m_pData != nullptr )
            {
			    this->DecreaseRef();
			    m_pData = nullptr;
            }
			return *this;
		}
		
		T* Raw() const
		{
			return m_pData;
		}

		T& operator*() const { return *m_pData; }
		T* operator->() const { return m_pData; }

		bool operator==( const RefPtr& rhs ) const { return m_pData == rhs.m_pData; }
		bool operator!=( const RefPtr& rhs ) const { return m_pData != rhs.m_pData; }

	private:

		void IncreaseRef() const
		{
			if ( m_pData != nullptr )
			{
				m_pData->IncreaseRef();
			}
		}

		void DecreaseRef()
		{
			if ( m_pData != nullptr )
			{
				if ( m_pData->DecreaseRef() == 0 )
				{
					EE::Delete( m_pData );
					m_pData = nullptr;
				}
			}
		}

	private:

		T*								m_pData = nullptr;
	};

	//-------------------------------------------------------------------------

	template <typename T>
    template <typename Ret, typename... Args>
	inline RefPtr<T> RefPtr<T>::New( Args&&... args )
	{
		return RefPtr<T>( EE::New<Ret, Args...>( std::forward<Args>( args )... ) );
	}
}