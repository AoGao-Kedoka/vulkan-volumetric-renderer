#include <fmt/format.h>
#include <filesystem>
#include "application.h"

//----------------------------------------------------------------------------------------
int main()
{
    auto pwd = std::filesystem::current_path();
    fmt::print("Current path is: {}\n", pwd.generic_string());

    try {
        Application app;
        app.init();
        app.run();
    } catch (const std::exception& e) {
        fmt::print("{}\n", e.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

//----------------------------------------------------------------------------------------