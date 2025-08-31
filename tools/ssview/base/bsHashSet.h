#pragma once

#include <cstring>

// Simple and fast flat hash set with linear open addressing
// - key is a uint64_t hash (so hashing is external. Ensure good enough hashing to
// avoid clusters)
// - provided sizes shall always be power of two

class bsHashSet
{
   public:
    bsHashSet(uint32_t initSize = 8) { rehash(initSize); }
    bsHashSet(bsHashSet&& other) noexcept : _nodes(other._nodes), _mask(other._mask), _size(other._size), _maxSize(other._maxSize)
    {
        other._nodes   = 0;
        other._size    = 0;
        other._maxSize = 0;
    }
    ~bsHashSet()
    {
        delete[] _nodes;
        _nodes = 0;
    }

    bsHashSet& operator=(const bsHashSet& other) noexcept
    {
        if (&other != this) {
            rehash(other._maxSize);
            _size = other._size;
            memcpy(&_nodes[0], &other._nodes[0], _size * sizeof(uint64_t));
        }
        return *this;
    }
    bsHashSet& operator=(bsHashSet&& other) noexcept
    {
        if (&other != this) {
            delete[] _nodes;
            _nodes         = other._nodes;
            _mask          = other._mask;
            _size          = other._size;
            _maxSize       = other._maxSize;
            other._nodes   = 0;
            other._size    = 0;
            other._maxSize = 0;
        }
        return *this;
    }

    void clear()
    {
        memset(&_nodes[0], 0, _maxSize * sizeof(uint64_t));
        _size = 0;
    }
    bool     empty() const { return (_size == 0); }
    uint32_t size() const { return _size; }

    bool set(uint64_t hash)
    {
        if (hash == 0) hash = 1;
        int idx = hash & _mask;
        while (PL_UNLIKELY(_nodes[idx])) {
            if (PL_UNLIKELY(_nodes[idx] == hash)) return false;  // Already present
            idx = (idx + 1) & _mask;                             // Always stops because load factor (including deleted) < 1
        }
        _nodes[idx] = hash;  // Never zero, so "non empty"
        _size += 1;
        if (_size * 3 > _maxSize * 2) rehash(2 * _maxSize);  // Max load factor is 0.66
        return true;
    }

    bool unset(uint64_t hash)
    {
        if (hash == 0) hash = 1;
        int idx = hash & _mask;
        // Search for the hash
        while (PL_UNLIKELY(_nodes[idx] && _nodes[idx] != hash)) {
            idx = (idx + 1) & _mask;  // Always stops because load factor (including deleted) < 1
        }
        if (_nodes[idx] == 0) return false;  // Not found
        // Remove it, without using tombstone
        int nextIdx = idx;
        while (1) {
            nextIdx           = (nextIdx + 1) & _mask;
            uint64_t nextHash = _nodes[nextIdx];
            if (!nextHash) break;                                                       // End of cluster, we shall erase the previous one
            if ((int)(nextHash & _mask) <= idx || (int)(nextHash & _mask) > nextIdx) {  // Check that the 'next hash' can be moved before
                _nodes[idx] = nextHash;
                idx         = nextIdx;
            }
        }
        _nodes[idx] = 0;  // Empty
        --_size;
        return true;
    }

    bool find(uint64_t hash)
    {
        if (hash == 0) hash = 1;
        int idx = hash & _mask;
        while (1) {  // Always stops because load factor <= 0.66
            if (_nodes[idx] == hash) return true;
            if (_nodes[idx] == 0) return false;  // Empty node
            idx = (idx + 1) & _mask;
        }
        return false;  // Never reached
    }

    void rehash(int maxSize)
    {
        int       oldSize  = _maxSize;
        uint64_t* oldNodes = _nodes;
        _nodes             = new uint64_t[maxSize];
        memset(&_nodes[0], 0, maxSize * sizeof(uint64_t));
        _maxSize = maxSize;
        _mask    = maxSize - 1;
        _size    = 0;
        for (int i = 0; i < oldSize; ++i) {  // Transfer the previous filled nodes
            if (oldNodes[i]) set(oldNodes[i]);
        }
        delete[] oldNodes;
    }

   private:
    uint64_t* _nodes   = 0;
    uint32_t  _mask    = 0;
    uint32_t  _size    = 0;
    uint32_t  _maxSize = 0;
};
