#pragma once

// Simple list (partial implementation)
// Note: iterators have not been tested thoroughfully (only the used part...)

// Internal
#include "asserted.h"

template<typename T>
class bsList
{
   private:
    struct Node {
        Node *prev, *next;
        T     value;
    };

   public:
    typedef T value_type;  // Required for some std functions
    class iterator;
    class const_iterator;

    bsList() = default;
    ~bsList() { clear(); }

    bool empty() const { return (_size == 0); }
    int  size() const { return _size; }

    iterator insert(const_iterator pos, const T& value)
    {
        Node* posNode = pos._node;
        Node* newNode = new Node({posNode->prev, posNode, value});
        if (posNode->prev) posNode->prev->next = newNode;
        if (posNode == _first) _first = newNode;
        posNode->prev = newNode;
        ++_size;
        return newNode;
    }

    iterator erase(const_iterator pos)
    {
        asserted(_size);
        Node* posNode = pos._node;
        asserted(posNode != &_end);
        posNode->next->prev = posNode->prev;
        if (posNode->prev) posNode->prev->next = posNode->next;
        if (posNode == _first) _first = posNode->next;
        Node* nextNode = posNode->next;
        delete posNode;
        --_size;
        return nextNode;
    }

    void pop_front() { erase(_first); }

    void pop_back() { erase(_end.prev); }

    void push_front(const T& value)
    {
        Node* newNode = new Node({0, _first, value});
        _first->prev  = newNode;
        _first        = newNode;
        ++_size;
    }

    void push_back(const T& value)
    {
        Node* newNode = new Node({_end.prev, &_end, value});
        if (_end.prev) _end.prev->next = newNode;
        if (&_end == _first) _first = newNode;
        _end.prev = newNode;
        ++_size;
    }

    void clear()
    {
        while (!empty()) pop_front();
    }

    void splice(const_iterator pos, bsList& other, const_iterator otherIt)
    {
        // Detach otherNode from the other list
        Node* otherNode = otherIt._node;
        Node* posNode   = pos._node;
        asserted(posNode != &_end);
        asserted(otherNode != &_end);
        if (otherNode == posNode) return;  // Nothing to do
        otherNode->next->prev = otherNode->prev;
        if (otherNode->prev) otherNode->prev->next = otherNode->next;
        if (otherNode == other._first) other._first = otherNode->next;
        --other._size;
        // Insert otherNode in current list
        if (posNode->prev) posNode->prev->next = otherNode;
        otherNode->prev = posNode->prev;
        posNode->prev   = otherNode;
        otherNode->next = posNode;
        if (posNode == _first) _first = otherNode;
        ++_size;
    }

    T& front()
    {
        asserted(_size);
        return _first->value;
    }
    const T& front() const
    {
        asserted(_size);
        return _first->value;
    }
    T& back()
    {
        asserted(_size);
        return _end.prev->value;
    }
    const T& back() const
    {
        asserted(_size);
        return _end.prev->value;
    }

    // Iterators
    class iterator
    {
       public:
        iterator(typename bsList<T>::Node* node) : _node(node) {}
        iterator()  = default;
        ~iterator() = default;
        T&       operator*() { return _node->value; }
        const T& operator*() const { return _node->value; }
        T*       operator->() const { return &_node->value; }
        T&       operator++()
        {
            _node = _node->next;
            return *this;
        }
        T operator++(int)
        {
            iterator newIt(*this);
            _node = _node->next;
            return newIt;
        }

       private:
        bsList<T>::Node* _node = 0;
        friend bsList<T>;
    };

    class const_iterator
    {
       public:
        const_iterator(typename bsList<T>::Node* node) : _node(node) {}
        const_iterator()  = default;
        ~const_iterator() = default;
        const T& operator*() const { return _node->value; }
        const T* operator->() const { return &_node->value; }
        T&       operator++()
        {
            _node = _node->next;
            return *this;
        }
        T operator++(int)
        {
            iterator newIt(*this);
            _node = _node->next;
            return newIt;
        }

       private:
        bsList<T>::Node* _node = 0;
        friend bsList<T>;
    };

    iterator       begin() { return _first; }
    const_iterator cbegin() const { return _first; }
    iterator       end() { return &_end; }
    const_iterator cend() const { return &_end; }

   private:
    int   _size  = 0;
    Node  _end   = {0, 0};
    Node* _first = &_end;
};
