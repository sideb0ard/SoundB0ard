#include <iostream>
#include <vector>

template <typename T>
class CircularBuffer {
 public:
  CircularBuffer(size_t capacity)
      : buffer(capacity), head(0), tail(0), size(0) {}

  void push_back(const T& val) {
    buffer[tail] = val;
    tail = (tail + 1) % buffer.size();
    if (size < buffer.size()) {
      size++;
    } else {
      head = (head + 1) % buffer.size();
    }
  }

  T& front() { return buffer[head]; }

  T& back() { return buffer[(tail - 1 + buffer.size()) % buffer.size()]; }

  size_t get_size() { return size; }

  bool empty() const { return size == 0; }

  bool full() const { return size == buffer.size(); }

 private:
  std::vector<T> buffer;
  size_t head;
  size_t tail;
  size_t size;
};
