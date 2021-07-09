#include <iostream>
#include "demo.hpp"

int main() {
    Demo demo;

    try {
        demo.initialize();
    } catch (std::exception &ex) {
        std::cerr << "Failed to load engine: " << ex.what() << std::endl;
        return 1;
    }

    demo.run();
    return 0;
}
