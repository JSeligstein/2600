#ifndef __TIA_H__
#define __TIA_H__

const uint8_t TIA_VSYNC = 0x0; 
const uint8_t TIA_VBLANK = 0x1;
const uint8_t TIA_WSYNC = 0x2;
const uint8_t TIA_RSYNC = 0x3;
const uint8_t TIA_COLUBK = 0x9;

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

