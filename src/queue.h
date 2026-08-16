/**
 * Software Rasterizer Playground.
 *
 * A thread safe queue.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include <mutex>

#include "containers/deque.h"

/** A thread safe queue. */
template<typename T>
class ThreadSafeQueue
{
    /** Queue mutex. */
    mutable std::mutex mutex;

    /** Underlying deque. */
    swr::deque<T> queue;

public:
    /** Constructors. */
    ThreadSafeQueue() = default;
    ThreadSafeQueue(const ThreadSafeQueue&) = delete;
    ThreadSafeQueue(ThreadSafeQueue&&) = delete;

    /** Assignment operators. */
    ThreadSafeQueue& operator=(const ThreadSafeQueue&) = delete;
    ThreadSafeQueue& operator=(ThreadSafeQueue&&) = delete;

    /** Return the queue's element count. */
    [[nodiscard]]
    std::size_t size() const
    {
        std::scoped_lock lock{mutex};
        return queue.size();
    }

    /** Returns whether the queue is empty. */
    [[nodiscard]]
    bool empty() const
    {
        std::scoped_lock lock{mutex};
        return queue.empty();
    }

    /** Clear the queue's contents. */
    void clear()
    {
        std::scoped_lock lock{mutex};
        queue.clear();
    }

    /** Push an element onto the queue. */
    void push_back(const T& elem)
    {
        std::scoped_lock lock{mutex};
        queue.push_back(elem);
    }

    /** Push an element onto the queue. */
    void push_back(T&& elem)
    {
        std::scoped_lock lock{mutex};
        queue.push_back(std::move(elem));
    }

    /** Emplace an element onto the queue. */
    template<typename... Args>
    void emplace_back(Args&&... args)
    {
        std::scoped_lock lock{mutex};
        queue.emplace_back(std::forward<Args>(args)...);
    }

    /**
     * Try to pop an element from the queue.
     *
     * @param elem Contains the element if successfully popped.
     * @returns Returns `true` if an element was popped, and `false` otherwise.
     */
    bool try_pop(T& elem)
    {
        std::scoped_lock lock{mutex};
        if(queue.empty())
        {
            return false;
        }

        elem = std::move(queue.front());
        queue.pop_front();
        return true;
    }

    /**
     * Returns the queue's contents and clears it.
     *
     * @returns Returns the queue's contents.
     */
    swr::deque<T> drain()
    {
        swr::deque<T> result;

        {
            std::scoped_lock lock{mutex};
            result.swap(queue);
        }

        return result;
    }
};
