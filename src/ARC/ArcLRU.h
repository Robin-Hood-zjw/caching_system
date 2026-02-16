#pragma once

#include "ArcNode.h"

#include <mutex>
#include <unordered_map>

namespace CacheSpace {
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

                auto it = _mainCache.find(key);
                if (it != _mainCache.end()) {
                    shouldTransform = updateNodeAccess(it->second);
                    value = it->second->getValue();
                    return true;
                }
                return false;
            }

            void put(Key key, Value value) {
                if (_capacity == 0) return;
                std::lock_guard<std::mutex> lock(_mutex);

                auto it = _mainCache.find(key);

                return it == _mainCache.end() ? 
                    addNewNode(key, value) : updateExistingNode(it->second, value);
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
            std::mutex _mutex;

            size_t _capacity;
            size_t _ghostCapacity;
            size_t _transformThreshold;

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

                _ghostHead = std::make_shared<node_type>();
                _ghostTail = std::make_shared<node_type>();
                _ghostHead->next = _ghostTail;
                _ghostTail->prev = _ghostHead;
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

            bool updateNodeAccess(node_ptr node) {
                moveToFront(node);
                node->incrementAccessCount();
                return node->getAccessCount() >= _transformThreshold;
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

                _mainCache.erase(leastRecent->getKey());
            }

            void removeFromMain(node_ptr node) {
                if (!node->prev.expired() && node->next) {
                    auto lastNode = node->prev.lock();
                    auto nextNode = node->next;

                    lastNode->next = nextNode;
                    nextNode->prev = lastNode;
                    node->prev.reset();
                    node->next = nullptr;
                }
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
                node->_accessCnt = 1;

                node->next = _ghostHead->next;
                node->prev = _ghostHead;
                _ghostHead->next->prev = node;
                _ghostHead->next = node;

                _ghostCache[node->getKey()] = node;
            }

            void removeOldestGhost() {
                node_ptr oldestGhost = _ghostTail->prev.lock();
                if (!oldestGhost || oldestGhost == _ghostHead) return;

                removeFromGhost(oldestGhost);
                _ghostCache.erase(oldestGhost->getKey());
            }
    };
}

