#include "application.hpp"

#include <cstdlib>
#include <iostream>

int main()
{
    try
    {
        Application app;
        app.run();
        return EXIT_SUCCESS;
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
    }
    catch (...)
    {
        std::cerr << "Unknown exception thrown" << std::endl;
    }

    return EXIT_FAILURE;
}
