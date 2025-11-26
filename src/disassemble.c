#include <stddef.h>
#include <stdint.h>
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
        case 0x33: // R-type
            switch (funct3)
            {
                case 0x0:
                    if (funct7 == 0x00)
                        snprintf(result, buf_size, "add x%d, x%d, x%d", rd, rs1, rs2);
                    else if (funct7 == 0x20)
                        snprintf(result, buf_size, "sub x%d, x%d, x%d", rd, rs1, rs2);
                    break;
                
                case 0x1:
                    snprintf(result, buf_size, "sll x%d, x%d, x%d", rd, rs1, rs2);
                    break;
                
                case 0x2:
                    snprintf(result, buf_size, "slt x%d, x%d, x%d", rd, rs1, rs2);
                    break;
                
                case 0x3:
                    snprintf(result, buf_size, "sltu x%d, x%d, x%d", rd, rs1, rs2);
                    break;
                
                case 0x4:
                    snprintf(result, buf_size, "xor x%d, x%d, x%d", rd, rs1, rs2);
                    break;
                
                case 0x5:
                    if (funct7 == 0x00)
                        snprintf(result, buf_size, "srl x%d, x%d, x%d", rd, rs1, rs2);
                    else if (funct7 == 0x20)
                        snprintf(result, buf_size, "sra x%d, x%d, x%d", rd, rs1, rs2);
                    break;

                case 0x6:
                    snprintf(result, buf_size, "or x%d, x%d, x%d", rd, rs1, rs2);
                    break;

                case 0x7:
                    snprintf(result, buf_size, "and x%d, x%d, x%d", rd, rs1, rs2);
                    break;

                default:
                    snprintf(result, buf_size, "unknown instruction");
                    break;
            }
        
            break;
        default:
            snprintf(result, buf_size, "unknown instruction");
            break;
    }


}
