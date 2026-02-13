#pragma once

#include "ArcLRU.h"
#include "ArcLFU.h"
#include "../CachePolicy.h"

#include <memory>

template<typename Key, typename Value>
class ARC_Cache : public CachePolicy<Key, Value> {
    public:
        explicit ARC_Cache(size_t capacity = 10, size_t threshold = 2):
            _capacity(capacity),
            _transformThreshold(threshold),
            _LRU_Part(std::make_unique<ARC_LRU<Key, Value>>(capacity, threshold)),
            _LFU_Part(std::make_unique<ARC_LFU<Key, Value>>(capacity, threshold)) {}
        ~ARC_Cache() override = default;

        Value get(Key key) override {
            Value value;
            get(key, value);
            return value;
        }

        bool get(Key key, Value& value) override {
            checkGhostCaches(key);

            bool shouldTransform = false;
            if (_LRU_Part->get(key, value, shouldTransform)) {
                if (shouldTransform) _LFU_Part->put(key, value);
                return true;
            }
            return _LFU_Part->get(key, value);
        }

        void put(Key key, Value value) override {
            checkGhostCaches(key);

            bool inLFU = _LFU_Part->contain(key);
            _LRU_Part->put(key, value);
            if (inLFU) _LFU_Part->put(key, value);
        }
    private:
        size_t _capacity;
        size_t _transformThreshold;

        std::unique_ptr<ARC_LRU<Key, Value>> _LRU_Part;
        std::unique_ptr<ARC_LFU<Key, Value>> _LFU_Part;

        bool checkGhostCaches(Key key) {
            bool inGhost = false;

            if (_LRU_Part->checkGhost(key)) {
                if (_LFU_Part->decreaseCapacity()) _LRU_Part->increaseCapacity();
                inGhost = true;
            } else if (_LFU_Part->checkGhost(key)) {
                if (_LRU_Part->decreaseCapacity()) _LFU_Part->increaseCapacity();
                inGhost = true;
            }

            return inGhost;
        }
};