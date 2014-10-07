#include "opcodes.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "tia.h"

// http://www.emulator101.com.s3-website-us-east-1.amazonaws.com/6502-emulator/
// 3 8-bit general purpose registers A, X, and Y
// 8-bit stack pointer (fixed at RAM address $100, so can address $100-$1ff)
// 16-bit program counter
// 148 total instructions, (a lot of these are the very similar)
// Little Endian architecture

uint16_t pc;   // program counter
int8_t acc;    // accumulator
int8_t rx, ry; // registers
int8_t sp;     // stack pointer
uint8_t zf;    // zero flag
int8_t sr;     // status register

unsigned char *memory;


// $0000 - $007F   TIA registers (128 bytes)
// $0080 - $00FF   RAM
// $0200 - $02FF   RIOT registers
// $1000 - $1FFF   ROM

void cleanup() {
    free(memory);
}


void core() {
    unsigned char opcode;
    uint16_t addr;
    int jc = 0;
    for (int i = 0; jc < 5; i++) {

        opcode = memory[pc];
        pc++;

        printf("pc: %x, opcode: %x\n", pc-1, opcode);

        switch (opcode) {
            case LDA_A9:
                acc = memory[pc];
                zf = !acc;
                pc++;
                break;

            case LDX_A2:
                rx = memory[pc];
                zf = !rx;
                pc++;
                break;

            case LDY_A0:
                ry = memory[pc];
                zf = !ry;
                pc++;
                break;

            case INX_E8:
                rx++;
                zf = !rx;
                break;
                


            case NOP_EA:
                break;

            case DEY_88:
                ry--;
                zf = !ry;
                break;

            case STY_84:
                // zero-paged
                addr = memory[pc];
                memory[addr] = ry;
                pc++;
                break;
                
            case STA_85:
                // zero-paged
                addr = memory[pc];
                memory[addr] = acc;
                pc++;
                break;

            case STX_86:
                // zero-paged
                addr = memory[pc];
                memory[addr] = rx;
                pc++;
                break;

            case JMP_4C:
                pc = (memory[pc+1] << 8) | memory[pc];
                printf("jumping to %x\n", pc);
                jc++;
                break;

            default:
                printf("Unknown opcode: %x\n", opcode);
                return;
                break;
        }

    }
}

void reset() {
    // 5th bit is unused in 6502 and must be a 1
    sr = 0x20;
    zf = 0;
    sp = 0xff;
    pc = (memory[0xFFFD] << 8) | memory[0xFFFFC];
    acc = 0;
    rx = 0;
    ry = 0;
}

int readRom(char *path) {
    FILE *fp = fopen(path, "rb");
    if (!fp) {
        printf("Could not load rom: %s", path);
        return 1;
    }

    fread(&memory[0xf000], 1, 0x1000, fp);
    fclose(fp);
    return 0;
}

int main(int argc, char **argv) {
    memory = (unsigned char *)malloc(65536);

    reset();
    readRom("/Users/joel/code/2600/games/kernel1.bin");
    pc = 0xf000;
    tia_start();
    core();
    cleanup();
}












    
