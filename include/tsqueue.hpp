// copied from https://github.com/Konstantin-2/misc/blob/master/tsqueue.h
#pragma once

#include <atomic>
#include <cassert>
#include <condition_variable>
#include <optional>
#include <queue>
#include <utility>

template <typename T> struct Tsqueue
{
    inline Tsqueue(size_t max_size = -1UL)
        : max_size_{max_size}, end_{false} {};

    void push(const T &);
    void push(T &&);

    void close();

    std::optional<T> pop();

  private:
    std::queue<T> queue_;
    std::mutex mutex_;
    std::condition_variable cv_empty_, cv_full_;
    const size_t max_size_;
    std::atomic<bool> end_;
};

template <typename T> void Tsqueue<T>::push(T &&t)
{
    std::unique_lock<std::mutex> lck(mutex_);
    while (queue_.size() == max_size_ && !end_)
        cv_full_.wait(lck);
    assert(!end_);
    queue_.push(std::move(t));
    cv_empty_.notify_one();
}

template <typename T> void Tsqueue<T>::push(T const &t)
{
    std::unique_lock<std::mutex> lck(mutex_);
    while (queue_.size() == max_size_ && !end_)
        cv_full_.wait(lck);
    assert(!end_);
    queue_.push(std::move(t));
    cv_empty_.notify_one();
}

template <typename T> std::optional<T> Tsqueue<T>::pop()
{
    std::unique_lock<std::mutex> lck(mutex_);
    while (queue_.empty() && !end_)
        cv_empty_.wait(lck);
    T t = std::move(queue_.front());
    queue_.pop();
    cv_full_.notify_one();
    return t;
}

template <typename T> void Tsqueue<T>::close()
{
    end_ = true;
    std::lock_guard<std::mutex> lck(mutex_);
    cv_empty_.notify_one();
    cv_full_.notify_one();
}
