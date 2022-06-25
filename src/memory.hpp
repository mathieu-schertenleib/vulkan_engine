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

    [[nodiscard]] bool
    do_is_equal(const std::pmr::memory_resource &other) const noexcept override;

private:
    std::pmr::memory_resource *m_upstream;
};

#endif // MEMORY_HPP
