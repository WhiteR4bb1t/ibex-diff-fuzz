#include "generator.h"
#include "wrapper_text.h"
#include "error.h"

#include <filesystem>
#include <vector>
#include <algorithm>
#include <cstdio>
#include <iostream>

namespace fs = std::filesystem;

Generator::Generator(std::string folder) 
    : gen_(std::random_device{}()), folder_(folder) {
    
    len_distrib_ = std::uniform_int_distribution<int>(MIN_LEN_, MAX_LEN_);
    register_distrib_ = std::uniform_int_distribution<uint8_t>(0, 31);
    instr_distrib_ = std::uniform_int_distribution<uint8_t>(0, static_cast<int>(Instr::COUNT) - 1);
}

constexpr InstrFormat Generator::getFormatOfInstruction(Instr instr) const {
    using enum Instr;
    switch(instr) {
        case ADD: case SUB: case SLL: case SLT: case SLTU: case XOR: case SRL: case SRA: case OR: case AND:
            return InstrFormat::R;
        case JALR: case LB: case LH: case LW: case LBU: case LHU: case ADDI: case SLTI: case SLTIU: case XORI: case ORI: case ANDI: case SLLI: case SRLI: case SRAI:
            return InstrFormat::I;
        case SB: case SH: case SW:
            return InstrFormat::S;
        case LUI: case AUIPC:
            return InstrFormat::U;
        case BEQ: case BNE: case BLT: case BGE: case BLTU: case BGEU:
            return InstrFormat::B;
        case JAL:
            return InstrFormat::J; 
        default:
            return InstrFormat::COUNT;
    }
}

std::pair<InstrFormat, Instr> Generator::getRandomInstr() {
    Instr chosen_instr = static_cast<Instr>(instr_distrib_(gen_));
    InstrFormat chosen_format = getFormatOfInstruction(chosen_instr);
    return {chosen_format, chosen_instr};
}

int Generator::getLabel(int i, int len, std::vector<int> &labels) {
    std::uniform_int_distribution<int> label_index_distrib(i + 1, len - 1);
    int label_index = label_index_distrib(gen_);

    auto it = std::ranges::find(labels, label_index);
    if(it != labels.end()) {
        int label = std::distance(labels.begin(), it);
        return label;
    }

    labels.push_back(label_index);
    return labels.size() - 1;
}


int Generator::generate(std::string file, int id) {
    std::string id_str = std::to_string(id);
    fs::path run_dir = fs::path(folder_) / id_str;
    fs::path asm_file = run_dir / file;
    asm_file += ".S";
    
    if (!fs::exists(run_dir) || !fs::is_directory(run_dir)) throw RunError("The folder " + run_dir.string() + " does not exist");

    int len = len_distrib_(gen_);
    
    std::string prog;
    prog.append(wrapper_start_);

    // Build asm file
    std::string rs1, rs2, rd, imm, offset, label;
    int instr_count = 0;

    std::vector<int> labels;

    for (int i = 0; i < len; i++) {
        using enum Instr;

        auto it = std::ranges::find(labels, i);
        if(it != labels.end()) {
            label = std::to_string(std::distance(labels.begin(), it));
            prog.append(label + ":\n");
            continue;
        }

        // This fixes the bug that a jump happens on the last instruction and cant have a label
        if (i == (len - 1)) break;

        // Get random instruction
        auto [format, instr] = getRandomInstr();
        // Get random registers
        rs1 = "x" + std::to_string(register_distrib_(gen_));
        rs2 = "x" + std::to_string(register_distrib_(gen_));
        rd = "x" + std::to_string(register_distrib_(gen_));

        switch(format) {
            case InstrFormat::B: {
                label = std::to_string(getLabel(i, len, labels));
                prog.append(instrToString(instr) + " " + rs1 + ", " + rs2 + ", " + label + "f\n");

                instr_count += 1;
                break;
            }
            case InstrFormat::I: {
                if(instr == JALR) {
                    // Create label and jump to it with jalr
                    label = std::to_string(getLabel(i, len, labels));
                    prog.append("lla " + rs1 + ", " + label + "f\n");
                    prog.append("jalr " + rd + ", 0(" + rs1 + ")\n");
                    instr_count += 3;
                } else if(instr == LB || instr == LH || instr == LW || instr == LBU || instr == LHU) {
                    // Load instruction
                    std::uniform_int_distribution<uint32_t> address_distrib(0x110000, 0x1FFFFF0);
                    uint32_t address = address_distrib(gen_);
                    int imm_int = address >> 12;
                    int offset_int = address & 0xFFF;
                    if (offset_int >= 2048) {
                        offset_int -= 4096;
                        imm_int++;
                    }
                    imm = std::to_string(imm_int);
                    offset = std::to_string(offset_int);

                    prog.append("lui " + rs1 + ", " + imm + "\n");
                    prog.append(instrToString(instr) + " " + rd + ", " + offset + "(" + rs1 + ")\n");
                     
                    instr_count += 2;
                } else if (instr == SLLI || instr == SRLI || instr == SRAI) {
                    // Shift instruction
                    std::uniform_int_distribution<int> shift_distrib(0, 31);
                    imm = std::to_string(shift_distrib(gen_));
                    prog.append(instrToString(instr) + " " + rd + ", " + rs1 + ", " + imm + "\n");
                    
                    instr_count += 1;
                } else {
                    // Get a value from -2^11 to 2^11 - 1
                    std::uniform_int_distribution<int> imm_distrib(-(1 << 11), (1 << 11) - 1);
                    imm = std::to_string(imm_distrib(gen_));
                    prog.append(instrToString(instr) + " " + rd + ", " + rs1 + ", " + imm + "\n");

                    instr_count += 1;
                }
                break;
            }
            case InstrFormat::J: {
                // JAL only
                label = std::to_string(getLabel(i, len, labels));
                prog.append(instrToString(instr) + " " + rd + ", " + label + "f\n");

                instr_count += 1;
                break;
            }
            case InstrFormat::U: {
                std::uniform_int_distribution<int> imm_distrib(0, (1<<20) - 1);
                imm = std::to_string(imm_distrib(gen_));
                prog.append(instrToString(instr) + " " + rd + ", " + imm + "\n");
                instr_count += 1;
                break;
            }
            case InstrFormat::R: {
                prog.append(instrToString(instr) + " " + rd + ", " + rs1 + ", " + rs2 + "\n");
                instr_count += 1;
                break;
            }
            case InstrFormat::S: {
                // Stores
                std::uniform_int_distribution<uint32_t> address_distrib(0x110000, 0x1FFFFF0);
                uint32_t address = address_distrib(gen_);
                int imm_int = address >> 12;
                int offset_int = address & 0xFFF;
                if (offset_int >= 2048) {
                    offset_int -= 4096;
                    imm_int++;
                }
                imm = std::to_string(imm_int);
                offset = std::to_string(offset_int);
                prog.append("lui " + rs1 + ", " + imm + "\n");
                prog.append(instrToString(instr) + " " + rs2 + ", " + offset + "(" + rs1 + ")\n");
                 
                instr_count += 2;
                break;
            }
            default: break;
        }
    }

    prog.append(wrapper_end_);

    std::FILE* f = std::fopen(asm_file.c_str(), "wb");
    if (!f) throw RunError("Could not open file (" + asm_file.string() + ")");

    std::fwrite(prog.data(), 1, prog.size(), f);
    std::fclose(f);

    // Instruction count + wrapper instructions
    return instr_count;
}

std::string Generator::instrToString(Instr instr) const {
    using enum Instr;

    switch(instr) {
        case ADD: return "add";
        case SUB: return "sub";
        case SLL: return "sll";
        case SLT: return "slt";
        case SLTU: return "sltu";
        case XOR: return "xor";
        case SRL: return "srl";
        case SRA: return "sra";
        case OR: return "or";
        case AND: return "and";
        case JALR: return "jalr";
        case LB: return "lb";
        case LH: return "lh";
        case LW: return "lw";
        case LBU: return "lbu";
        case LHU: return "lhu";
        case ADDI: return "addi";
        case SLTI: return "slti";
        case SLTIU: return "sltiu";
        case XORI: return "xori";
        case ORI: return "ori";
        case ANDI: return "andi";
        case SLLI: return "slli";
        case SRLI: return "srli";
        case SRAI: return "srai";
        case SB: return "sb";
        case SH: return "sh";
        case SW: return "sw";
        case LUI: return "lui";
        case AUIPC: return "auipc";
        case BEQ: return "beq";
        case BNE: return "bne";
        case BLT: return "blt";
        case BGE: return "bge";
        case BLTU: return "bltu";
        case BGEU: return "bgeu";
        case JAL: return "jal";
        default: return "Unknown Instruction";
    }
}
