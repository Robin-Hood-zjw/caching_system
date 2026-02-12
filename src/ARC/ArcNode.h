#pragma once

#include <memory>

template<typename Key, typename Value>
class ArcNode {
    public:
        ArcNode(): _accessCnt(1), next(nullptr) {}

        ArcNode(Key key, Value value): 
            _key(key), _value(value),
            _accessCnt(1), next(nullptr) {}

        Key getKey() const {
            return _key;
        }

        Value getValue() const {
            return _value;
        }

        size_t getAccessCount() const {
            return _accessCnt;
        }

        void setValue(const Value& value) {
            _value = value;
        }

        void incrementAccessCount() {
            _accessCnt++;
        }

        template<typename Key, typename Value> friend class ARC_LRU;
        template<typename Key, typename Value> friend class ARC_LFU;
    private:
        Key _key;
        Value _value;
        size_t _accessCnt;
        std::weak_ptr<ArcNode> prev;
        std::shared_ptr<ArcNode> next;
};