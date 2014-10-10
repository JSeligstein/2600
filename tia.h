#ifndef __TIA_H__
#define __TIA_H__

const uint8_t TIA_VSYNC = 0x0; 
const uint8_t TIA_VBLANK = 0x1;
const uint8_t TIA_WSYNC = 0x2;
const uint8_t TIA_RSYNC = 0x3;

const uint8_t TIA_COLUPF = 0x8;     // playfield color
const uint8_t TIA_COLUBK = 0x9;     // background color

const uint8_t TIA_CTRLPF = 0xA;     // playfield control, ball, collisions

const uint8_t TIA_PF0 = 0xD;        // first 4 bytes of playfield
const uint8_t TIA_PF1 = 0xE;        // next 8 bytes of platfield
const uint8_t TIA_PF2 = 0xF;        // last 8 bytes of playfield


const uint8_t TIA_MIRROR_BIT = 0x1; // part of TIA_CTRLPF: should we mirror playfield?

void *tia_exec_thread(void *param);
void *tia_hold_open_thread(void *param);
void *tia_update_thread(void *param);
void tia_end();
int tia_start(unsigned char *pmem);
uint8_t tia_process_cycle();
void tia_process_until(uint8_t condition);
void tia_colubk_set();

extern const uint32_t ntsc_colors[][3];

#endif

