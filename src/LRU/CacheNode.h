#pragma once

#include <memory>

namespace CacheSpace {
    template<typename Key, typename Value>
    class Node {
        public:
            Node(Key k, Value v): _key(k), _val(v), _accessCnt(1) {}

            Key getKey() const { return _key; }

            Value getValue() const { return _val; }

            void setValue(const Value& value) { _val = value; }

            size_t getAccessCount() const { return _accessCnt; }

            void incrementAccessCount() { ++_accessCnt; }

            friend class LRU_Cache<Key, Value>;
        private:
            Key _key;
            Value _val;
            size_t _accessCnt;
            std::weak_ptr<Node<Key, Value>> prev;
            std::shared_ptr<Node<Key, Value>> next;
    };
}
