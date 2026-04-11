/**
 * @file main.cpp
 * @brief Cura - Photo Deduplication Tool entry point
 */

#include "ui/cura_app.hpp"
#include <slint.h>

int main(int argc, char* argv[]) {
    cura::CuraApp app;

    // Run the Slint UI
    app.run();

    return 0;
}