#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <tuple>

#include "fuzzer.h"

int classify_test();
int timeout_classify_test();

int main(int argc, char *argv[]) {
    int exit_code = 0;

    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " [test]\n";
        std::cerr << "Possible Tests: all, classify, timeout_classify\n";
        return 1;
    }
    
    std::string opt = std::string(argv[1]);

    if (opt == "all") {
        exit_code += classify_test();
        exit_code += timeout_classify_test();
        return exit_code;
    }
    else if (opt == "classify") {
        return classify_test();
    }
    else if (opt == "timeout_classify") {
        return timeout_classify_test();
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

int timeout_classify_test() {
    int exit_code = 0;
    struct TCTest {
        Verdict expected;
        RunResult runA;
        RunResult runB;
    };

    std::vector<TCTest> testcases = {
        {Verdict::Loop, {.verdict=Verdict::Timeout, .cycles=495, .retired=262, .exec_cycles=500}, {.verdict=Verdict::Timeout, .cycles=995, .retired=512, .exec_cycles=1000}},
        {Verdict::Hang, {.verdict=Verdict::Timeout, .cycles=495, .retired=32, .exec_cycles=500}, {.verdict=Verdict::Timeout, .cycles=995, .retired=32, .exec_cycles=1000}},
        {Verdict::Sleep, {.verdict=Verdict::Timeout, .cycles=39, .retired=33, .exec_cycles=500}, {.verdict=Verdict::Timeout, .cycles=39, .retired=33, .exec_cycles=1000}},
        {Verdict::Match, {.verdict=Verdict::Timeout, .cycles=495, .retired=262, .exec_cycles=500}, {.verdict=Verdict::Match, .cycles=83, .retired=42, .exec_cycles=87}},
    };
    // General testcases
    for (auto testcase : testcases) {
        RunResult v = timeout_classify(testcase.runA, testcase.runB);
        if(v.verdict != testcase.expected) {
            std::cout << "Error in timeout_classify(). Expected " << verdict_to_string(testcase.expected) << " but got " << verdict_to_string(v.verdict) << "\n";
            exit_code = 1;
        }
        if (testcase.expected == Verdict::Match && !v.bad_timeout) {
            std::cout << "Error in timeout_classify(). Expected bad_timeout to true.\n";
            exit_code = 1;
        }
    }
    // Exception tests
    RunResult runA = {.verdict=Verdict::Timeout, .cycles=495, .retired=262, .exec_cycles=500};
    try {
        timeout_classify(runA, {.verdict = Verdict::Unknown});
        std::cout << "Error in timeout_classify(). Expected error for verdict Unknown.\n";
        exit_code = 1;
    }
    catch (const std::runtime_error &e) {}
    
    try {
        timeout_classify(runA, {.verdict = Verdict::Timeout, .cycles=600, .exec_cycles=500});
        std::cout << "Error in timeout_classify(). Expected error for exec_cycles < cycles.\n";
        exit_code = 1;
    }
    catch (const std::runtime_error &e) {}

    return exit_code;
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
