#pragma once
#include "Base/Esoterica.h"

//-------------------------------------------------------------------------

namespace eastl
{
    class allocator;

    template <typename T>
    struct equal_to;

    template <typename T>
    struct less;

    template <typename T>
    struct hash;

    template <typename T, typename Allocator>
    class basic_string;

    template <typename T, int nodeCount, bool bEnableOverflow, typename OverflowAllocator>
    class fixed_string;

    template <typename T, typename Allocator>
    class vector;

    template <typename T, size_t nodeCount, bool bEnableOverflow, typename OverflowAllocator>
    class fixed_vector;

    template <typename T, size_t N>
    struct array;

    template <typename Key, typename T, typename Hash, typename Predicate, typename Allocator, bool bCacheHashCode>
    class hash_map;

    template <typename Key, typename T, size_t nodeCount, size_t bucketCount, bool bEnableOverflow, typename Hash, typename Predicate, bool bCacheHashCode, typename OverflowAllocator>
    class fixed_hash_map;

    template <typename T1, typename T2>
    struct pair;

    template <typename Key, typename Compare, typename Allocator>
    class set;

    template <typename Key, typename Compare, typename Allocator>
    class multiset;

    template <typename Value, typename Hash, typename Predicate, typename Allocator, bool bCacheHashCode>
    class hash_set;

    template <typename Value, typename Hash, typename Predicate, typename Allocator, bool bCacheHashCode>
    class hash_multiset;

    template <typename Value, typename Hash, typename Predicate, typename Allocator, bool bCacheHashCode>
    using unordered_set = hash_set<Value, Hash, Predicate, Allocator, bCacheHashCode>;

    template <typename Value, typename Hash, typename Predicate, typename Allocator, bool bCacheHashCode>
    using unordered_multiset = hash_multiset<Value, Hash, Predicate, Allocator, bCacheHashCode>;

    template <typename Key, typename T, typename Compare, typename Allocator>
    class map;

    template <typename Key, typename T, size_t nodeCount, bool bEnableOverflow, typename Compare, typename OverflowAllocator>
    class fixed_map;

    template <typename E, typename Allocator>
    class list;

    template <typename T, size_t nodeCount, bool bEnableOverflow, typename OverflowAllocator>
    class fixed_list;
}

//-------------------------------------------------------------------------

namespace EE
{
    using String = eastl::basic_string<char, eastl::allocator>;
    template<size_t S> using TInlineString = eastl::fixed_string<char, S, true, eastl::allocator>;
    using InlineString = eastl::fixed_string<char, 255, true, eastl::allocator>;

    template<typename T> using TVector = eastl::vector<T, eastl::allocator>;
    template<typename T, size_t S> using TInlineVector = eastl::fixed_vector<T, S, true, eastl::allocator>;
    template<typename T, size_t S> using TArray = eastl::array<T, S>;

    using Blob = TVector<uint8_t>;

    template<typename K, typename V> using THashMap = eastl::hash_map<K, V, eastl::hash<K>, eastl::equal_to<K>, eastl::allocator, false>;
    //template<typename K, typename V, size_t NodeCount> using TInlineHashMap = eastl::fixed_hash_map<K, V, NodeCount, NodeCount + 1, true, eastl::hash<K>, eastl::equal_to<K>, false, eastl::allocator>;
    //template<typename K, typename V, size_t NodeCount> using TFixedHashMap = eastl::fixed_hash_map<K, V, NodeCount, NodeCount + 1, false, eastl::hash<K>, eastl::equal_to<K>, false, eastl::allocator>;

    template<typename K, typename V> using TPair = eastl::pair<K, V>;

    template <typename T> using TSet = eastl::set<T, eastl::less<T>, eastl::allocator>;
    template <typename T> using TMultiSet = eastl::multiset<T, eastl::less<T>, eastl::allocator>;
    template <typename T> using TUnorderedSet = eastl::unordered_set<T, eastl::hash<T>, eastl::equal_to<T>, eastl::allocator, false>;
    template <typename T> using TUnorderedMultiSet = eastl::unordered_multiset<T, eastl::hash<T>, eastl::equal_to<T>, eastl::allocator, false>;

    template <class K, class V> using TMap = eastl::map<K, V, eastl::less<K>, eastl::allocator>;
    template <class K, class V, size_t NodeCount> using TInlineMap = eastl::fixed_map<K, V, NodeCount, true, eastl::less<K>, eastl::allocator>;
    template <class K, class V, size_t NodeCount> using TFixedMap = eastl::fixed_map<K, V, NodeCount, false, eastl::less<K>, eastl::allocator>;

    template <typename E> using TList = eastl::list<E, eastl::allocator>;

    template<typename T, size_t S> using TInlineList = eastl::fixed_list<T, S, true, eastl::allocator>;
    template<typename T, size_t S> using TFixedList = eastl::fixed_list<T, S, false, eastl::allocator>;
}