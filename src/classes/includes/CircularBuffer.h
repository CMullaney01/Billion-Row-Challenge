#ifndef CIRCULARBUFFER_H
#define CIRCULARBUFFER_H

#include <vector>
#include <atomic> // For atomic operations
#include <tuple>
#include <string>

class CircularBuffer {
private:
    std::vector<std::tuple<std::string, double>> buffer; // Buffer of tuples (city name, temperature)
    size_t capacity;
    std::atomic<size_t> head; // Atomic indices for thread-safe access
    std::atomic<size_t> tail;

public:
    CircularBuffer(size_t capacity);
    bool isEmpty() const;
    bool isFull() const;
    void enqueue(const std::tuple<std::string, double>& item);
    std::tuple<std::string, double> dequeue();
};

#include "CircularBuffer.cpp" // Include the implementation file at the end of the header

#endif /* CIRCULARBUFFER_H */
