#pragma once

#include <cstdint>
#include <optional>
#include <string>

enum class Verdict { Match, Mismatch, Timeout, Unknown, Loop, Hang, Sleep, ElfError };

struct RunResult {
    Verdict verdict = Verdict::Unknown;
    std::optional<uint64_t> cycles;
    std::optional<uint64_t> retired;
    std::optional<uint64_t> exec_cycles;
    bool bad_timeout = false;
};

RunResult classify(const std::string& log);
RunResult timeout_classify(const RunResult& a, const RunResult& b);

