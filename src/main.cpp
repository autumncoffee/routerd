#include <routerd_lib/main.hpp>
#include <iostream>

int main(int argc, const char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " /path/to/config.json" << std::endl;
        return 1;
    }

    return NAC::RouterDMain(argv[1]);
}
