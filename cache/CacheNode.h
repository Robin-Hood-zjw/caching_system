#pragma once

template<typename Key, typename Value>
class Node {
    protected:
        Key key;
        Value val;

    public:
        Node(Key k, Value v): key(k), val(v) {}

        Key getKey() const { return key; }

        Value getValue() const { return val; }

        void setValue(const Value& value) { val = value; }
};

template<typename Key, typename Value>
class LRU_Node : public CacheNode<Key, Value> {
    public:
        weak_ptr<LRU_Node<Key, Value>> prev;
        shared_ptr<LRU_Node<Key, Value>> next;

        LRU_Node(Key key, Value val) : CacheNode<Key, Value>(key, val) {}

        friend class LRU_Cache<Key, Value>;
};