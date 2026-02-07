#pragma once

#include "CacheNode.h"
#include "CachePolicy.h"

#include <mutex>
#include <vector>
#include <memory>
#include <cstring>
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
class LRU_K_Cache : public CachePolicy<Key, Value> {
    public:
        LRU_K_Cache(int capacity, int countCapacity, int k)
            : LRU_Cache<Key, Value>(capacity),
            pendingList(make_unique<LRU_Cache<Key, size_t>>(countCapacity)),
            _k(k) {}

        Value get(Key key) {
            Value result;
            bool inCache = LRU_Cache<Key, Value>::get(key, result);
            if (inCache) return result;

            size_t pendingCnt = 0;
            pendingList->get(key, pendingCnt);
            pendingCnt++;
            pendingList->put(key, pendingCnt);

            if (pendingMap.count(key) && pendingCnt >= _k) {
                result = pendingMap[key];

                LRU_Cache<Key, Value>::put(key, result);
                pendingList->remove(key);
                pendingMap.erase(key);
            }

            return result;
        }

        void put(Key key, Value val) {
            Value result;
            bool inCache = LRU_Cache<Key, Value>::get(key, result);

            if (inCache) {
                LRU_Cache<Key, Value>::put(key, val);
                return;
            }

            size_t pendingCnt = 0;
            pendingList->get(key, pendingCnt);
            pendingCnt++;
            pendingList->put(key, pendingCnt);
            pendingMap[key] = val;

            if (pendingCnt >= _k) {
                LRU_Cache<Key, Value>::put(key, val);
                pendingList->remove(key);
                pendingMap.erase(key);
            }
        }
    private:
        int _k;
        unordered_map<Key, Value> pendingMap;
        unique_ptr<LRU_Cache<Key, size_t>> pendingList;
};

template<typename Key, typename Value>
class Hash_LRU_Cache : public CachePolicy<Key, Value> {
    public:
        Hash_LRU_Cache(size_t capacity, int sliceNum):
            _sliceNUm(sliceNum > 0 ? sliceNum : thread::hardware_concurrency()),
            _capacity(capacity) {
                size_t size = ceil(_capacity / static_cast<double>(_sliceNum));

                for (size_t i = 0; i < _sliceNum; i++) {
                    LRU_sliced_Cache.push_back(new LRU_Cache<Key, Value>(size));
                }
            }

        Value get(Key key) {
            Value result;
            memset(&result, 0, sizeof(result));
            get(key, result);

            return result;
        }

        bool get(Key key, Value& val) {
            size_t index = Hash(key) % _sliceNum;
            LRU_sliced_Cache[index]->get(key, val);
        }

        void put(Key key, Value val) {}
    private:
        int _sliceNum;
        size_t _capacity;
        vector<unique_ptr<LRU_Cache<Key, Value>>> LRU_sliced_Cache;

        size_t Hash(Key key) {
            hash<Key> hashFunc;
            return hashFunc(key);
        }
};