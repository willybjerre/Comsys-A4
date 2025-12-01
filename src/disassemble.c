#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include "disassemble.h"

void disassemble(uint32_t addr, uint32_t instruction, char* result, size_t buf_size, struct symbols* symbols){

    uint32_t opcode = instruction & 0x7F;
    uint32_t rd = (instruction >> 7) & 0x1F;
    uint32_t funct3 = (instruction >> 12) & 0x7;
    uint32_t rs1 = (instruction >> 15) & 0x1F;
    uint32_t rs2 = (instruction >> 20) & 0x1F;
    uint32_t funct7 = (instruction >> 25) & 0x7F;
    

    switch (opcode)
    {
        case 0x33:{ // R-type 
            switch (funct3)
            {
                case 0x0:
                    if (funct7 == 0x00)
                        snprintf(result, buf_size, "add x%d, x%d, x%d", rd, rs1, rs2);
                    else if (funct7 == 0x20)
                        snprintf(result, buf_size, "sub x%d, x%d, x%d", rd, rs1, rs2);
                    else if (funct7 == 0x01)
                        snprintf(result, buf_size, "mul x%d, x%d, x%d", rd, rs1, rs2);
                    break;
                
                case 0x1:
                    if (funct7 == 0x00)
                        snprintf(result, buf_size, "sll x%d, x%d, x%d", rd, rs1, rs2);     
                    else if (funct7 == 0x01)
                        snprintf(result, buf_size, "mulh x%d, x%d, x%d", rd, rs1, rs2);
                    break;
                
                case 0x2:
                    if (funct7 == 0x00)
                        snprintf(result, buf_size, "slt x%d, x%d, x%d", rd, rs1, rs2);
                    else if (funct7 == 0x01)
                        snprintf(result, buf_size, "mulhsu x%d, x%d, x%d", rd, rs1, rs2);
                    break;
                
                case 0x3:
                    if (funct7 == 0x00)
                        snprintf(result, buf_size, "sltu x%d, x%d, x%d", rd, rs1, rs2);
                    else if (funct7 == 0x01)
                        snprintf(result, buf_size, "mulhu x%d, x%d, x%d", rd, rs1, rs2);
                    break;
                
                case 0x4:
                    if (funct7 == 0x00)
                        snprintf(result, buf_size, "xor x%d, x%d, x%d", rd, rs1, rs2);
                    else if (funct7 == 0x01)
                        snprintf(result, buf_size, "div x%d, x%d, x%d", rd, rs1, rs2);
                    break;
                
                case 0x5:
                    if (funct7 == 0x00)
                        snprintf(result, buf_size, "srl x%d, x%d, x%d", rd, rs1, rs2);
                    else if (funct7 == 0x20)
                        snprintf(result, buf_size, "sra x%d, x%d, x%d", rd, rs1, rs2);
                    else if (funct7 == 0x01)
                        snprintf(result, buf_size, "divu x%d, x%d, x%d", rd, rs1, rs2);
                    break;

                case 0x6:
                    if (funct7 == 0x00)
                        snprintf(result, buf_size, "or x%d, x%d, x%d", rd, rs1, rs2);
                    else if (funct7 == 0x01)
                        snprintf(result, buf_size, "rem x%d, x%d, x%d", rd, rs1, rs2);
                    break;

                case 0x7:
                    if (funct7 == 0x00)
                        snprintf(result, buf_size, "and x%d, x%d, x%d", rd, rs1, rs2);
                    else if (funct7 == 0x01)
                        snprintf(result, buf_size, "remu x%d, x%d, x%d", rd, rs1, rs2);
                    break;

                default:
                    snprintf(result, buf_size, "unknown instruction");
                    break;
            }
            break;
        }
            
        case 0x03: { // I-type ALU
            int32_t imm = (int32_t)instruction >> 20; // sign-extend 12-bit imm
            switch (funct3)
            {
                case 0x0:
                    snprintf(result, buf_size, "lb x%d, %d(x%d)", rd, imm, rs1);
                    break;
                case 0x1:
                    snprintf(result, buf_size, "lh x%d, %d(x%d)", rd, imm, rs1);
                    break;
                case 0x2:
                    snprintf(result, buf_size, "lw x%d, %d(x%d)", rd, imm, rs1);
                    break;
                case 0x4:
                    snprintf(result, buf_size, "lbu x%d, %d(x%d)", rd, imm, rs1);
                    break;
                case 0x5:
                    snprintf(result, buf_size, "lhu x%d, %d(x%d)", rd, imm, rs1);
                    break;
                default:
                    snprintf(result, buf_size, "unknown instruction");
                    break;
            }
            break;
        }
        case 0x13: { // I-type ALU
            int32_t imm = (int32_t)instruction >> 20; // sign-extend 12-bit imm
            uint32_t shamt = (instruction >> 20) & 0x1F;   // 5-bit shift amount

            switch (funct3)
            {
                case 0x0:
                    snprintf(result, buf_size, "addi x%d, x%d, %d", rd, rs1, imm);
                    break;
                case 0x2:
                    snprintf(result, buf_size, "slti x%d, x%d, %d", rd, rs1, imm);
                    break;
                case 0x3:
                    snprintf(result, buf_size, "sltiu x%d, x%d, %d", rd, rs1, imm);
                    break;
                case 0x4:
                    snprintf(result, buf_size, "xori x%d, x%d, %d", rd, rs1, imm);
                    break;
                case 0x6:
                    snprintf(result, buf_size, "ori x%d, x%d, %d", rd, rs1, imm);
                    break;
                case 0x7:
                    snprintf(result, buf_size, "andi x%d, x%d, %d", rd, rs1, imm);
                    break;
                case 0x1:
                    if (funct7 == 0x00)
                        snprintf(result, buf_size, "slli x%d, x%d, %d", rd, rs1, shamt);
                    break;
                case 0x5:
                    if (funct7 == 0x00)
                        snprintf(result, buf_size, "srli x%d, x%d, %d", rd, rs1, shamt);
                    else if (funct7 == 0x20)
                        snprintf(result, buf_size, "srai x%d, x%d, %d", rd, rs1, shamt);
                    break;
                default:
                    snprintf(result, buf_size, "unknown instruction");
                    break;
            }
            break;
        }
        case 0x23: { // S-type (stores)
                int32_t imm = ((instruction >> 7) & 0x1F) |  // bits 11–7
                  ((instruction >> 25) << 5);    // bits 31–25
                if (imm & 0x800) imm |= 0xFFFFF000;          // sign-extend 12-bit
            switch (funct3)
            {
                case 0x0:
                    snprintf(result, buf_size, "sb x%d, %d(x%d)", rs2, imm, rs1);
                    break;
                case 0x1:
                    snprintf(result, buf_size, "sh x%d, %d(x%d)", rs2, imm, rs1);
                    break;
                case 0x2:
                    snprintf(result, buf_size, "sw x%d, %d(x%d)", rs2, imm, rs1);
                    break;
                default:
                    snprintf(result, buf_size, "unknown instruction");
                    break;
            }
            break;
        }
        case 0x63: { // B-type
                int32_t imm = ((instruction >> 7) & 0x1E) |        // bits 11–8
                              ((instruction >> 20) & 0x7E0) |      // bits 30–25
                              ((instruction << 4) & 0x800) |       // bit 7
                              ((instruction >> 19) & 0x1000);      // bit 31
                if (imm & 0x1000) imm |= 0xFFFFE000;              // sign-extend 13-bit
                uint32_t target = addr + imm;
            switch (funct3)
            {
                case 0x0:
                    snprintf(result, buf_size, "beq x%d, x%d, %d", rs1, rs2, target);
                    break;
                case 0x1:
                    snprintf(result, buf_size, "bne x%d, x%d, %d", rs1, rs2, target);
                    break;
                case 0x4:
                    snprintf(result, buf_size, "blt x%d, x%d, %d", rs1, rs2, target);
                    break;
                case 0x5:
                    snprintf(result, buf_size, "bge x%d, x%d, %d", rs1, rs2, target);
                    break;
                case 0x6:
                    snprintf(result, buf_size, "bltu x%d, x%d, %d", rs1, rs2, target);
                    break;
                case 0x7:
                    snprintf(result, buf_size, "bgeu x%d, x%d, %d", rs1, rs2, target);
                    break;
                default:
                    snprintf(result, buf_size, "unknown instruction");
                    break;
            }
            break;
        }
        case 0x17: { // U-type AUIPC
            int32_t imm = instruction & 0xFFFFF000; // øverste 20 bits
            snprintf(result, buf_size, "auipc x%d, %d", rd, imm);
            break;
        }
        case 0x37: { // U-type LUI
            int32_t imm = instruction & 0xFFFFF000; // øverste 20
            snprintf(result, buf_size, "lui x%d, %d", rd, imm);
            break;
        }
        case 0x6F: { // J-type
            int32_t imm = 0;
            imm |= (instruction & 0xFF000);          // bits 19:12 -> imm[19:12]
            imm |= (instruction >> 9) & 0x800;       // bit 20  -> imm[11]
            imm |= (instruction >> 20) & 0x7FE;      // bits 30:21 -> imm[10:1]
            imm |= (instruction >> 11) & 0x100000;   // bit 31 -> imm[20]
            if (imm & 0x100000) imm |= 0xFFE00000;   // sign-extend 21-bit


            uint32_t target = addr + imm;
            snprintf(result, buf_size, "jal x%d, %d", rd, target);
            break;
        }
        case 0x67: { // I-type JALR
            int32_t imm = (int32_t)instruction >> 20;
            snprintf(result, buf_size, "jalr x%d, %d(x%d)", rd, imm, rs1);
            break;
        }
        case 0x73: { // ECALL
            if (instruction == 0x00000073)
                snprintf(result, buf_size, "ecall");
            break;
        }
        case 0x0F: { //pause
            if (instruction == 0x0000000F)
                snprintf(result, buf_size, "pause");
            break;
        }
        
        default:
            snprintf(result, buf_size, "unknown instruction");
            break;
    }
}
