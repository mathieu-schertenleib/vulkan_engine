#ifndef MEMORY_HPP
#define MEMORY_HPP

#include <memory_resource>

class Debug_memory_resource : public std::pmr::memory_resource
{
public:
    explicit Debug_memory_resource(
        std::pmr::memory_resource *upstream = std::pmr::get_default_resource());

    [[nodiscard]] void *do_allocate(std::size_t bytes,
                                    std::size_t alignment) override;

    void
    do_deallocate(void *ptr, std::size_t bytes, std::size_t alignment) override;

    [[nodiscard]] constexpr bool
    do_is_equal(const std::pmr::memory_resource &other) const noexcept override
    {
        return this == &other;
    }

    [[nodiscard]] constexpr std::size_t usage() const noexcept
    {
        return m_total_allocated - m_total_deallocated;
    }

private:
    std::pmr::memory_resource *m_upstream {};
    std::size_t m_total_allocated {};
    std::size_t m_total_deallocated {};
};

#endif // MEMORY_HPP
