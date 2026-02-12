#pragma once

#include "ArcNode.h"

#include <map>
#include <mutex>
#include <unordered_map>

template<typename Key, typename Value>
class ARC_LFU {
    public:
        using node_type = ArcNode<Key, Value>;
        using node_ptr = std::shared_ptr<node_ptr>;
        using node_map = std::unordered_map<Key, node_ptr>;
        using freq_map = std::map<size_t, std::list<node_ptr>>;

        explicit ARC_LFU(size_t capacity, size_t threshold):
            _minFreq(0), _capacity(capacity), _ghostCapacity(capacity), _transformThreshold(threshold) {
                initializeLists();
        }

        bool get(Key key, Value& value) {
            std::lock_guard<std::mutex> lock(_mutex);

            auto itr = _mainCache.find(key);
            if (itr != _mainCache.end()) {
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

            return it != _mainCache.end() ? 
                updateExistingNode(it->second, value) : addNewNode(key, value);
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
            if (_capacity <= 0) return false;
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


};