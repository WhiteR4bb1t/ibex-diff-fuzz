#include <iostream>
#include <filesystem>
#include <sstream>
#include <fstream>
#include <cassert>
#include <stdexcept>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "run.h"
#include "classifier.h"

namespace fs = std::filesystem;

Runner::Runner(std::string folder, int LIMIT_1 = 1000, int LIMIT_2 = 2000) : folder_(folder), LIMIT_1_(LIMIT_1), LIMIT_2_(LIMIT_2) {
    proj_dir_ = fs::read_symlink("/proc/self/exe").parent_path().parent_path();
    ibex_simple_system_ = proj_dir_ / "build" / "lowrisc_ibex_ibex_simple_system_cosim_0" / "sim-verilator" / "Vibex_simple_system";
    link_ = proj_dir_ / "third_party" / "ibex" / "examples" / "sw" / "simple_system" / "common" / "link.ld";

    if (!fs::exists(proj_dir_) || !fs::is_directory(proj_dir_)) throw std::runtime_error("The folder " + proj_dir_.string() + " does not exist");
    if (!fs::exists(ibex_simple_system_)) throw std::runtime_error("Ibex could not be found in " + ibex_simple_system_.string());
    if (!fs::exists(link_)) throw std::runtime_error("The link file " + link_.string() + " does not exist");
}

RunResult Runner::run_one(std::string file, int id) {
    RunResult result;
    
    // Paths
    std::string id_str = std::to_string(id);
    fs::path run_dir = fs::path(folder_) / id_str;

    if (!fs::exists(run_dir) || !fs::is_directory(run_dir)) throw std::runtime_error("The folder " + run_dir.string() + " does not exist");

    // Assemble .elf
    run_gcc(run_dir, file); 
    // Run .elf file
    run_ibex(run_dir, file, LIMIT_1_, ibex_out_1_);
    
    std::ifstream f(run_dir / ibex_out_1_);
    if (!f) throw std::runtime_error("Failed to open file (" + ibex_out_1_ + ")");
    std::stringstream ss;
    ss << f.rdbuf();
    std::string log = ss.str();

    result = classify(log);

    if(result.verdict == Verdict::Timeout) {
        run_ibex(run_dir, file, LIMIT_2_, ibex_out_2_);

        std::ifstream f(run_dir / ibex_out_2_);
        if (!f) throw std::runtime_error("Failed to open file (" + ibex_out_2_ + ")");
        std::stringstream ss;
        ss << f.rdbuf();
        std::string log = ss.str();
        
        RunResult result_a = result;
        RunResult result_b = classify(log);
        result = timeout_classify(result_a, result_b);
    }

    return result;
}

void Runner::run_gcc(fs::path dir, std::string file) {
    fs::path asm_file = dir / file;
    fs::path elf_file = dir / file;
    asm_file += ".S";
    elf_file += ".elf";

    // riscv32-unknown-elf-gcc -march=rv32imc -mabi=ilp32 -nostdlib -nostartfiles  
    // -T third_party/ibex/examples/sw/simple_system/common/link.ld -o run/sleep.elf tests/control_sleep.S 
    pid_t pid = fork();
    if (pid < 0) {
        throw std::runtime_error("Error in forking for riscv32-unknown-elf-gcc");
    }
    if (pid == 0) { // child process
        fs::path log_file = dir / "riscv32-unknown-elf-gcc.out";
        // Mange fds
        int fd = open(log_file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0) _exit(127);
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);
        close(fd);

       char *args[] = {
            (char*)"riscv32-unknown-elf-gcc", 
            (char*)"-march=rv32imc", 
            (char*)"-mabi=ilp32", 
            (char*)"-nostdlib", 
            (char*)"-nostartfiles", 
            (char*)"-T", (char*)link_.c_str(), 
            (char*)"-o", (char*)elf_file.c_str(), 
            (char*)asm_file.c_str(), 
            nullptr};
        execvp("riscv32-unknown-elf-gcc", args);
        _exit(127);
    } else {
        int status;
        waitpid(pid, &status, 0);
        if(!WIFEXITED(status)) throw std::runtime_error("Error in execution of riscv32-unknown-elf-gcc");
    }

}

void Runner::run_ibex(fs::path dir, std::string file, int limit, std::string out) {
    // build/lowrisc_ibex_ibex_simple_system_cosim_0/sim-verilator/Vibex_simple_system --meminit=ram,match.elf -c 1000
    std::string limit_str = std::to_string(limit);

    // Fork
    pid_t pid = fork();
    if (pid < 0) {
        throw std::runtime_error("Error in forking for Vibex_simple_system");
    }
    if (pid == 0) { // child process
        chdir(dir.c_str());
        // Mange fds
        int fd = open(out.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0) _exit(127);
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);
        close(fd);

        // Build args
        std::string meminit_arg = "--meminit=ram," + file + ".elf";
        char *args[] = {
            (char*)ibex_simple_system_.c_str(), 
            meminit_arg.data(), 
            (char*)"-c", (char*)limit_str.data(), 
            nullptr};
        // Execute
        execvp(ibex_simple_system_.c_str(), args);
        _exit(127);
    } else {
        int status;
        waitpid(pid, &status, 0);
        if(!WIFEXITED(status)) throw std::runtime_error("Error in execution of Vibex_simple_system");
    }
}
