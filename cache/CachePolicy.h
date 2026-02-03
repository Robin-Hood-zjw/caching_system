#pragma once

template<typename Key, typename Value>
class CachePolicy {
    public:
        virtual ~CachePolicy() {};

        virtual bool get(Key key, Value val) = 0;
        virtual
};