#pragma once

#include "CacheList.h"
#include "CachePolicy.h"

#include <mutex>
#include <unordered_map>

template<typename Key, typename Value>
class LFU_Cache : public CachePolicy<key, Value> {
    public:
        using Node = typename FrequenceList<Key, Value>::Node;
        using node_ptr = shared_ptr<Node>;
        using node_map = unordered_map<Key, node_ptr>;

        LFU_Cache(int capacity): _capacity(capacity), minFreq(INT8_MAX) {}
        ~LFU_Cache() override = default;

        
    private:
        int _capacity;
        int minFreq;
        mutex _mutex; 
        node_map nodeRecords;
        unordered_map<int, FrequenceList<Key, Value>*> freqMap;
};