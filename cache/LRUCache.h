#pragma once
using namespace std;

#include "CacheNode.h"

template<typename Key, typename Value>
class LRU_Node : public CacheNode<Key, Value> {
    public:
        weaker_ptr<LRU_Node<Key, Value>> prev;
        shared_ptr<LRU_Node<Key, Value>> next;

        LRU_Node<Key key, Value val> : CacheNode<Key, Value>(key, val) {}
};

