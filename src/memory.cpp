#include "memory.hpp"

#include <iostream>

Debug_memory_resource::Debug_memory_resource(
    std::pmr::memory_resource *upstream)
    : m_upstream {upstream}
{
}

void *Debug_memory_resource::do_allocate(std::size_t bytes,
                                         std::size_t alignment)
{
    std::cout << "do_allocate " << bytes << " bytes\n";

    return m_upstream->allocate(bytes, alignment);
}

void Debug_memory_resource::do_deallocate(void *ptr,
                                          std::size_t bytes,
                                          std::size_t alignment)
{
    std::cout << "do_deallocate " << bytes << " bytes\n";

    m_upstream->deallocate(ptr, bytes, alignment);
}

bool Debug_memory_resource::do_is_equal(
    const std::pmr::memory_resource &other) const noexcept
{
    return this == &other;
}