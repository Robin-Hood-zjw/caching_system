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

        std::cout << std::endl;
}

void testHotDataAccess() {
    std::cout << "\n=== 测试场景1: 热点数据访问测试 ===" << std::endl;

    const int CAPACITY = 20;
    const int OPERATIONS = 500000;
    const int HOT_KEYS = 20;
    const int COLD_KEYS = 5000;

    CacheSpace::LRU_Cache<int, std::string> LRU(CAPACITY);
    CacheSpace::LFU_Cache<int, std::string> LFU(CAPACITY);
    CacheSpace::ARC_Cache<int, std::string> ARC(CAPACITY);

    CacheSpace::LRU_K_Cache<int, std::string> LRU_K(CAPACITY, HOT_KEYS + COLD_KEYS, 2);
    CacheSpace::LFU_Cache<int, std::string> LFU_Aging(CAPACITY, 20000);

    std::random_device rd;
    std::mt19937 gen(rd());

    std::array<CacheSpace::CachePolicy<int, std::string>*, 5> caches = {&LRU, &LFU, &ARC, &LRU_K, &LFU_Aging};
    std::vector<int> hits (5, 0);
    std::vector<int> get_operations (5, 0);
    std::vector<std::string> names = {"LRU", "LFU", "ARC", "LRU-K", "LFU-Aging"};

        for (int i = 0; i < caches.size(); ++i) {
        // 先预热缓存，插入一些数据
        for (int key = 0; key < HOT_KEYS; ++key) {
            std::string value = "value" + std::to_string(key);
            caches[i]->put(key, value);
        }
        
        // 交替进行put和get操作，模拟真实场景
        for (int op = 0; op < OPERATIONS; ++op) {
            // 大多数缓存系统中读操作比写操作频繁
            // 所以设置30%概率进行写操作
            bool isPut = (gen() % 100 < 30); 
            int key;
            
            // 70%概率访问热点数据，30%概率访问冷数据
            if (gen() % 100 < 70) {
                key = gen() % HOT_KEYS; // 热点数据
            } else {
                key = HOT_KEYS + (gen() % COLD_KEYS); // 冷数据
            }
            
            if (isPut) {
                // 执行put操作
                std::string value = "value" + std::to_string(key) + "_v" + std::to_string(op % 100);
                caches[i]->put(key, value);
            } else {
                // 执行get操作并记录命中情况
                std::string result;
                get_operations[i]++;
                if (caches[i]->get(key, result)) {
                    hits[i]++;
                }
            }
        }

        printResults("热点数据访问测试", CAPACITY, get_operations, hits);

    }
}

int main() {
    return 0;
};