#pragma once

#include <mutex>
#include <vector>

namespace qhenki::gfx
{
template<typename T> struct ThreadLocalResourceTracker
{
    std::mutex mutex;
    std::vector<T*> resources;
    void add(T* resource)
    {
        std::lock_guard lock(mutex);
        resources.push_back(resource);
    }
};
}; // namespace qhenki::gfx
