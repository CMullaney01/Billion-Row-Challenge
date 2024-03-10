#ifndef BUFFER_DEFINED
#define BUFFER_DEFINED

#include <vector>
#include <atomic> // For atomic operations
#include <tuple>
#include <string>

class Buffer {
private:
    std::vector<std::tuple<std::string, double>> buffer; // Buffer of tuples (city name, temperature)
    size_t capacity;
    std::atomic<size_t> head; // Atomic indices for thread-safe access
    std::atomic<size_t> tail;

public:
    Buffer(size_t capacity);
    bool isEmpty() const;
    bool isFull() const;
    void enqueue(const std::tuple<std::string, double>& item);
    std::tuple<std::string, double> dequeue();
};

#endif
