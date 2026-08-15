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

template<typename T>
class ThreadSafeQueue
{
    mutable std::mutex mutex;
    swr::deque<T> queue;

public:
    ThreadSafeQueue() = default;

    std::size_t qsize() const
    {
        std::scoped_lock lock{mutex};
        return queue.size();
    }

    bool empty() const
    {
        return qsize() == 0;
    }

    void clear()
    {
        std::scoped_lock lock{mutex};
        queue.clear();
    }

    void push_back(const T& elem)
    {
        std::scoped_lock lock{mutex};
        queue.push_back(elem);
    }

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

    template<typename F>
    void for_each(F&& f)
    {
        for(auto it = queue.begin(); it != queue.end();)
        {
            if(f(*it))
            {
                it = queue.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }
};
