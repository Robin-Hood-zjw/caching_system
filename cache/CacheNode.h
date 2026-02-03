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