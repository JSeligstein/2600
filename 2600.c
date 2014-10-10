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

uint16_t pc;    // program counter
int8_t acc;     // accumulator
uint8_t rx, ry; // index registers
int8_t sp;      // stack pointer
uint8_t zf;     // zero flag
uint8_t cf;     // carry flag
uint8_t sf;     // sign flag
int8_t sr;      // status register

unsigned char *memory;

// $0000 - $007F   TIA registers (128 bytes)
// $0080 - $00FF   RAM
// $0200 - $02FF   RIOT registers
// $1000 - $1FFF   ROM

void cleanup() {
    free(memory);
}


inline uint8_t write_memory(uint16_t addr, unsigned char data) {
    memory[addr] = data;
    if (addr < 0x1000) {
        return tia_write_memory(addr, data);
    } else {
        return 0;
    }
}

inline int core_cycle() {
    unsigned char opcode;

    uint16_t addr;
    int8_t subres;

    //printf("core_cycle\n");
    opcode = memory[pc];
    pc++;

    //printf("pc: %x, opcode: %x\n", pc-1, opcode);
    //sleep(1);
    uint8_t cycles_executed = 0;
    uint8_t tia_waited = 0;
        
    switch (opcode) {
        case STY_84:
            // zero-paged
            addr = memory[pc];
            tia_waited = write_memory(addr, ry);
            pc++;
            cycles_executed = 3;
            break;

        case STA_85:
            // zero-paged
            addr = memory[pc];
            tia_waited = write_memory(addr, acc);
            pc++;
            cycles_executed = 3;
            break;

        case STX_86:
            // zero-paged
            addr = memory[pc];
            tia_waited = write_memory(addr, rx);
            pc++;
            cycles_executed = 3;
            break;

       case DEY_88:
            ry--;
            zf = !ry;
            cycles_executed = 2;
            break;

       case STA_95:
            // zero-page, x
            addr = memory[pc]+rx;
            tia_waited = write_memory(addr, acc);
            pc++;
            cycles_executed = 4;
            break;

       case LDY_A0:
            ry = memory[pc];
            zf = !ry;
            pc++;
            cycles_executed = 2;
            break;

       case LDX_A2:
            rx = memory[pc];
            zf = !rx;
            pc++;
            cycles_executed = 2;
            break;

       case LDA_A5:
            // zero-page
            addr = memory[pc];
            acc = memory[addr];
            zf = !acc;
            pc++;
            cycles_executed = 3;
            break;

       case LDA_A9:
            acc = memory[pc];
            zf = !acc;
            pc++;
            cycles_executed = 2;
            break;

       case CPY_C0:
            zf = (memory[pc] == ry);
            pc++;
            cycles_executed = 2;
            break;

       case INY_C8:
            ry++;
            zf = !ry;
            cycles_executed = 2;
            break;

       case BNE_D0:
            // todo: page boundaries?
            if (zf) {
                cycles_executed = 2;
                pc++;
            } else {
                if (memory[pc] & 0x80) {
                    pc -= (0xff - memory[pc]);
                } else {
                    pc += memory[pc] + 1;
                }
                cycles_executed = 3;
            }
            break;

       case CPX_E0:
            subres = rx - memory[pc];
            if (subres == 0) {
                zf = 1;
                cf = 1;
                sf = 0;
            } else if (subres > 0) {
                cf = 1;
                zf = 0;
                sf = 0;
            } else {
                cf = 0;
                zf = 0;
                sf = 1;
            }
            pc++;
            cycles_executed = 2;
            break;

       case INC_E6:
            // zero-page
            addr = memory[pc];
            memory[addr]++;
            zf = !memory[addr];
            pc++;
            cycles_executed = 5;
            break;

       case INX_E8:
            rx++;
            zf = !rx;
            cycles_executed = 2;
            break;

       case NOP_EA:
            cycles_executed = 2;
            break;

       case JMP_4C:
            pc = (memory[pc+1] << 8) | memory[pc];
            //printf("jumping to %x\n", pc);
            cycles_executed = 3;
            break;

       default:
            printf("Unknown opcode: %x\n", opcode);
            exit(1);
            break;
    }

    if (!tia_waited) {
        for (; cycles_executed; cycles_executed--) {
            tia_process_cycle();
            tia_process_cycle();
            tia_process_cycle();
        }
    }

    return cycles_executed;
}

void core() {
    int cycles_executed;
    while (1) {
        cycles_executed = core_cycle();
    }
}

void reset() {
    // 5th bit is unused in 6502 and must be a 1
    sr = 0x20;
    zf = 0;
    cf = 0;
    sf = 0;
    sp = 0xff;
    pc = (memory[0xFFFD] << 8) | memory[0xFFFFC];
    acc = 0;
    rx = 0;
    ry = 0;
}

int readRom(char *path) {
    printf("loading rom: %s\n", path);
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
    if (argc == 2) {
        readRom(argv[1]);
    } else {
        readRom((char *)"/Users/joel/code/2600/games/kernel14.5.bin");
    }
    pc = 0xf000;
    tia_start(memory);
    core();
    cleanup();
}



