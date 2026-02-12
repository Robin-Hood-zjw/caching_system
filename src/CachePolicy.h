#pragma once

template<typename Key, typename Value>
class CachePolicy {
    public:
        virtual ~CachePolicy() {};

        virtual Value get(Key key) = 0;
        virtual bool get(Key key, Value& val) = 0;
        virtual void put(Key key, Value val) = 0;
};