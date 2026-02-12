#pragma once

#include "ArcNode.h"

#include <mutex>
#include <unordered_map>

template<typename Key, typename Value>
class ARC_LRU {
    public:
        using node_type = ArcNode<Key, Value>;
        using node_ptr = std::shared_ptr<node_type>;
        using node_map = std::unordered_map<Key, node_ptr>;

        explicit ARC_LRU(size_t capacity, int threshold):
            _capacity(capacity),
            _ghostCapacity(capacity),
            _transformThreshold(threshold) {
                initializeLists();
            }
        
        bool get(Key key, Value& value, bool& shouldTransform) {
            std::lock_guard<std::mutex> lock(_mutex);

            auto itr = _mainCache.find(key);
            if (itr != _mainCache.end()) {
                shouldTransform = updateNodeAccess(itr->second);
                value = it->second->getValue();
                return true;
            }
            
            return false;
        }

        void put(Key key, Value value) {
            if (_capacity == 0) return;
            std::lock_guard<std::mutex> lock(_mutex);

            auto it = _mainCache.find(key);

            return it != _mainCache.end() ? 
                updateExistingNode(it->second, value) : ddNewNode(key, value);
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
            if (_mainCache.size() == _capacity) evictLeastRecent();

            --_capacity;
            return true;
        }
    private:
        size_t _capacity;
        size_t _ghostCapacity;
        size_t _transformThreshold;
        std::mutex _mutex;
        node_map _mainCache;
        node_map _ghostCache;
        node_ptr _mainHead;
        node_ptr _mainTail;
        node_ptr _ghostHead;
        node_ptr _ghostTail;

        void initializeLists() {
            _mainHead = std::make_shared<node_type>();
            _mainTail = std::make_shared<node_type>();
            _mainHead->next = _mainTail;
            _mainTail->prev = _mainHead;

            _ghostHead = std::make_shared<>(node_type);
            _ghostTail = std::make_shared<>(node_type);
            ghostHead_->next_ = ghostTail_;
            ghostTail_->prev_ = ghostHead_;
        }

        bool updateExistingNode(node_ptr node, const Value& value) {
            node->setValue(value);
            moveToFront(node);
            return true;
        }

        bool addNewNode(const Key& key, const Value& value) {
            if (_mainCache.size() >= _capacity) evictLeastRecent();

            node_ptr newNode = std::make_shared<node_type>(key, value);
            _mainCache[key] = newNode;
            addToFront(newNode);
            return true;
        }

        void moveToFront(node_ptr node) {
            if (!node->prev.expired() && node->next) {
                auto lastNode = node->prev.lock();
                auto nextNode = node->next;

                lastNode->next = nextNode;
                nextNode->prev = lastNode;
                node->next = nullptr;
            }

            addToFront(node);
        }

        void addToFront(node_ptr node) {
            auto oldHead = _mainHead->next;

            _mainHead->next = node;
            node->prev = _mainHead;
            node->next = oldHead;
            oldHead->prev = node;
        }

        void evictLeastRecent() {
            node_ptr leastRecent = _mainTail->prev.lock();
            if (!leastRecent || leastRecent == _mainHead) return;

            removeFromMain(leastRecent);

            if (_ghostCache.size() >= _ghostCapacity) removeOldestGhost();
            addToGhost(leastRecent);

            _mainCache.erase(leastRecent);
        }
};