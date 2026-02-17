#pragma once

#include <chrono>

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
