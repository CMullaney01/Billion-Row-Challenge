#include "buffer.h"

Buffer::Buffer(size_t capacity) : buffer(capacity), capacity(capacity), head(0), tail(0) {}

bool Buffer::isEmpty() const {
    return head == tail;
}

bool Buffer::isFull() const {
    return (tail + 1) % capacity == head;
}

void Buffer::enqueue(const std::tuple<std::string, double>& item) {
    if (isFull()) {
        // Buffer is full, handle accordingly (e.g., wait or overwrite)
        return; // For simplicity, let's return for now
    }

    buffer[tail] = item;
    tail = (tail + 1) % capacity;
}

std::tuple<std::string, double> Buffer::dequeue() {
    if (isEmpty()) {
        // Buffer is empty, handle accordingly (e.g., wait or return a default value)
        return std::make_tuple("", 0.0); // For simplicity, return a default-constructed tuple
    }

    std::tuple<std::string, double> item = buffer[head];
    head = (head + 1) % capacity;
    return item;
}
