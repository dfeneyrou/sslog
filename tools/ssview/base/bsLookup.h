#pragma once

#include "bs.h"

// Simple flat hashmap with linear open addressing.
// Requirements: simple, fast, with few allocations and specialized for fixed lookups
//   - no deletion
//   - key is a non zero integer
//   - fixed hash function
//   - value is a trivially copyable structure, we never insert twice the same key, table size is a power of two

template<class K, class T>
class bsLookup
{
   public:
    bsLookup(int size = 1024)
    {
        int sizePo2 = 1;
        while (sizePo2 < size) sizePo2 *= 2;
        rehash(sizePo2);
    }

    ~bsLookup() { delete[] _nodes; }

    int size() const { return _size; }

    bool empty() const { return (_size == 0); }

    void clear()
    {
        _size = 0;
        for (int i = 0; i < _maxSize; ++i) _nodes[i].key = 0;
    }

    void insert(K key, T value)
    {
        uint32_t idx = hashFunc(key) & _mask;
        while (_nodes[idx].key) { idx = (idx + 1) & _mask; }  // Always stops because load factor < 1
        _nodes[idx].key   = key;                              // Never zero, so "non empty"
        _nodes[idx].value = value;
        _size += 1;
        if (_size * 3 > _maxSize * 2) { rehash(2 * _maxSize); }  // Max load factor is 0.66
    }

    bool find(K key, T& value) const
    {
        uint32_t idx = hashFunc(key) & _mask;
        while (true) {  // Always stops because load factor <= 0.66
            if (_nodes[idx].key == key) {
                value = _nodes[idx].value;
                return true;
            }
            if (_nodes[idx].key == 0) { return false; }  // Empty node
            idx = (idx + 1) & _mask;
        }
        return false;  // Never reached
    }

    bool exist(K key) const
    {
        uint32_t idx = hashFunc(key) & _mask;
        while (true) {  // Always stops because load factor <= 0.66
            if (_nodes[idx].key == key) return true;
            if (_nodes[idx].key == 0) return false;  // Empty node
            idx = (idx + 1) & _mask;
        }
        return false;  // Never reached
    }

    bool replace(K key, const T& newValue)
    {
        uint32_t idx = hashFunc(key) & _mask;
        while (true) {  // Always stops because load factor <= 0.66
            if (_nodes[idx].key == key) {
                _nodes[idx].value = newValue;
                return true;
            }
            if (_nodes[idx].key == 0) return false;  // Empty node
            idx = (idx + 1) & _mask;
        }
        return false;  // Never reached
    }

    void rehash(int maxSize)
    {
        int   oldSize = _maxSize;
        Node* old     = _nodes;
        _nodes        = new Node[maxSize];  // keys are set to zero (=empty)
        _maxSize      = maxSize;
        _mask         = (unsigned int)(maxSize - 1);
        _size         = 0;
        for (int i = 0; i < oldSize; ++i) {  // Transfer the previous filled nodes
            if (old[i].key == 0) continue;
            insert(old[i].key, old[i].value);
        }
        delete[] old;
    }

   private:
    uint32_t hashFunc(K key) const { return 16777619 * (uint32_t)(key ^ 2166136261); }

    struct Node {
        K key = 0;  // Caution: key cannot take the value zero in this lookup
        T value;
    };
    Node*    _nodes   = 0;
    uint32_t _mask    = 0;
    int      _size    = 0;
    int      _maxSize = 0;
    bsLookup(const bsLookup& other);      // To please static analyzers
    bsLookup& operator=(bsLookup other);  // To please static analyzers
};
