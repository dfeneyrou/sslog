#pragma once

#include <cstring>

#include "bs.h"
#include "bsVec.h"

// Simple and fast flat hash table with linear open addressing and external hashing (= uint64_t key)
//  - Null hash is turned into hash=1 (sentinel value)
//  - Only for numeric or integral types
//  - The initial size shall be a power of two

template<typename V>
class bsKeyLessHashMap
{
   public:
    bsKeyLessHashMap(int initSize = 8) { rehash(initSize); }

    bsKeyLessHashMap(const bsKeyLessHashMap<V>& other) noexcept
    {
        rehashPo2(other._maxSize);
        _size = other._size;
        memcpy(&_nodes[0], &other._nodes[0], _size * sizeof(Node));
    }

    bsKeyLessHashMap(bsKeyLessHashMap<V>&& other) noexcept
        : _nodes(other._nodes), _mask(other._mask), _size(other._size), _maxSize(other._maxSize)
    {
        other._nodes   = 0;
        other._size    = 0;
        other._maxSize = 0;
    }

    ~bsKeyLessHashMap()
    {
        delete[] _nodes;
        _nodes = nullptr;
    }

    bsKeyLessHashMap<V>& operator=(const bsKeyLessHashMap<V>& other) noexcept
    {
        if (&other == this) return *this;
        rehashPo2(other._maxSize);
        _size = other._size;
        memcpy(&_nodes[0], &other._nodes[0], _size * sizeof(Node));
        return *this;
    }

    bsKeyLessHashMap<V>& operator=(bsKeyLessHashMap<V>&& other) noexcept
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

    bool insert(uint64_t hash, V value)
    {
        asserted(hash != 0);
        int idx = hash & _mask;
        while (PL_UNLIKELY(_nodes[idx].hash)) {
            if (_nodes[idx].hash == hash) {  // Case overwrite existing value
                _nodes[idx].value = value;
                return false;  // Overwritten
            }
            idx = (idx + 1) & _mask;  // Always stops because load factor < 1
        }
        _nodes[idx] = {hash, value};  // Hash is never zero, so "non empty"
        _size += 1;
        if (_size * 3 > _maxSize * 2) {
            rehashPo2(2 * _maxSize);  // Max load factor is 0.66
        }
        return true;  // Added
    }

    bool erase(uint64_t hash)
    {
        asserted(hash != 0);
        int idx = hash & _mask;
        // Search for the hash
        while (PL_UNLIKELY(_nodes[idx].hash && _nodes[idx].hash != hash)) {
            idx = (idx + 1) & _mask;  // Always stops because load factor < 1
        }
        if (_nodes[idx].hash == 0) {
            return false;  // Not found
        }
        // Remove it, without using tombstone
        int nextIdx = idx;
        while (true) {
            nextIdx           = (nextIdx + 1) & _mask;
            uint64_t nextHash = _nodes[nextIdx].hash;
            if (!nextHash) {
                break;  // End of cluster, we shall erase the previous one
            }
            // Can the 'next hash' replace the one to remove(=idx)? Due to the wrap, it is one of these cases:
            int nextHashIndex = (int)(nextHash & _mask);
            if ((nextIdx > idx && (nextHashIndex <= idx || nextHashIndex > nextIdx)) ||  // NextIdx did not wrap
                (nextIdx < idx && (nextHashIndex <= idx && nextHashIndex > nextIdx))) {  // NextIdx wrapped
                _nodes[idx] = _nodes[nextIdx];
                idx         = nextIdx;
            }
        }
        _nodes[idx].hash = 0;  // Empty
        --_size;
        return true;
    }

    V findUnref(uint64_t hash) const
    {
        V* ptr = find(hash);
        return ptr ? *ptr : V{};
    }

    V* find(uint64_t hash) const
    {
        if (hash == 0) return nullptr;
        int idx = (int)(hash & _mask);
        while (true) {  // Always stops because load factor <= 0.66
            if (_nodes[idx].hash == hash) { return &_nodes[idx].value; }
            if (_nodes[idx].hash == 0) {
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

   private:
    // Mandatory: maxSize shall be a power of two
    void rehashPo2(int maxSize)
    {
        int   oldSize  = _maxSize;
        Node* oldNodes = _nodes;
        _nodes         = new Node[maxSize];
        for (int i = 0; i < maxSize; ++i) _nodes[i].hash = 0;
        _maxSize = maxSize;
        _mask    = maxSize - 1;
        _size    = 0;
        for (int i = 0; i < oldSize; ++i) {  // Transfer the previous filled nodes
            if (oldNodes[i].hash) insert(oldNodes[i].hash, oldNodes[i].value);
        }
        delete[] oldNodes;
    }

    // Fields
    Node*    _nodes   = nullptr;
    uint32_t _mask    = 0;
    uint32_t _size    = 0;
    uint32_t _maxSize = 0;
};
