#include "opcodes.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

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

int num_vblanks = 0;

inline void write_memory(uint16_t addr, unsigned char data) {
    memory[addr] = data;
    if (addr < 0x1000) {
        // special addresses
        if (addr == TIA_WSYNC) {
            tia_process_until(TIA_WSYNC);
        } else if (addr == TIA_VSYNC) {
            tia_process_until(TIA_VSYNC);
        } else if (addr == TIA_VBLANK) {
            tia_process_until(TIA_VBLANK);
            num_vblanks++;
        } else if (addr == TIA_COLUBK) {
            tia_colubk_set();
        } else {
        //    memory[addr] = data;
        }
    } else {
        //memory[addr] = data;
    }
}

inline int core_cycle(int cycles_to_execute) {
    unsigned char opcode;
    uint16_t addr;

    int cycles_left = cycles_to_execute;
    while (cycles_left > 0) {
        //printf("core_cycle\n");
        opcode = memory[pc];
        pc++;

        printf("pc: %x, opcode: %x\n", pc-1, opcode);

        switch (opcode) {
            case STY_84:
                // zero-paged
                addr = memory[pc];
                write_memory(addr, ry);
                pc++;
                cycles_left -= 3;
                break;

            case STA_85:
                // zero-paged
                addr = memory[pc];
                write_memory(addr, acc);
                pc++;
                cycles_left -= 3;
                break;

            case STX_86:
                // zero-paged
                addr = memory[pc];
                write_memory(addr, rx);
                pc++;
                cycles_left -= 3;
                break;

            case DEY_88:
                ry--;
                zf = !ry;
                cycles_left -= 2;
                break;

            case LDY_A0:
                ry = memory[pc];
                zf = !ry;
                pc++;
                cycles_left -= 2;
                break;

            case LDX_A2:
                rx = memory[pc];
                zf = !rx;
                pc++;
                cycles_left -= 2;
                break;

            case LDA_A9:
                acc = memory[pc];
                zf = !acc;
                pc++;
                cycles_left -= 2;
                break;

            case INX_E8:
                rx++;
                zf = !rx;
                cycles_left -= 2;
                break;

            case NOP_EA:
                cycles_left -= 2;
                break;

            case JMP_4C:
                pc = (memory[pc+1] << 8) | memory[pc];
                //printf("jumping to %x\n", pc);
                cycles_left -= 3;
                break;

            default:
                printf("Unknown opcode: %x\n", opcode);
                return cycles_to_execute-cycles_left;
                break;
        }
    }

    return cycles_to_execute - cycles_left;
}

void core() {
    int cycles_executed;
    while (1) {
        cycles_executed = core_cycle(1);
        for (; cycles_executed; cycles_executed--) {
            tia_process_cycle();
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
    readRom((char *)"/Users/joel/code/2600/games/kernel1.bin");
    pc = 0xf000;
    tia_start(memory);
    core();
    cleanup();
}












    
