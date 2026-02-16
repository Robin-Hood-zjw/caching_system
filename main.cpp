#include "./src/CachePolicy.h"
#include "./src/LRU/LRUCache.h"
#include "./src/LFU/LFUCache.h"
#include "./src/ARC/ArcCache.h"

#include <array>
#include <string>
#include <chrono>
#include <vector>
#include <random>
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

void printResults(
    const std::string& testName, int capacity, 
    const std::vector<int>& get_operations,
    const std::vector<int>& hits) {
        std::cout << "=== " << testName << " 结果汇总 ===" << std::endl;
        std::cout << "缓存大小： " << capacity << std::endl;

        std::vector<std::string> names;
        if (hits.size() == 3) {
            names = {"LRU", "LFU", "ARC"};
        } else if (hits.size() == 4) {
            names = {"LRU", "LFU", "ARC", "LRU-K"};
        } else if (hits.size() == 5) {
            names = {"LRU", "LFU", "ARC", "LRU-K", "LFU-Aging"};
        }

        for (size_t i = 0; i < hits.size(); i++) {
            double hitRate = 100.0 * hits[i] / get_operations[i];
            string starter = i < names.size() ? names[i] : "Algorithm" + std::to_string(i + 1);

            std::cout << starter << " - 命中率: " << std::fixed << std::setprecision(2) << hitRate << std::endl;
            std::cout << "(" << hits[i] << "/" << get_operations[i] << ")" << std::endl;
        }
        
}

int main() {
    return 0;
};