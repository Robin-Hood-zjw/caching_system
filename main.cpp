#include "./src/CachePolicy.h"
#include "./src/LRU/LRUCache.h"
#include "./src/LFU/LFUCache.h"
#include "./src/ARC/ArcCache.h"

#include <array>
#include <string>
#include <chrono>
#include <vector>
#include <iomanip>
#include <iostream>
#include <algorithm>

class Timer {
    public:
        Timer(): _start(std::chrono::high_resolution_clock::now()) {}

        double elapsed() {
            auto now = std::chrono::high_resolution_clock::now();
            return std::chrono::duration_cast<std::chrono::milliseconds>(now - _start).count();
        }

    private:
        std::chrono::time_point<std::chrono::high_resolution_clock> _start;
};

int main() {
    return 0;
};