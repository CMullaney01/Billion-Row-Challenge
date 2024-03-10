#include <iostream>
#include "buffer.h"

Buffer::Buffer(size_t capacity) : buffer(capacity), capacity(capacity), head(0), tail(0) {}

bool Buffer::isEmpty() const {
    return head == tail;
}

bool Buffer::isFull() const {
    return (tail + 1) % capacity == head;
}

void Buffer::enqueue(std::tuple<std::string, double>&& item) {
    if (isFull()) {
        // Handle full buffer
        return;
    }
    buffer[tail] = std::move(item);
    tail = (tail + 1) % capacity;
}

std::tuple<std::string, double> Buffer::dequeue() {
    if (isEmpty()) {
        // Handle empty buffer
        return std::make_tuple("", 0.0);
    }
    std::tuple<std::string, double> item = std::move(buffer[head]);
    head = (head + 1) % capacity;
    return item;
}

void Buffer::printBuffer() const {
    std::cout << "Buffer Contents:" << std::endl;
    for (size_t i = head; i != tail; i = (i + 1) % capacity) {
        std::cout << std::get<0>(buffer[i]) << " " << std::get<1>(buffer[i]) << std::endl;
    }
}
