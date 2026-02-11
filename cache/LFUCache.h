#pragma once

#include "CacheList.h"
#include "CachePolicy.h"

#include <mutex>
#include <unordered_map>

using namespace std;

template<typename Key, typename Value>
class LFU_Cache : public CachePolicy<Key, Value> {
    public:
        using Node = typename FreqList<Key, Value>::Node;
        using node_ptr = shared_ptr<Node>;
        using node_map = unordered_map<Key, node_ptr>;

        LFU_Cache(int capacity, int maxAverageNum = 1000000): 
            _capacity(capacity),
            _minFreq(INT_MAX),
            _maxAvgNum(maxAverageNum),
            _curAvgNum(0),
            _curTotalNum(0) {}
        ~LFU_Cache() override = default;

        Value get(Key key) override {
            Value value;
            get(key, value);
            return value;
        }

        bool get(Key key, Value& val) override {
            lock_guard<mutex> lock(_mutex);

            if (_nodeRecords.count(key)) {
                getInternal(_nodeRecords[key], val);
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

            putInternal(key, val);
        }

        void purge() {
            lock_guard<mutex> lock(_mutex);

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
        unordered_map<int, unique_ptr<FreqList<Key, Value>>> _freqLists;


        void getInternal(node_ptr node, Value& val) {
            val = node->value;
            removeFromFreqList(node);

            node->freq++;
            addIntoFreqList(node);

            if (node->freq == _minFreq + 1 && _freqLists[_minFreq]->isEmpty()) _minFreq++;
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
            // node_ptr node = _freqLists[_minFreq]->getFirstNode();

            auto it = _freqLists.find(_minFreq);
            if (it == _freqLists.end() || it->second->isEmpty())
                updateMinFreq();

            node_ptr node = _freqLists[_minFreq]->getFirstNode();
            _curTotalNum -= node->freq;

            removeFromFreqList(node);
            _nodeRecords.erase(node->key);
        }

        void removeFromFreqList(node_ptr node) {
            if (!node) return;

            int freq = node->freq;
            auto it = _freqLists.find(freq);
            if (it != _freqLists.end()) it->second->removeNode(node);
        }

        void addIntoFreqList(node_ptr node) {
            if (!node) return;

            int freq = node->freq;
            if (!_freqLists.count(freq)) _freqLists[freq] = make_unique<FreqList<Key, Value>>(freq);

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
                removeFromFreqList(node);

                int decay = _maxAvgNum / 2;
                node->freq = max(1, node->freq - decay);

                int delta = node->freq - oldFreq;
                _curTotalNum += delta;
                addIntoFreqList(node);
            }

            updateMinFreq();
        }

        void updateMinFreq() {
            _minFreq = INT_MAX;

            for (const auto& pair : _freqLists) {
                if (pair.second && !pair.second->isEmpty()) {
                    _minFreq = min(_minFreq, pair.first);
                }
            }
            if (_minFreq == INT_MAX) _minFreq = 1;
        }
};