#pragma once

#include "CacheNode.h"
#include "CachePolicy.h"

#include <mutex>
#include <vector>
#include <memory>
#include <unordered_map>

using namespace std;

template<typename Key, typename Value>
class LRU_Node : public CacheNode<Key, Value> {
    public:
        weak_ptr<LRU_Node<Key, Value>> prev;
        shared_ptr<LRU_Node<Key, Value>> next;

        LRU_Node(Key key, Value val) : CacheNode<Key, Value>(key, val) {}

        friend class LRU_Cache<Key, Value>;
};

template<typename Key, typename Value>
class LRU_Cache : public CachePolicy<Key, Value> {
    public:
        using LRU_node_type = LRU_Node<Key, Value>;
        using node_ptr = shared_ptr<LRU_node_type>;
        using node_map = unordered_map<Key, node_ptr>;

        LRU_Cache(int capacity): _capacity(capacity) {
            initializeList();
        }
        ~LRU_Cache() override = default;

        Value get(Key key) override {
            Value val;
            get(key, val);

            return val;
        }

        bool get(Key key, Value& val) override {
            lock_guard<mutex> lock(_mutex);

            if (nodeRecords.count(key)) {
                moveToMostRecent(nodeRecords[key]);
                val = nodeRecords[key]->getValue();
                return true;
            }

            return false;
        }

        void put(Key key, Value val) override {
            if (_capacity <= 0) return;
            lock_guard<mutex> lock(_mutex);

            if (nodeRecords.count(key)) {
                updateExistingNode(nodeRecords[key], val);
                return;
            }

            addNewNode(key, val);
        }

        void remove(Key key) {
            lock_guard<mutex> lock(_mutex);

            if (nodeRecords.count(key)) {
                removeNode(nodeRecords[key]);
                nodeRecords.erase(key);
            }
        }

    private:
        mutex _mutex;
        int _capacity;
        node_ptr dummyHead;
        node_ptr dummyTail;
        node_map nodeRecords;

        void initializeList() {
            dummyHead = make_shared<LRU_node_type>(Key(), Value());
            dummyTail = make_shared<LRU_node_type>(Key(), Value());
            dummyHead->next = dummyTail;
            dummyTail->prev = dummyHead;
        }

        void updateExistingNode(node_ptr node, const Value& val) {
            node->setValue(val);
            moveToMostRecent(node);
        }

        void moveToMostRecent(node_ptr node) {
            removeNode(node);
            insertNode(node);
        }

        void removeNode(node_ptr node) {
            if (!node->prev.expired() && node->next) {
                auto lastNode = node->prev.lock();
                auto nextNode = node->next;
                lastNode->next = nextNode;
                nextNode->prev = lastNode;

                node->prev.reset();
                node->next = nullptr;
            }
        }

        void insertNode(node_ptr node) {
            auto oldRecent = dummyTail->prev.lock();

            oldRecent->next = node;
            node->prev = oldRecent;
            node->next = dummyTail;
            dummyTail->prev = node;
        }

        void addNewNode(const Key& key, const Value& val) {
            if (nodeRecords.size() >= _capacity) evictLeastRecent();

            node_ptr node = make_shared<LRU_node_type>(key, val);
            nodeRecords[key] = node;
            insertNode(node);
        }

        void evictLeastRecent() {
            node_ptr node = dummyHead->next;
            nodeRecords.erase(node->getKey());
            removeNode(node);
        }
};

template<typename Key, typename Value>
class LRU_Cache_K : public CachePolicy<Key, Value> {
    private:
        int _k;

};

// template<typename Key, typename Value>
// class LRU_Cache_Hash : public CachePolicy<Key, Value> {
//     private:
//         int sliceNum;
//         int _capacity;

// };