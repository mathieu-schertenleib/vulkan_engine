#include "memory.hpp"

#include <cstdlib>
#include <iostream>
#include <memory>

namespace
{

Debug_memory_resource memory_resource;

template <typename T>
auto make_unique()
{
    auto deleter = [](T *pointer)
    {
        pointer->~T();
        memory_resource.deallocate(pointer, sizeof(T), alignof(T));
    };
    return std::unique_ptr<T, decltype(deleter)>(
        new (memory_resource.allocate(sizeof(T), alignof(T))) T {}, deleter);
}

} // namespace

struct S
{
    S()
    {
        std::cout << "S()\n";
    }
    ~S()
    {
        std::cout << "~S()\n";
    }

    char data[1024] {};
};

int main()
{

    const auto p = make_unique<S>();

    return EXIT_SUCCESS;
}