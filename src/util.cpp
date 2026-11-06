#include "util.h"

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
