# pragma once

#include <string>
#include <filesystem>

#include "classifier.h"

namespace fs = std::filesystem;

class Runner {
public:
    Runner(std::string folder, int LIMIT_1_, int LIMIT_2_);
    RunResult run_one(std::string file, int id);
private:
    void run_gcc(fs::path dir, std::string file);
    void run_ibex(fs::path dir, std::string file, int limit, std::string out);

    std::string folder_;
    fs::path proj_dir_; 
    fs::path ibex_simple_system_;
    fs::path link_;

    int LIMIT_1_;
    int LIMIT_2_;
    std::string ibex_out_1_ = "ibex_1.out";
    std::string ibex_out_2_ = "ibex_2.out";
};
