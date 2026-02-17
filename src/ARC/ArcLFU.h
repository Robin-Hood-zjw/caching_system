#pragma once

#include "ArcNode.h"

#include <map>
#include <list>
#include <mutex>
#include <unordered_map>

namespace CacheSpace {
    template<typename Key, typename Value>
    class ARC_LFU {
        public:
            using node_type = ArcNode<Key, Value>;
            using node_ptr = std::shared_ptr<node_type>;
            using node_map = std::unordered_map<Key, node_ptr>;
            using freq_map = std::map<size_t, std::list<node_ptr>>;

            explicit ARC_LFU(size_t capacity, size_t threshold):
                _minFreq(0), _capacity(capacity), 
                _ghostCapacity(capacity), _transformThreshold(threshold) {
                    initializeLists();
            }

            bool get(Key key, Value& value) {
                std::lock_guard<std::mutex> lock(_mutex);

                auto it = _mainCache.find(key);
                if (it != _mainCache.end()) {
                    updateNodeFreq(it->second);
                    value = it->second->getValue();
                    return true;
                }
                return false;
            }

            bool put(Key key, Value value) {
                if (_capacity == 0) return false;
                std::lock_guard<std::mutex> lock(_mutex);

                auto it = _mainCache.find(key);

                return it == _mainCache.end() ? 
                    addNewNode(key, value) : updateExistingNode(it->second, value);
            }

            bool contain(Key key) {
                return _mainCache.find(key) != _mainCache.end();
            }

            bool checkGhost(Key key) {
                auto it = _ghostCache.find(key);

                if (it != _ghostCache.end()) {
                    removeFromGhost(it->second);
                    _ghostCache.erase(it);
                    return true;
                }
                return false;
            }

            void increaseCapacity() {
                _capacity++;
            }

            bool decreaseCapacity() {
                if (_capacity == 0) return false;
                if (_mainCache.size() == _capacity) evictLeastFreq();

                _capacity--;
                return true;
            }
        private:
            size_t _minFreq;
            size_t _capacity;
            size_t _ghostCapacity;
            size_t _transformThreshold;

            std::mutex _mutex;

            node_map _mainCache;
            node_map _ghostCache;
            freq_map _freqMap;

            node_ptr _ghostHead;
            node_ptr _ghostTail;

            void initializeLists() {
                _ghostHead = std::make_shared<node_type>();
                _ghostTail = std::make_shared<node_type>();
                _ghostHead->next = _ghostTail;
                _ghostTail->prev = _ghostHead;
            }

            bool updateExistingNode(node_ptr node, const Value& value) {
                node->setValue(value);
                updateNodeFreq(node);
                return true;
            }

            bool addNewNode(const Key& key, const Value& value) {
                if (_mainCache.size() >= _capacity) evictLeastFreq();

                node_ptr newNode = std::make_shared<node_type>(key, value);
                _mainCache[key] = newNode;

                if (_freqMap.find(1) == _freqMap.end()) {
                    _freqMap[1] = std::list<node_ptr>();
                }
                _freqMap[1].push_back(newNode);
                _minFreq = 1;

                return true;
            }

            void updateNodeFreq(node_ptr node) {
                size_t oldFreq = node->getAccessCount();
                node->incrementAccessCount();

                auto& oldList = _freqMap[oldFreq];
                oldList.remove(node);

                size_t newFreq = node->getAccessCount();
                if (oldList.empty()) {
                    _freqMap.erase(oldFreq);
                    if (oldFreq == _minFreq) _minFreq = newFreq;
                }

                if (_freqMap.find(newFreq) == _freqMap.end()) {
                    _freqMap[newFreq] = std::list<node_ptr>();
                }
                _freqMap[newFreq].push_back(node);
            }

            void evictLeastFreq() {
                if (_freqMap.empty()) return;

                auto& minFreqList = _freqMap[_minFreq];
                if (minFreqList.empty()) return;

                node_ptr leastNode = minFreqList.front();
                minFreqList.pop_front();

                if (minFreqList.empty()) {
                    _freqMap.erase(_minFreq);
                    if (!_freqMap.empty()) _minFreq = _freqMap.begin()->first;
                }

                if (_ghostCache.size() >= _ghostCapacity) removeOldestGhost();
                addToGhost(leastNode);
                _mainCache.erase(leastNode->getKey());
            }

            void removeFromGhost(node_ptr node) {
                if (!node->prev.expired() && node->next) {
                    auto lastNode = node->prev.lock();
                    auto nextNode = node->next;

                    lastNode->next = nextNode;
                    nextNode->prev = lastNode;
                    node->prev.reset();
                    node->next = nullptr;
                }
            }

            void addToGhost(node_ptr node) {
                auto oldTail = _ghostTail->prev.lock();

                node->prev = oldTail;
                node->next = _ghostTail;

                if (!_ghostTail->prev.expired()) oldTail->next = node;
                _ghostTail->prev = node;
                _ghostCache[node->getKey()] = node;
            }

            void removeOldestGhost() {
                node_ptr oldestGhost = _ghostHead->next;

                if (oldestGhost && oldestGhost != _ghostTail) {
                    removeFromGhost(oldestGhost);
                    _ghostCache.erase(oldestGhost->getKey());
                }
            }
    };
}
