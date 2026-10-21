#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <tuple>

#include "fuzzer.h"

int classify_test();

int main(int argc, char *argv[]) {
    int exit_code = 0;

    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " [test]\n";
        std::cerr << "Possible Tests: all, classify\n";
        return 1;
    }
    
    std::string opt = std::string(argv[1]);

    if (opt == "all") {
        exit_code += classify_test();
        return exit_code;
    }
    else if (opt == "classify") {
        return classify_test();
    }
    else {
        std::cerr << "Test '" << opt << "' does not exit\n";
        return 1;
    }
}

bool compare_result(RunResult a, RunResult b) {
    if(!(a.cycles == b.cycles) || !(a.exec_cycles == b.exec_cycles) || !(a.retired == b.retired) || !(a.verdict == b.verdict))
        return false;
    return true;
}

int classify_test() {
    int exit_code = 0;

    std::string base_path = "tests/logs/";
    std::vector<std::tuple<std::string, RunResult>> testcases = {
        {"elf_error.txt", {.verdict=Verdict::ElfError}},
        {"invalid.txt", {.verdict=Verdict::Unknown, .exec_cycles=87}},
        {"loop.txt", {.verdict=Verdict::Timeout, .cycles=495, .retired=262, .exec_cycles=500}},
        {"match.txt", {.verdict=Verdict::Match, .cycles=83, .retired=42, .exec_cycles=87}},
        {"mismatch.txt", {.verdict=Verdict::Mismatch, .exec_cycles=44}},
        {"sleep.txt", {.verdict=Verdict::Timeout, .cycles=39, .retired=33, .exec_cycles=500}},
    };

    for (auto testcase : testcases) {
        std::string path = base_path + get<0>(testcase);
        std::ifstream f(path);

        if (!f) {
            std::cerr << "Failed to open file (" << path << ")\n";
            return 1;
        }
        std::stringstream ss;
        ss << f.rdbuf();
        std::string log = ss.str();
        RunResult result = classify(log);

        if (!compare_result(get<1>(testcase), result)) {
            std::cout << "Error while classifying " << path << "\n\nExpected:\n";
            print_result(std::cout, get<1>(testcase));
            std::cout << "\nResult:\n";
            print_result(std::cout, result);
            exit_code = 1;
        }

    }

    return exit_code;
}
