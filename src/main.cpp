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
        std::cerr << e.what() << '\n';
    }
    catch (...)
    {
        std::cerr << "Unknown exception thrown\n";
    }

    return EXIT_FAILURE;
}
