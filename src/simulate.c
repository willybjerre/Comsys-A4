#include <stdint.h>
#include <stdio.h>
#include "simulate.h"
#include "memory.h"
#include "disassemble.h"
#include "read_elf.h"   // for struct symbols

// --- Branch predictor state (Del 2) ---

// Tabelstørrelser til Bimodal og gShare (antal 2-bit tilstandsmaskiner)
static const int predictor_sizes[NUM_PRED_SIZES] = {256, 1024, 4096, 16384};

// 2-bit tilstandsmaskiner (vi bruger kun de nederste 2 bits)
static uint8_t bimodal_tables[NUM_PRED_SIZES][16384];
static uint8_t gshare_tables[NUM_PRED_SIZES][16384];

// Global History Register til gShare
static uint32_t ghr = 0;
static const int ghr_bits = 14; // nok til 16K entries (2^14)

// 2-bit counter: MSB bestemmer taget/ikke taget
static inline int counter_predict(uint8_t c) {
    // 00,01 -> 0 (ikke taget), 10,11 -> 1 (taget)
    return (c >> 1) & 1;
}

static inline uint8_t counter_update(uint8_t c, int taken) {
    if (taken) {
        if (c < 3) c++;
    } else {
        if (c > 0) c--;
    }
    return c;
}

static void init_predictors(void) {
    // Sæt alle counters til "svagt ikke taget" (01)
    for (int i = 0; i < NUM_PRED_SIZES; i++) {
        int size = predictor_sizes[i];
        for (int j = 0; j < size; j++) {
            bimodal_tables[i][j] = 1;
            gshare_tables[i][j]  = 1;
        }
    }
    ghr = 0;
}

// x0 må aldrig skrives til
static inline void write_reg(int32_t regs[32], uint32_t rd, int32_t value) {
    if (rd != 0) regs[rd] = value;
}

struct Stat simulate(struct memory *mem, int start_addr,
                     FILE *log_file, struct symbols* symbols)
{
    struct Stat stats = {0};

    // init branch predictors for hver simulering
    init_predictors();

    int32_t regs[32] = {0};       // x0..x31
    uint32_t pc = (uint32_t)start_addr;

    for (;;) {
        uint32_t inst = (uint32_t)memory_rd_w(mem, pc);
        stats.insns++;

        uint32_t opcode = inst & 0x7F;
        uint32_t rd     = (inst >> 7) & 0x1F;
        uint32_t funct3 = (inst >> 12) & 0x7;
        uint32_t rs1    = (inst >> 15) & 0x1F;
        uint32_t rs2    = (inst >> 20) & 0x1F;
        uint32_t funct7 = (inst >> 25) & 0x7F;

        uint32_t next_pc = pc + 4;
        
        if (log_file) {
            char buf[128];
            disassemble(pc, inst, buf, sizeof buf, symbols);
            fprintf(log_file, "%8ld  %08x : %08x   %s\n",
                    stats.insns, pc, inst, buf);
        }

        switch (opcode) {

        //R-type (RV32I + RV32M)
        case 0x33: {
            int32_t v1 = regs[rs1];
            int32_t v2 = regs[rs2];

            switch (funct3) {
            case 0x0: // ADD / SUB / MUL
                if (funct7 == 0x00) {        // add
                    write_reg(regs, rd, v1 + v2);
                } else if (funct7 == 0x20) { // sub
                    write_reg(regs, rd, v1 - v2);
                } else if (funct7 == 0x01) { // mul
                    write_reg(regs, rd, v1 * v2);
                }
                break;

            case 0x1: // SLL / MULH
                if (funct7 == 0x00) {        // sll
                    write_reg(regs, rd, v1 << (v2 & 0x1F));
                } else if (funct7 == 0x01) { // mulh
                    int64_t prod = (int64_t)v1 * (int64_t)v2;
                    write_reg(regs, rd, (int32_t)(prod >> 32));
                }
                break;

            case 0x2: // SLT / MULHSU
                if (funct7 == 0x00) {        // slt
                    write_reg(regs, rd, (v1 < v2) ? 1 : 0);
                } else if (funct7 == 0x01) { // mulhsu
                    int64_t a = (int64_t)v1;
                    uint64_t b = (uint32_t)v2;
                    int64_t prod = a * (int64_t)b;
                    write_reg(regs, rd, (int32_t)(prod >> 32));
                }
                break;

            case 0x3: // SLTU / MULHU
                if (funct7 == 0x00) {        // sltu
                    write_reg(regs, rd,
                              ((uint32_t)v1 < (uint32_t)v2) ? 1 : 0);
                } else if (funct7 == 0x01) { // mulhu
                    uint64_t prod =
                        (uint64_t)(uint32_t)v1 * (uint64_t)(uint32_t)v2;
                    write_reg(regs, rd, (int32_t)(prod >> 32));
                }
                break;

            case 0x4: // XOR / DIV
                if (funct7 == 0x00) {        // xor
                    write_reg(regs, rd, v1 ^ v2);
                } else if (funct7 == 0x01) { // div
                    if (v2 == 0)
                        write_reg(regs, rd, -1);
                    else
                        write_reg(regs, rd, v1 / v2);
                }
                break;

            case 0x5: // SRL / SRA / DIVU
                if (funct7 == 0x00) {        // srl
                    write_reg(regs, rd,
                              (int32_t)((uint32_t)v1 >> (v2 & 0x1F)));
                } else if (funct7 == 0x20) { // sra
                    write_reg(regs, rd, v1 >> (v2 & 0x1F));
                } else if (funct7 == 0x01) { // divu
                    if ((uint32_t)v2 == 0)
                        write_reg(regs, rd, -1);
                    else
                        write_reg(regs, rd,
                                  (int32_t)((uint32_t)v1 / (uint32_t)v2));
                }
                break;

            case 0x6: // OR / REM
                if (funct7 == 0x00) {        // or
                    write_reg(regs, rd, v1 | v2);
                } else if (funct7 == 0x01) { // rem
                    if (v2 == 0)
                        write_reg(regs, rd, v1);
                    else
                        write_reg(regs, rd, v1 % v2);
                }
                break;

            case 0x7: // AND / REMU
                if (funct7 == 0x00) {        // and
                    write_reg(regs, rd, v1 & v2);
                } else if (funct7 == 0x01) { // remu
                    if ((uint32_t)v2 == 0)
                        write_reg(regs, rd, v1);
                    else
                        write_reg(regs, rd,
                                  (int32_t)((uint32_t)v1 % (uint32_t)v2));
                }
                break;
            }
            break;
        }

        // LOADS (I-type, opcode 0x03)
        case 0x03: {
            int32_t imm = (int32_t)inst >> 20;
            uint32_t addr = (uint32_t)(regs[rs1] + imm);

            switch (funct3) {
            case 0x0: { // lb
                int8_t v = (int8_t)memory_rd_b(mem, addr);
                write_reg(regs, rd, v);
                break;
            }
            case 0x1: { // lh
                int16_t v = (int16_t)memory_rd_h(mem, addr);
                write_reg(regs, rd, v);
                break;
            }
            case 0x2: { // lw
                int32_t v = memory_rd_w(mem, addr);
                write_reg(regs, rd, v);
                break;
            }
            case 0x4: { // lbu
                uint8_t v = (uint8_t)memory_rd_b(mem, addr);
                write_reg(regs, rd, (int32_t)v);
                break;
            }
            case 0x5: { // lhu
                uint16_t v = (uint16_t)memory_rd_h(mem, addr);
                write_reg(regs, rd, (int32_t)v);
                break;
            }
            }
            break;
        }

        //  I-type ALU/SHIFTS (0x13)
        case 0x13: {
            int32_t imm = (int32_t)inst >> 20;
            uint32_t shamt = (inst >> 20) & 0x1F;
            int32_t v1 = regs[rs1];

            switch (funct3) {
            case 0x0: // addi
                write_reg(regs, rd, v1 + imm);
                break;
            case 0x2: // slti
                write_reg(regs, rd, (v1 < imm) ? 1 : 0);
                break;
            case 0x3: // sltiu
                write_reg(regs, rd,
                          ((uint32_t)v1 < (uint32_t)imm) ? 1 : 0);
                break;
            case 0x4: // xori
                write_reg(regs, rd, v1 ^ imm);
                break;
            case 0x6: // ori
                write_reg(regs, rd, v1 | imm);
                break;
            case 0x7: // andi
                write_reg(regs, rd, v1 & imm);
                break;

            case 0x1: // slli
                if (funct7 == 0x00)
                    write_reg(regs, rd, v1 << shamt);
                break;
            case 0x5: // srli / srai
                if (funct7 == 0x00)
                    write_reg(regs, rd,
                              (int32_t)((uint32_t)v1 >> shamt));
                else if (funct7 == 0x20)
                    write_reg(regs, rd, v1 >> shamt);
                break;
            }
            break;
        }

        //  STORES (S-type, 0x23) 
        case 0x23: {
            int32_t imm = ((inst >> 7) & 0x1F) | ((inst >> 25) << 5);
            if (imm & 0x800) imm |= 0xFFFFF000;
            uint32_t addr = (uint32_t)(regs[rs1] + imm);
            int32_t v2 = regs[rs2];

            switch (funct3) {
            case 0x0: // sb
                memory_wr_b(mem, addr, v2);
                break;
            case 0x1: // sh
                memory_wr_h(mem, addr, v2);
                break;
            case 0x2: // sw
                memory_wr_w(mem, addr, v2);
                break;
            }
            break;
        }

        //  BRANCHES (B-type, 0x63)
        case 0x63: {
            int32_t imm = 0;
            imm |= (inst >> 7) & 0x1E;          // imm[4:1]
            imm |= (inst >> 20) & 0x7E0;        // imm[10:5]
            imm |= (inst << 4) & 0x800;         // imm[11]
            imm |= (int32_t)inst >> 19 & 0x1000;// imm[12]
            if (imm & 0x1000) imm |= 0xFFFFE000;
            uint32_t target = pc + imm;

            int32_t v1 = regs[rs1], v2 = regs[rs2];
            int take = 0;

            switch (funct3) {
            case 0x0: take = (v1 == v2); break;                       // beq
            case 0x1: take = (v1 != v2); break;                       // bne
            case 0x4: take = (v1 <  v2); break;                       // blt
            case 0x5: take = (v1 >= v2); break;                       // bge
            case 0x6: take = ((uint32_t)v1 <  (uint32_t)v2); break;   // bltu
            case 0x7: take = ((uint32_t)v1 >= (uint32_t)v2); break;   // bgeu
            }

            // --- Branch prediction instrumentation (Del 2) ---
            {
                int actual_taken = take;
                int is_backward = (imm < 0);

                // Always Not Taken (NT)
                stats.nt.predictions++;
                if (actual_taken)
                    stats.nt.mispredictions++;

                // Backward Taken, Forward Not Taken (BTFNT)
                int btfnt_pred = is_backward ? 1 : 0;
                stats.btfnt.predictions++;
                if (btfnt_pred != actual_taken)
                    stats.btfnt.mispredictions++;

                // Bimodal + gShare for alle 4 størrelser
                uint32_t pc_index = pc >> 2;   // brug word-alignet PC
                uint32_t ghr_mask = (1u << ghr_bits) - 1;
                uint32_t ghr_local = ghr & ghr_mask;

                for (int i = 0; i < NUM_PRED_SIZES; i++) {
                    int size = predictor_sizes[i];
                    int mask = size - 1;   // alle størrelser er 2-potens

                    // ---- Bimodal ----
                    int idx = pc_index & mask;
                    uint8_t c = bimodal_tables[i][idx];
                    int pred = counter_predict(c);

                    stats.bimodal[i].predictions++;
                    if (pred != actual_taken)
                        stats.bimodal[i].mispredictions++;

                    bimodal_tables[i][idx] = counter_update(c, actual_taken);

                    // ---- gShare ----
                    int gidx = (int)((pc_index ^ ghr_local) & mask);
                    c = gshare_tables[i][gidx];
                    pred = counter_predict(c);

                    stats.gshare[i].predictions++;
                    if (pred != actual_taken)
                        stats.gshare[i].mispredictions++;

                    gshare_tables[i][gidx] = counter_update(c, actual_taken);
                }

                // Opdater global history til gShare
                ghr = ((ghr << 1) | (actual_taken ? 1u : 0u)) & ((1u << ghr_bits) - 1);
            }
           

            if (take) next_pc = target;
            break;
        }

        //  AUIPC / LUI
        case 0x17: { // auipc
            int32_t imm = (int32_t)(inst & 0xFFFFF000);
            write_reg(regs, rd, pc + imm);
            break;
        }
        case 0x37: { // lui
            int32_t imm = (int32_t)(inst & 0xFFFFF000);
            write_reg(regs, rd, imm);
            break;
        }

        //  JAL / JALR
        case 0x6F: { // jal
            int32_t imm = 0;
            imm |= (inst & 0xFF000);            // 19:12
            imm |= (inst >> 9) & 0x800;         // 11
            imm |= (inst >> 20) & 0x7FE;        // 10:1
            imm |= (inst >> 11) & 0x100000;     // 20
            if (imm & 0x100000) imm |= 0xFFE00000;

            write_reg(regs, rd, pc + 4);
            next_pc = pc + imm;
            break;
        }

        case 0x67: { // jalr
            int32_t imm = (int32_t)inst >> 20;
            uint32_t target = (uint32_t)(regs[rs1] + imm) & ~1u;
            write_reg(regs, rd, pc + 4);
            next_pc = target;
            break;
        }

        // SYSTEM / ECALL 
        case 0x73: {
            if (inst == 0x00000073) { // ecall
                int32_t a7 = regs[17]; // syscall nr
                int32_t a0 = regs[10];

                if (a7 == 1) {          // getchar
                    int c = getchar();
                    if (c == EOF) c = -1;
                    write_reg(regs, 10, c);
                } else if (a7 == 2) {   // putchar
                    putchar(a0 & 0xFF);
                    fflush(stdout);
                } else if (a7 == 3 || a7 == 93) { // exit
                    return stats;
                }
            }
            break;
        }

        default:
            fprintf(stderr, "Unknown instruction %08x at %08x\n", inst, pc);
            return stats;
        }

        regs[0] = 0;   // x0 er altid 0
        pc = next_pc;
    }
}