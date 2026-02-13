#pragma once

#include "CacheNode.h"
#include "../CachePolicy.h"

#include <mutex>
#include <vector>
#include <memory>
#include <cstring>
#include <unordered_map>

template<typename Key, typename Value>
class LRU_Cache : public CachePolicy<Key, Value> {
    public:
        using node_type = Node<Key, Value>;
        using node_ptr = std::shared_ptr<node_type>;
        using node_map = std::unordered_map<Key, node_ptr>;

        LRU_Cache(int capacity): _capacity(capacity) {
            initializeList();
        }
        ~LRU_Cache() override = default;

        Value get(Key key) override {
            Value val;
            get(key, val);

            return val;
        }

        bool get(Key key, Value& value) override {
            std::lock_guard<std::mutex> lock(_mutex);

            if (_nodeRecords.count(key)) {
                moveToMostRecent(_nodeRecords[key]);
                value = _nodeRecords[key]->getValue();
                return true;
            }
            return false;
        }

        void put(Key key, Value value) override {
            if (_capacity <= 0) return;
            std::lock_guard<std::mutex> lock(_mutex);

            if (_nodeRecords.count(key)) {
                updateExistingNode(_nodeRecords[key], value);
                return;
            }

            addNewNode(key, value);
        }

        void remove(Key key) {
            std::lock_guard<std::mutex> lock(_mutex);

            if (_nodeRecords.count(key)) {
                removeNode(_nodeRecords[key]);
                _nodeRecords.erase(key);
            }
        }

    private:
        int _capacity;
        std::mutex _mutex;
        node_ptr _dummyHead;
        node_ptr _dummyTail;
        node_map _nodeRecords;

        void initializeList() {
            _dummyHead = std::make_shared<node_type>(Key(), Value());
            _dummyTail = std::make_shared<node_type>(Key(), Value());
            _dummyHead->next = _dummyTail;
            _dummyTail->prev = _dummyHead;
        }

        void updateExistingNode(node_ptr node, const Value& value) {
            node->setValue(value);
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
            auto oldRecent = _dummyTail->prev.lock();

            oldRecent->next = node;
            node->prev = oldRecent;
            node->next = _dummyTail;
            _dummyTail->prev = node;
        }

        void addNewNode(const Key& key, const Value& value) {
            if (_nodeRecords.size() >= _capacity) evictLeastRecent();

            node_ptr node = std::make_shared<node_type>(key, value);
            _nodeRecords[key] = node;
            insertNode(node);
        }

        void evictLeastRecent() {
            node_ptr node = _dummyHead->next;
            _nodeRecords.erase(node->getKey());
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
            Value oldValue;
            bool inCache = LRU_Cache<Key, Value>::get(key, oldValue);

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
        int  _k;
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
                    _slicedCache.push_back(new LRU_Cache<Key, Value>(size));
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
            return _slicedCache[index]->get(key, val);
        }

        void put(Key key, Value val) {
            size_t index = Hash(key) % _sliceNum;
            _slicedCache[index]->put(key, val);
        }
    private:
        int _sliceNum;
        size_t _capacity;
        vector<unique_ptr<LRU_Cache<Key, Value>>> _slicedCache;

        size_t Hash(Key key) {
            hash<Key> hashFunc;
            return hashFunc(key);
        }
};