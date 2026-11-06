# pragma once

#include <string>
#include <ostream>

#include "classifier.h"

void print_result(std::ostream& os, const RunResult& result);
std::string verdict_to_string(Verdict v);
