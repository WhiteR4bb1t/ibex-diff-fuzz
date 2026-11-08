# pragma once

#include <source_location>
#include <stdexcept>

class FuzzError : public std::runtime_error {
public:
    FuzzError(const std::string& msg,
              std::source_location loc = std::source_location::current())
        : std::runtime_error(msg), loc_(loc) {}
    const std::source_location& where() const { return loc_; }
private:
    std::source_location loc_;
};

class FatalError : public FuzzError { using FuzzError::FuzzError; };   // Fatal error for the camapin
class RunError   : public FuzzError { using FuzzError::FuzzError; };   // Only error in this run
