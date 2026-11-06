#include <sstream>
#include <cassert>
#include <stdexcept>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "classifier.h"
#include "util.h"

RunResult classify(const std::string& log) {
    RunResult result;

    if (log.find("Terminating simulation by software request") != std::string::npos)
        result.verdict = Verdict::Match;
    else if (log.find("Co-simulation mismatch") != std::string::npos)
        result.verdict = Verdict::Mismatch;
    else if (log.find("Simulation timeout of") != std::string::npos)
        result.verdict = Verdict::Timeout;
    else if (log.find("Failed to load ELF") != std::string::npos)
        result.verdict = Verdict::ElfError;
    else
        result.verdict = Verdict::Unknown;

    std::istringstream in(log); 
    std::string line;
    std::string delimiter = ":";
    try {
        while (std::getline(in, line)) {
            if (line.starts_with("Executed cycles:"))
                result.exec_cycles = std::stoull(line.substr(line.find(delimiter) + 1));
            else if (line.starts_with("Cycles:"))
                result.cycles = std::stoull(line.substr(line.find(delimiter) + 1));
            else if (line.starts_with("Instructions Retired:"))
                result.retired = std::stoull(line.substr(line.find(delimiter) + 1));
        }
    } catch (const std::exception&) {
        result.verdict = Verdict::Unknown;
    }

    return result;
}

RunResult timeout_classify(const RunResult &a, const RunResult &b) {
    assert(a.verdict == Verdict::Timeout && "No timeout in Result a");
    RunResult result = b;
    if (b.verdict != Verdict::Timeout) {
        if (b.verdict == Verdict::Unknown || b.verdict == Verdict::ElfError)
            throw std::runtime_error("Error in timeout classification. B has verdict " + verdict_to_string(b.verdict));
        result.bad_timeout = true;

        return result;
    }

    if(!b.exec_cycles || !b.cycles || !b.retired || !a.retired || !a.cycles || !a.exec_cycles)
        throw std::runtime_error("Error in timeout classification. Statistics are missing");

    if(b.exec_cycles < b.cycles)
        throw std::runtime_error("Error in timeout classification. Executed cycles smaller than cycles");
    
    if(b.exec_cycles.value() - b.cycles.value() <= 5) {
        // Loop or Hang
        if(a.retired.value() == b.retired.value()) 
            result.verdict = Verdict::Hang;
        else
            result.verdict = Verdict::Loop;
    }
    else {
        // Sleep
        result.verdict = Verdict::Sleep;
    }

    return result;
}
