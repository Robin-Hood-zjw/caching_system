#pragma once

#include "CacheList.h"
#include "CachePolicy.h"

#include <mutex>
#include <unordered_map>

using namespace std;

template<typename Key, typename Value>
class LFU_Cache : public CachePolicy<key, Value> {
    public:
        using Node = typename FreqList<Key, Value>::Node;
        using node_ptr = shared_ptr<Node>;
        using node_map = unordered_map<Key, node_ptr>;

        LFU_Cache(int capacity, int maxAverageNum = 1000000):
            _capacity(capacity),
            _minFreq(INT8_MAX),
            _maxAvgNum(maxAverageNum) {}
        ~LFU_Cache() override = default;

        Value get(Key key) override {
            Value value;
            get(key, value);
            return value;
        }

        bool get(Key key, Value& val) override {
            lock_guard<mutex> lock(_mutex);

            if (_nodeRecords.count(key)) {
                putInternal(_nodeRecords[key], val);
                return true;
            }
            return false;
        }

        void put(Key key, Value& val) override {
            if (_capacity == 0) return;
            lock_guard<mutex> lock(_mutex);

            if (_nodeRecords.count(key)) {
                _nodeRecords[key]->value = val;
                getInternal(_nodeRecords[key], val);
                return;
            }

            putInternal(key, value);
        }

        void purge() {
            _nodeRecords.clear();
            _freqLists.clear();
        }
    private:
        int _capacity;
        int _minFreq;

        int _maxAvgNum;
        int _curAvgNum;
        int _curTotalNum;

        mutex _mutex; 
        node_map _nodeRecords;
        unordered_map<int, FreqList<Key, Value>*> _freqLists;

        void getInternal(node_ptr node, Value& val) {
            val = node->value;
            removeFromFreqList(node);

            node->freq++;
            addIntoFreqList(node);

            if (node->freq == _minFreq + 1 && _freqLists[node->freq - 1]->isEmpty()) _minFreq++;
            addFreqNum();
        }

        void putInternal(Key key, Value value) {
            if (_nodeRecords.size() == _capacity) cleanData();

            node_ptr node = make_shared<Node>(key, value);
            _nodeRecords[key] = node;
            addIntoFreqList(node);
            addFreqNum();
            _minFreq = min(_minFreq, 1);
        }

        void cleanData() {
            node_ptr node = _freqLists[_minFreq]->getFirstNode();
            removeFromFreqList(node);
            _nodeRecords.erase(node->key);
        }

        void removeFromFreqList(node_ptr node) {
            if (!node) return;

            int freq = node->freq;
            _freqLists[freq]->removeNode(node);
        }

        void addIntoFreqList(node_ptr node) {
            if (!node) return;

            int freq = node->freq;
            if (!_freqLists.count(freq)) _freqLists[freq] = new FreqList<key, Value>(freq);
            _freqLists[freq]->addNode(node);
        }

        void addFreqNum() {
            _curTotalNum++;
            _curAvgNum = _nodeRecords.empty() ? 
                0 : _curTotalNum / _nodeRecords.size();

            if (_curAvgNum > _maxAvgNum) handleOverMaxAvgNum();
        }

        void decreaseFreqNum(int num) {
            _curTotalNum -= num;

            _curAvgNum = _nodeRecords.empty() ? 
                0 : _curTotalNum / _nodeRecords.size();
        }

        void handleOverMaxAvgNum() {
            if (_nodeRecords.empty()) return;

            for (auto it = _nodeRecords.begin(); it != _nodeRecords.end(); ++it) {
                if (!it->second) continue;

                node_ptr node = it->second;
                int oldFreq = node->freq;
                int decay = _maxAvgNum / 2;

                node->freq -= decay;
                removeFromFreqList(node);

                if (node->freq < 1) node->freq = 1;

                int delta = node->feq - oldFreq;
                _curTotalNum += delta;
                addIntoFreqList(node);
            }
        }

        void updateMinFreq() {
            _minFreq = INT8_MAX;

            for (const auto& pair : _freqLists) {
                if (pair.second && !pair.second->isEmpty()) {
                    _minFreq = min(_minFreq, pair.first);
                }
            }
            if (_minFreq == INT8_MAX) _minFreq = 1;
        }
};