#include <iostream>
#include <fstream>
#include <sstream>

#include "fuzzer.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "Give filepath\n";
        return 1;
    }
    std::ifstream f(argv[1]);
    if (!f) {
        std::cerr << "Failed to open file (" << argv[1] << ")\n";
        return 1;
    }
    std::stringstream ss;
    ss << f.rdbuf();
    std::string log = ss.str();

    RunResult result = classify(log);

    print_result(std::cout, result);
}
