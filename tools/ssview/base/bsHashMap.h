#pragma once

#include <cstring>

#include "bs.h"
#include "bsVec.h"

// Simple and fast flat hash table with linear open addressing, dedicated to
// build a lookup
// - Hashing is internal (for uint32_t & uint64_t keys) and an external api is provided (for performance).
//     If external, ensure that hashing is good enough to avoid clusters, and that external api is always used
// - best storage packing is for 32 bits key size
// - single value per key (overwrite of existing value)

template<typename K, typename V, uint64_t HashFunc(K)>
class bsHashMap
{
   public:
    bsHashMap(int initSize = 8) { rehash(initSize); }
    bsHashMap(const bsHashMap<K, V, HashFunc>& other) noexcept
    {
        asserted(other._maxSize > 0);
        rehashPo2(other._maxSize);
        _size = other._size;
        memcpy(&_nodes[0], &other._nodes[0],
               _size * sizeof(Node));  // Only for numeric or integral types
    }
    bsHashMap(bsHashMap<K, V, HashFunc>&& other) noexcept
        : _nodes(other._nodes), _mask(other._mask), _size(other._size), _maxSize(other._maxSize)
    {
        other._nodes   = 0;
        other._size    = 0;
        other._maxSize = 0;
    }
    ~bsHashMap()
    {
        delete[] _nodes;
        _nodes = nullptr;
    }

    bsHashMap<K, V, HashFunc>& operator=(const bsHashMap<K, V, HashFunc>& other) noexcept
    {
        if (&other == this) return *this;
        rehashPo2(other._maxSize);
        _size = other._size;
        memcpy(&_nodes[0], &other._nodes[0],
               _size * sizeof(Node));  // Only for numeric or integral types
        return *this;
    }
    bsHashMap<K, V, HashFunc>& operator=(bsHashMap<K, V, HashFunc>&& other) noexcept
    {
        delete[] _nodes;
        _nodes         = other._nodes;
        _mask          = other._mask;
        _size          = other._size;
        _maxSize       = other._maxSize;
        other._nodes   = 0;
        other._size    = 0;
        other._maxSize = 0;
        return *this;
    }

    void clear()
    {
        for (uint32_t i = 0; i < _maxSize; ++i) _nodes[i].hash = 0;
        _size = 0;
    }

    bool empty() const { return (_size == 0); }

    uint32_t size() const { return _size; }

    uint32_t capacity() const { return _maxSize; }

    // Exclusive usage, either hash is provided, either hash is computed, do not
    // mix
    bool insert(K key, V value) { return insert(HashFunc(key), key, value); }
    bool insert(uint64_t hash, K key, V value)
    {
        plgScope(BSHL, "insert");
        plgData(BSHL, "hash", hash);
        if (hash == 0) hash = 1;
        int idx = hash & _mask;
        while (PL_UNLIKELY(_nodes[idx].hash)) {
            plgData(BSHL, "busy index", idx);
            if (_nodes[idx].hash == hash && _nodes[idx].key == key) {  // Case overwrite existing value
                plgData(BSHL, "override index", idx);
                _nodes[idx].value = value;
                return false;  // Overwritten
            }
            idx = (idx + 1) & _mask;  // Always stops because load factor < 1
        }
        plgData(BSHL, "write index", idx);
        _nodes[idx] = {hash, key, value};  // Hash is never zero, so "non empty"
        _size += 1;
        if (_size * 3 > _maxSize * 2) {
            rehashPo2(2 * _maxSize);  // Max load factor is 0.66
        }
        return true;  // Added
    }

    // Exclusive usage, either hash is provided, either hash is computed, do not
    // mix
    bool erase(K key) { return erase(HashFunc(key), key); }
    bool erase(uint64_t hash, K key)
    {
        plgScope(BSHL, "erase");
        plgData(BSHL, "hash", hash);
        if (hash == 0) hash = 1;
        int idx = hash & _mask;
        // Search for the hash
        while (PL_UNLIKELY(_nodes[idx].hash && (_nodes[idx].hash != hash || _nodes[idx].key != key))) {
            idx = (idx + 1) & _mask;  // Always stops because load factor < 1
        }
        if (_nodes[idx].hash == 0) {
            plgText(BSHL, "Action", "Not found");
            return false;  // Not found
        }
        // Remove it, without using tombstone
        int nextIdx = idx;
        plgData(BSHL, "start index", idx);
        while (true) {
            nextIdx           = (nextIdx + 1) & _mask;
            uint64_t nextHash = _nodes[nextIdx].hash;
            plgData(BSHL, "next index", nextIdx);
            if (!nextHash) {
                plgText(BSHL, "Action", "empty next hash: end of cluster");
                break;  // End of cluster, we shall erase the previous one
            }
            plgData(BSHL, "next index hash", (int)(nextHash & _mask));
            // Can the 'next hash' replace the one to remove(=idx)? Due to the wrap, it is one of these cases:
            int nextHashIndex = (int)(nextHash & _mask);
            if ((nextIdx > idx && (nextHashIndex <= idx || nextHashIndex > nextIdx)) ||  // NextIdx did not wrap
                (nextIdx < idx && (nextHashIndex <= idx && nextHashIndex > nextIdx))) {  // NextIdx wrapped
                plgText(BSHL, "Action", "current replaced by next");
                _nodes[idx] = _nodes[nextIdx];
                idx         = nextIdx;
            }
        }
        plgData(BSHL, "nullified index", idx);
        _nodes[idx].hash = 0;  // Empty
        --_size;
        return true;
    }

    // Exclusive usage, either hash is provided, either hash is computed, do not mix
    V* find(K key) const { return find(HashFunc(key), key); }
    V  findUnref(K key) const
    {
        V* ptr = find(HashFunc(key), key);
        return ptr ? *ptr : V{};
    }
    V* find(uint64_t hash, K key) const
    {
        plgScope(BSHL, "find");
        plgData(BSHL, "hash", hash);
        if (hash == 0) hash = 1;
        int idx = (int)(hash & _mask);
        while (true) {  // Always stops because load factor <= 0.66
            plgData(BSHL, "testing index", idx);
            if (_nodes[idx].hash == hash && _nodes[idx].key == key) {
                plgText(BSHL, "Action", "key found!");
                return &_nodes[idx].value;
            }
            if (_nodes[idx].hash == 0) {
                plgText(BSHL, "Action", "empty hash: end of cluster");
                return nullptr;  // Empty node
            }
            idx = (idx + 1) & _mask;
        }
        return nullptr;  // Never reached
    }

    void rehash(int newSize)
    {
        int sizePo2 = 1;
        while (sizePo2 < newSize) sizePo2 *= 2;
        rehashPo2(sizePo2);
    }

    struct Node {
        uint64_t hash;
        K        key;
        V        value;
    };

    Node* begin() { return next(_nodes - 1); }

    Node* next(Node* it)
    {
        asserted(it);
        ++it;
        while (it < _nodes + _maxSize && it->hash == 0) ++it;
        return (it < _nodes + _maxSize) ? it : nullptr;
    }

    // Simple hashing based on FNV1a
    static uint64_t hashFunc(uint64_t key) { return ((key ^ 14695981039346656037ULL) * 1099511628211ULL); }

   private:
    // Mandatory: maxSize shall be a power of two
    void rehashPo2(int maxSize)
    {
        plgScope(BSHL, "rehashPo2");
        plgData(BSHL, "old size", _maxSize);
        plgData(BSHL, "new size", maxSize);
        int   oldSize  = _maxSize;
        Node* oldNodes = _nodes;
        _nodes         = new Node[maxSize];
        for (int i = 0; i < maxSize; ++i) _nodes[i].hash = 0;
        _maxSize = maxSize;
        _mask    = maxSize - 1;
        _size    = 0;
        for (int i = 0; i < oldSize; ++i) {  // Transfer the previous filled nodes
            if (oldNodes[i].hash) insert(oldNodes[i].hash, oldNodes[i].key, oldNodes[i].value);
        }
        delete[] oldNodes;
    }

    // Fields
    Node*    _nodes   = nullptr;
    uint32_t _mask    = 0;
    uint32_t _size    = 0;
    uint32_t _maxSize = 0;
};

// Hashing, based on FNV1a-64, but per uint64_t and not characters
static constexpr uint64_t BS_FNV_HASH_OFFSET   = 14695981039346656037ULL;
static constexpr uint64_t BS_FNV_HASH_PRIME    = 1099511628211ULL;
static constexpr uint64_t BS_FNV_HASH32_OFFSET = 2166136261ULL;
static constexpr uint64_t BS_FNV_HASH32_PRIME  = 16777619ULL;
inline uint64_t
bsHashStep(uint64_t novelty, uint64_t previous = BS_FNV_HASH_OFFSET)
{
    return (novelty ^ previous) * BS_FNV_HASH_PRIME;
}
inline uint64_t
bsHashStepChain(uint64_t value)
{
    return bsHashStep(value);
}
// Recent steps first
template<typename... Args>
inline uint64_t
bsHashStepChain(uint64_t value, Args... args)
{
    return bsHashStep(value, bsHashStepChain(args...));
}
inline uint64_t
bsHashString(const char* s)
{
    uint64_t h = BS_FNV_HASH_OFFSET;
    while (*s) h = (h ^ ((uint64_t)(*s++))) * BS_FNV_HASH_PRIME;
    return h ? h : 1;
}
inline uint64_t
bsHashString(const char* s, const char* sEnd)
{
    uint64_t h = BS_FNV_HASH_OFFSET;
    while (s != sEnd) h = (h ^ ((uint64_t)(*s++))) * BS_FNV_HASH_PRIME;
    return h ? h : 1;
}
inline uint64_t
bsHash32String(const char* s)
{
    uint64_t h = BS_FNV_HASH32_OFFSET;
    while (*s) h = (h ^ ((uint64_t)(*s++))) * BS_FNV_HASH32_PRIME;
    return h ? (h & 0xFFFFFFFF) : 1;
}

#ifdef BS_TESTU
// Compile with:
//  g++ -I ../../include -I . -DBS_TESTU=1 -DPL_IMPLEMENTATION=1 -DUSE_PL=1
//  -DPL_GROUP_BSHL=1 -ggdb3 -fsanitize=address -fno-omit-frame-pointer -x c++
//  bsHashMap.h -lasan -lpthread
int
main(int argc, char* argv[])
{
#define TH(v) ((14695981039346656037ULL ^ (uint64_t)(v)) * 1099511628211ULL)
    const int ITEM_QTY      = 512;
    const int ITERATION_QTY = 50;
    if (argc > 1 && !strcmp(argv[1], "console")) peStartModeConsole("testu bsHashMap");
    if (argc > 1 && !strcmp(argv[1], "file")) peStartModeFile("testu bsHashMap", "testuBsHashMap.plt");
    printf("Start unit test for bsHashMap\n");
    auto h = bsHashMap<int, int>(1024);  // Start with low capacity to stress the rehash
    int  tmp;
    // Add all numbers
    plgBegin(BSHL, "Initial fill");
    for (int i = 0; i < ITEM_QTY; ++i) { asserted(h.insert(TH(i), i, i), i, TH(i)); }
    plgEnd(BSHL, "");
    // Stress through iterations
    for (int iteration = 0; iteration < ITERATION_QTY; ++iteration) {
        plgScope(BSHL, "Iteration");
        plgData(BSHL, "Number", iteration);
        {
            plgScope(BSHL, "Check all items are inside");
            asserted(h.size() == ITEM_QTY);
            for (int i = 0; i < ITEM_QTY; ++i) { asserted(h.find(TH(i), i, tmp), iteration, i); }
            plgText(BSHL, "Status", "Ok");
            plgData(BSHL, "Capacity", h.capacity());
        }
        const int startI   = iteration * 2;
        const int fraction = 2 + iteration;
        {
            plgScope(BSHL, "Remove part of items");
            for (int i = startI; i < startI + ITEM_QTY / fraction; ++i) { asserted(h.erase(TH(i), i), iteration, i); }
            for (int i = startI; i < startI + ITEM_QTY / fraction; ++i) { asserted(!h.find(TH(i), i, tmp), iteration, i); }
            for (int i = startI + 1 + ITEM_QTY / fraction; i < ITEM_QTY; ++i) { asserted(h.find(TH(i), i, tmp), iteration, i); }
            asserted(h.size() == ITEM_QTY - (ITEM_QTY / fraction));
            plgText(BSHL, "Status", "Ok");
        }
        {
            plgScope(BSHL, "Put back first half of items");
            for (int i = startI; i < startI + ITEM_QTY / fraction; ++i) { asserted(h.insert(TH(i), i, i), iteration, i); }
        }
    }
    peStop();
    printf("End unit test for bsHashMap: success\n");
    return 0;
}
#endif
