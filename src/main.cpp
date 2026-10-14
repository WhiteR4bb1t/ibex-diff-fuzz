#include <iostream>
#include <sstream>
#include <string>

enum class Verdict { Match, Mismatch, Timeout, Unknown, Loop, Hang, Sleep, Error };

struct RunResult {
    Verdict verdict;
    int cycles;
    int retired;
    int exec_cycles;
};

RunResult classify(const std::string& log);
RunResult timeout_classify(const RunResult& a, const RunResult& b);

int main(int argc, char *argv[]) {
    std::string test_log = "Terminating simulation by software reques";
    RunResult result = classify(test_log);
}

RunResult classify(const std::string& log) {
    RunResult result;
    
    if (log.find("Terminating simulation by software request") != std::string::npos)
        result.verdict = Verdict::Match;
    else if (log.find("Co-simulation mismatch") != std::string::npos)
        result.verdict = Verdict::Mismatch;
    else if (log.find("Simulation timeout of") != std::string::npos)
        result.verdict = Verdict::Timeout;
    else if (log.find("Failed to load ELF") != std::string::npos)
        result.verdict = Verdict::Error;

    std::istringstream in(log); 
    std::string line;
    std::string delimiter = ":";
    while (std::getline(in, line)) {
        if (line.starts_with("Executed cycles:"))
            result.exec_cycles = std::stoull(line.substr(line.find(delimiter) + 1));
        else if (line.starts_with("Cycles:"))
            result.cycles = std::stoull(line.substr(line.find(delimiter) + 1));
        else if (line.starts_with("Instructions Retired:"))
            result.retired = std::stoull(line.substr(line.find(delimiter) + 1));
    }
    
    return result;
}
