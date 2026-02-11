#pragma once

#include "CacheList.h"
#include "CachePolicy.h"

#include <mutex>
#include <unordered_map>

template<typename Key, typename Value>
class LFU_Cache : public CachePolicy<key, Value> {
    public:
        using Node = typename FreqList<Key, Value>::Node;
        using node_ptr = shared_ptr<Node>;
        using node_map = unordered_map<Key, node_ptr>;

        LFU_Cache(int capacity): _capacity(capacity), minFreq(INT8_MAX) {}
        ~LFU_Cache() override = default;

        Value get(Key key) override {
            Value value;
            get(key, value);
            return value;
        }

        bool get(Key key, Value& val) override {
            lock_guard<mutex> lock(_mutex);

            if (nodeRecords.count(key)) {
                putInternal(nodeRecords[key], val);
                return true;
            }
            return false;
        }

        void put(Key key, Value& val) override {
            if (_capacity == 0) return;
            lock_guard<mutex> lock(_mutex);

            if (nodeRecords.count(key)) {
                nodeRecords[key]->value = val;
                getInternal(nodeRecords[key], val);
                return;
            }

            putInternal(key, value);
        }

        void purge() {
            nodeRecords.clear();
            freqLists.clear();
        }
    private:
        int _capacity;
        int minFreq;
        mutex _mutex; 
        node_map nodeRecords;
        unordered_map<int, FreqList<Key, Value>*> freqLists;

        void getInternal(node_ptr node, Value& value) {
            value = node->value;
            removeFromList(node);

            node->freq++;
            addIntoList(node);

            if (node->freq == minFreq + 1 && freqLists[node->freq - 1]->isEmpty()) minFreq++;
        }

        void putInternal(Key key, Value value) {
            if (nodeRecords.size() == _capacity) cleanData();

            node_ptr node = make_shared<Node>(key, value);
            nodeRecords[key] = node;
            addIntoList(node);
            minFreq = min(minFreq, 1);
        }

        void cleanData() {
            node_ptr node = freqLists[minFreq]->getFirstNode();
            removeFromList(node);
            nodeRecords.erase(node->key);
        }

        void removeFromList(node_ptr node) {
            if (!node) return;

            int freq = node->freq;
            freqLists[freq]->removeNode(node);
        }

        void addIntoList(node_ptr node) {
            if (!node) return;

            int freq = node->freq;
            if (!freqLists.count(freq)) freqLists[freq] = new FreqList<key, Value>(freq);
            freqLists[freq]->addNode(node);
        }
};