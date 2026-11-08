#include <iostream>
#include <filesystem>
#include <string>
#include <array>

#include "run.h"
#include "util.h"
#include "classifier.h"
#include "generator.h"
#include "loop.h"
#include "error.h"

namespace fs = std::filesystem;

#define WRAPPER_INSTR_SIZE 37

int loop(int argc, char *argv[]) {
    int actual_instruction_count, retired;
    int exec_percentage, measured = 0, average_percentage = 0, percentage_sum = 0;

    int error_count = 0;

    std::string id_str;
    int run_length = 1000;

    fs::path curr_dir;
    std::string prog_name = "prog";
    fs::path run_dir = "run";
    std::error_code ec;

    bool first_draw = true;

    if (!fs::exists(run_dir)) {
        if (!fs::create_directory(run_dir)) {
            std::cerr << "Could not create directory " << run_dir << "\n";
            return 1;
        }
    }

    Generator *generator;
    Runner *runner;
    try {
        generator = new Generator(run_dir);
        runner = new Runner(run_dir, 10000, 20000);
    }
    catch (const FatalError& e) {
        std::cerr << e.where().function_name() << ": " << e.what() << "\n";
        return 1;
    }
    catch (const std::exception& e) {
        std::cerr << "std: " << e.what() << "\n";
        return 1;
    }
    std::array<int, VERDICT_COUNT> verdict_counter = {0};

    // Fuzz loop
    for(int i = 1; i <= run_length; i++) {
        RunResult result;

        id_str = std::to_string(i);
        curr_dir = run_dir / id_str;
        if(!fs::exists(curr_dir)) {
            if (!fs::create_directory(curr_dir)) {
                std::cerr << "Could not create directory " << run_dir << "\n";
                continue;
            }
        }
        try {
            actual_instruction_count = generator->generate(prog_name, i);
            result = runner->run_one(prog_name, i);
        }
        catch (const RunError& e) {
            std::cerr << "\033[31m(ID " << i << ") " << e.where().function_name() << ": " << e.what() << "\033[0m\n";
            error_count++;
            continue;
        }
        catch (const FatalError& e) {
            std::cerr << "(ID " << i << ") " << e.where().function_name() << ": " << e.what() << "\n";
            return 1;
        }
        catch (const std::exception& e) {
            std::cerr << "(ID " << i << ") std: " << e.what() << "\n";
            return 1;
        }
       
        // Modify lists depending on Verdict
        if(result.verdict == Verdict::Match) {
            verdict_counter[static_cast<int>(Verdict::Match)]++;
            fs::remove_all(curr_dir, ec);
        } else {
            switch(result.verdict) {
                // Display elf error wihich is probably an error in the compiler
                case Verdict::ElfError: {
                    verdict_counter[static_cast<int>(Verdict::ElfError)]++;
                    std::cout << "ID: " << i << "\n";
                    std::cout << "\033[31m" << "ElfError in ID " << i << "\033[0m" << "\n";
                    break;
                }
                case Verdict::Mismatch: {
                    verdict_counter[static_cast<int>(Verdict::Mismatch)]++;
                    break;
                }
                case Verdict::Hang: {
                    verdict_counter[static_cast<int>(Verdict::Hang)]++;
                    break;
                }
                case Verdict::Loop: {
                    verdict_counter[static_cast<int>(Verdict::Loop)]++;
                    break;
                }
                case Verdict::Sleep: {
                    verdict_counter[static_cast<int>(Verdict::Sleep)]++;
                    break;
                }
                case Verdict::Unknown: {
                    verdict_counter[static_cast<int>(Verdict::Unknown)]++;
                    break;
                }
                default: break;
            }
        }

        // Edit retired and calculate execution percentage
        if(result.retired) {
            retired = result.retired.value();
            exec_percentage = ((retired - WRAPPER_INSTR_SIZE) * 100) / actual_instruction_count;
            measured++;
            percentage_sum += exec_percentage;
            average_percentage = percentage_sum / measured;
        }

        // Display Live Display
        if (!first_draw) std::cout << "\033[2A";
        first_draw = false;
        std::cout << "\r\033[2KRun [" << i << "/" << run_length << "]\n"
                << "\033[2K" << verdict_counter[static_cast<int>(Verdict::Mismatch)] << " Mismatches, "
                << verdict_counter[static_cast<int>(Verdict::Hang)] << " Hangs, "
                << verdict_counter[static_cast<int>(Verdict::Loop)] << " Loops, "
                << verdict_counter[static_cast<int>(Verdict::Sleep)] << " Sleeps, "
                << verdict_counter[static_cast<int>(Verdict::Unknown)] << " Unknowns, "
                << verdict_counter[static_cast<int>(Verdict::ElfError)] << " ElfErrors, "
                << "exec ~ " << average_percentage << "%\n"
                << "\033[31mErrors: " << error_count << "\033[0m"
                << std::flush;
    }

    std::cout << "\n";

    // Dispaly end statistics

    return 0;
}
