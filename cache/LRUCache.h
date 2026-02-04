#pragma once

#include "CacheNode.h"
#include "CachePolicy.h"

#include <mutex>
#include <memory>
#include <unordered_map>

using namespace std;

template<typename Key, typename Value>
class LRU_Node : public CacheNode<Key, Value> {
    public:
        weaker_ptr<LRU_Node<Key, Value>> prev;
        shared_ptr<LRU_Node<Key, Value>> next;

        LRU_Node<Key key, Value val> : CacheNode<Key, Value>(key, val) {}

        friend class LRU_Cache<Key, Value>;
};

template<typename Key, typename Value>
class LRU_Cache : public CachePolicy<Key, Value> {
    public:
        using LRU_node_type = LRU_Node<key, Value>;
        using node_ptr = shared_ptr<LRU_node_type>;
        using node_map = unordered_map<Key, node_ptr>;

        LRU_Cache(int capacity): _capacity(capacity) {}
        ~LRU_Cache() override = default;

        Value get(Key key) override {
            Value val;
            get(key, val);

            return vl;
        }

        bool get(Key key, Value& val) override {
            lock_guard<mutex> lock(_mutex);

            if (nodeRecords.count(key)) {
                value = nodeRecords[key]->getValue();
                return true;
            }

            return false;
        }

        void put(Key key, Value val) override {
            if (_capacity <= 0) return;
            lock_guard<mutex> lock(_mutex);

            if (nodeRecords.count(key)) {
                return;
            }
        }

    private:
        mutex _mutex;
        int _capacity;
        node_ptr dummyHead;
        node_ptr dummyTail;
        node_map nodeRecords;

        void initializeList() {
            
        }
};

// template<typename Key, typename Value>
// class LRU_Cache_K : public CachePolicy<Key, Value> {};

// template<typename Key, typename Value>
// class LRU_Cache_Hash : public CachePolicy<Key, Value> {};