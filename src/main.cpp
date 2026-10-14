#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstdint>
#include <optional>

enum class Verdict { Match, Mismatch, Timeout, Unknown, Loop, Hang, Sleep, ElfError };

struct RunResult {
    Verdict verdict = Verdict::Unknown;
    std::optional<uint64_t> cycles;
    std::optional<uint64_t> retired;
    std::optional<uint64_t> exec_cycles;
};

RunResult classify(const std::string& log);
RunResult timeout_classify(const RunResult& a, const RunResult& b);
void print_result(std::ostream& os, const RunResult& result);
std::string verdict_to_string(Verdict v);

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

std::string verdict_to_string(Verdict v) {
    switch (v) {
        case Verdict::Hang:
            return "Hang";
        case Verdict::ElfError:
            return "ElfError";
        case Verdict::Loop:
            return "Loop";
        case Verdict::Match:
            return "Match";
        case Verdict::Mismatch:
            return "Mismatch";
        case Verdict::Sleep:
            return "Sleep";
        case Verdict::Timeout:
            return "Timeout";
        case Verdict::Unknown:
            return "Unknown";
    }
    return "<invalid>";
}  

void print_result(std::ostream& os, const RunResult& result) {
    std::string v = verdict_to_string(result.verdict);
    os << "Verdict: " << v << "\n";
    os << "Exec Cycles: " << (result.exec_cycles ? std::to_string(*result.exec_cycles) : "-") << "\n";
    os << "Cycles:      " << (result.cycles ? std::to_string(*result.cycles) : "-") << "\n";
    os << "Retired:     " << (result.retired ? std::to_string(*result.retired) : "-") << "\n";
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
