#pragma once

#include <memory>

using namespace std;

template<typename Key, typename Value>
class FreqList {
    public:
        explicit FreqList(int n) : _freq(n) {
            _head = std::make_shared<Node>();
            _tail = std::make_shared<Node>();
            _head->next = _tail;
            _tail->prev = _head;
        }

        bool isEmpty() const {
            return _head->next == _tail;
        }

        void addNode(node_ptr node) {
            if (!node || !_head || !_tail) return;
            node_ptr lastNode = _tail->prev.lock();

            lastNode->next = node;
            node->prev = lastNode;
            node->next = _tail;
            _tail->prev = node;
        }

        void removeNode(node_ptr node) {
            if (!node || !_head || !_tail) return;
            if (node->prev.expired() || !node->next) return;

            auto lastNode = node->prev.lock();
            auto nextNode = node->next;

            lastNode->next = nextNode;
            nextNode->prev = lastNode;
            node->prev.reset();
            node->next = nullptr;
        }

        node_ptr getFirstNode() const {
            return _head->next;
        }

        friend class LFU_Cache<Key, Value>;

    private:
        struct Node {
            int freq;
            Key key;
            Value value;
            std::weak_ptr<Node> prev;
            std::shared_ptr<Node> next;

            Node(): freq(1), next(nullptr) {}
            Node(Key key, Value val): freq(1), key(key), value(val), next(nullptr) {}
        };
        using node_ptr = std::shared_ptr<Node>;

        int _freq;
        node_ptr _head;
        node_ptr _tail;
};