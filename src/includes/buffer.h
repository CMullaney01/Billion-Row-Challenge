#ifndef BUFFER_H
#define BUFFER_H

#include <vector>
#include <tuple>
#include <string>

class Buffer {
private:
    std::vector<std::tuple<std::string, double>> buffer;
    size_t capacity;
    size_t head;
    size_t tail;

public:
    Buffer(size_t capacity);
    bool isEmpty() const;
    bool isFull() const;
    void enqueue(std::tuple<std::string, double>&& item);
    std::tuple<std::string, double> dequeue();
    void printBuffer() const;
};

#endif /* BUFFER_H */
