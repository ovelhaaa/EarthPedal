#pragma once
#include <vector>

template <typename T>
class RingBuffer {
public:
    RingBuffer(size_t size) : buffer_(size), size_(size), head_(0) {
        // Initialize with zeros
        for(size_t i=0; i<size; ++i) buffer_[i] = T(0);
    }

    void push(T value) {
        head_ = (head_ + size_ - 1) % size_; // Move head back (simulating push to front or back depending on access pattern)
        // Wait, the original q::ring_buffer might push to front.
        // Let's assume standard ring buffer behavior: push to one end, access relative to head.
        // If q::ring_buffer[0] is the newest element, then push needs to put it at head.
        buffer_[head_] = value;
    }

    // Accessor: 0 is newest
    T operator[](size_t index) const {
        return buffer_[(head_ + index) % size_];
    }

private:
    std::vector<T> buffer_;
    size_t size_;
    size_t head_;
};
