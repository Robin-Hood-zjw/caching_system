#pragma once

#include "CacheList.h"
#include "../CachePolicy.h"

#include <mutex>
#include <thread>
#include <unordered_map>


namespace CacheSpace {
    template<typename Key, typename Value>
    class LFU_Cache : public CachePolicy<Key, Value> {
        public:
            using Node = typename FreqList<Key, Value>::Node;
            using node_ptr = std::shared_ptr<Node>;
            using node_map = std::unordered_map<Key, node_ptr>;

            LFU_Cache(int capacity, int maxAverageNum = 1000000): 
                _minFreq(INT_MAX),
                _capacity(capacity),
                _maxAvgNum(maxAverageNum),
                _curAvgNum(0),
                _curTotalNum(0) {}
            ~LFU_Cache() override = default;

            Value get(Key key) override {
                Value value;
                get(key, value);
                return value;
            }

            bool get(Key key, Value& value) override {
                std::lock_guard<std::mutex> lock(_mutex);

                if (_nodeRecords.count(key)) {
                    getInternal(_nodeRecords[key], value);
                    return true;
                }
                return false;
            }

            void put(Key key, Value value) override {
                if (_capacity == 0) return;
                std::lock_guard<std::mutex> lock(_mutex);

                if (_nodeRecords.count(key)) {
                    _nodeRecords[key]->value = value;
                    getInternal(_nodeRecords[key], value);
                    return;
                }

                putInternal(key, value);
            }

            void purge() {
                std::lock_guard<std::mutex> lock(_mutex);
                _nodeRecords.clear();
                _freqLists.clear();
            }
        private:
            int _minFreq;
            int _capacity;
            int _maxAvgNum;
            int _curAvgNum;
            int _curTotalNum;

            std::mutex _mutex; 
            node_map _nodeRecords;
            std::unordered_map<int, FreqList<Key, Value>*> _freqLists;


            void getInternal(node_ptr node, Value& value) {
                value = node->value;
                removeFromFreqList(node);

                node->freq++;
                addIntoFreqList(node);

                if (node->freq == _minFreq + 1 && _freqLists[_minFreq]->isEmpty()) _minFreq++;
                addFreqNum();
            }

            void putInternal(Key key, Value value) {
                if (_nodeRecords.size() == _capacity) cleanData();

                node_ptr node = std::make_shared<Node>(key, value);
                _nodeRecords[key] = node;
                addIntoFreqList(node);
                addFreqNum();
                _minFreq = std::min(_minFreq, 1);
            }

            void cleanData() {
                node_ptr node = _freqLists[_minFreq]->getFirstNode();
                _curTotalNum -= node->freq;
                _nodeRecords.erase(node->key);
                decreaseFreqNum(node->freq);
            }

            void removeFromFreqList(node_ptr node) {
                if (!node) return;

                int freq = node->freq;
                auto it = _freqLists.find(freq);
                if (it != _freqLists.end()) _freqLists[freq]->removeNode(node);
            }

            void addIntoFreqList(node_ptr node) {
                if (!node) return;

                int freq = node->freq;
                if (!_freqLists.count(freq))
                    _freqLists[freq] = new FreqList<Key, Value>(freq);

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
                    removeFromFreqList(node);

                    int oldFreq = node->freq;
                    int decay = _maxAvgNum / 2;
                    node->freq = std::max(1, oldFreq - decay);

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
                        _minFreq = std::min(_minFreq, pair.first);
                    }
                }
                if (_minFreq == INT_MAX) _minFreq = 1;
            }
    };

    template<typename Key, typename Value>
    class Hash_LFU_Cache : public CachePolicy<Key, Value> {
        public:
            Hash_LFU_Cache(size_t capacity, int sliceNum, int maxAvgNum = 10):
                _capacity(capacity),
                _sliceNum(sliceNum > 0 ? sliceNum : std::thread::hardware_concurrency()) {
                    size_t size = std::ceil(_capacity / static_cast<double>(_sliceNum));

                    for (size_t i = 0; i < _sliceNum; i++) {
                        _slicedCache.push_back(new LFU_Cache<Key, Value>(size, maxAvgNum));
                    }
                }

            Value get(Key key) {
                Value value;
                get(key, value);
                return value;
            }

            bool get(Key key, Value& value) {
                size_t index = Hash(key) % _sliceNum;
                return _slicedCache[index]->get(key, value);
            }

            void put(Key key, Value value) {
                size_t index = Hash(key) % _sliceNum;
                _slicedCache[index]->put(key, value);
            }

            void purge() {
                for (auto& cache : _slicedCache) cache->purge();
            }
        private:
            int _sliceNum;
            size_t _capacity;
            std::vector<std::unique_ptr<LFU_Cache<Key, Value>>> _slicedCache;

            size_t Hash(Key key) {
                std::hash<Key> hashFunc;
                return hashFunc(key);
            }
    };
}
