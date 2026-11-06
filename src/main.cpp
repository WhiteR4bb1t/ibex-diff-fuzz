#include <iostream>

#include "run.h"
#include "util.h"
#include "classifier.h"

int main(int argc, char *argv[]) {
    Runner *runner = new Runner("run", 1000, 2000);
    RunResult result = runner->run_one("control_sleep", 1);
    print_result(std::cout, result);
}
