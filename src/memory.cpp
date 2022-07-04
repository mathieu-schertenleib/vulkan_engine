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
    m_total_allocated += bytes;

    std::cout << "Allocating " << bytes
              << " bytes; total = " << m_total_allocated - m_total_deallocated
              << " bytes\n";

    return m_upstream->allocate(bytes, alignment);
}

void Debug_memory_resource::do_deallocate(void *ptr,
                                          std::size_t bytes,
                                          std::size_t alignment)
{
    m_total_allocated -= bytes;

    std::cout << "Deallocating " << bytes
              << " bytes; total = " << m_total_allocated - m_total_deallocated
              << " bytes\n";

    m_upstream->deallocate(ptr, bytes, alignment);
}
