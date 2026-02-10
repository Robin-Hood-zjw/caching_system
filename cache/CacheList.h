#pragma once

#include <memory>

using namespace std;

template<typename Key, typename Value>
class FrequenceList {
    public:
        
    private:
        struct Node {
            int freq;
            Key key;
            Value value;
            weak_ptr<Node> prev;
            shared_ptr<Node> next;

            Node(): freq(1), next(nullptr) {}
            Node(Key key, Value val): freq(1), key(key), value(val), next(nullptr) {}
        };
        using node_ptr = shared_ptr<Node>;

        int _freq;
        node_ptr _head;
        node_ptr _tail;
};