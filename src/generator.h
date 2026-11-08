# pragma once

#include <string>
#include <random>
#include <cstdint>

enum class InstrFormat : uint8_t {
    R,
    I,
    S,
    U,
    B,
    J,
    COUNT
};

enum class Instr : uint8_t {
    // R
    ADD, SUB, SLL, SLT, SLTU, XOR, SRL, SRA, OR, AND,
    // I
    JALR, LB, LH, LW, LBU, LHU, ADDI, SLTI, SLTIU, XORI, ORI, ANDI, SLLI, SRLI, SRAI,
    // S
    SB, SH, SW,
    // U
    LUI, AUIPC,
    // B
    BEQ, BNE, BLT, BGE, BLTU, BGEU,
    // J
    JAL,

    COUNT
};

class Generator {
public:    
    Generator(std::string folder);
    int generate(std::string file, int id);
private:
    std::string instrToString(Instr instr) const;
    int getLabel(int i, int len, std::vector<int> &labels);
    constexpr InstrFormat getFormatOfInstruction(Instr instr) const;
    std::pair<InstrFormat, Instr> getRandomInstr();
    // Constants
    int MAX_LEN_ = 100;
    int MIN_LEN_ = 5;

    // Random devices
    std::minstd_rand gen_;
    std::uniform_int_distribution<int> len_distrib_;
    std::uniform_int_distribution<uint8_t> register_distrib_;
    std::uniform_int_distribution<uint8_t> instr_distrib_;
    
    std::string folder_;
};
