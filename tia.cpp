#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include "CImg.h"

#include "tia.h"

/*

RANDOM NOTES:

Playfield is 40 units wide and 4 clocks per pixel

*/


using namespace cimg_library;

unsigned char *program_memory;
unsigned char *tia_memory;
int tia_started = 0;
pthread_t tia_threads[3];
const int TIA_THREAD_EXEC = 0;
const int TIA_THREAD_HOLD = 1;
const int TIA_THREAD_UPDATE = 2;

// starting with NTSC
uint16_t ntsc_width = 228;
uint16_t ntsc_height = 262;

// track the image
CImg<unsigned char> *tia_image;
CImgDisplay *tia_display;

// scanline data
uint16_t tia_scanline = 0;
uint16_t tia_scanline_x = 0;

// random
uint8_t waiting_condition = 0xff;
uint8_t ntsc_outline_color[] = {155, 155, 0};

// condition flags
uint8_t tia_fwsync = 0;
uint8_t tia_fvblank_top = 0;
uint8_t tia_fvblank_bottom = 0;
uint8_t tia_fvsync = 0;
uint8_t tia_fcolubk = 0;

// tracking framerate
const int TIA_FRAMERATE_MAX_SAMPLES = 20;
int tia_tickidx = 0;
int tia_ticksum = 0;
int *tia_ticklist;
clock_t tia_framestart;
clock_t tia_frameend;

double tia_framerate(int newtick) {
    tia_ticksum -= tia_ticklist[tia_tickidx];
    tia_ticksum += newtick;
    tia_ticklist[tia_tickidx] = newtick;
    tia_tickidx++;
    if (tia_tickidx == TIA_FRAMERATE_MAX_SAMPLES) {
        tia_tickidx = 0;
    }

    return((double)tia_ticksum/TIA_FRAMERATE_MAX_SAMPLES);
}

void *tia_exec_thread(void *param) {
    //printf("exec thread started \n");

    //printf("thread started:  %p, %p\n", tia_image, tia_display);
    //printf("starting update,hold threads\n");

    int thc = pthread_create(&tia_threads[TIA_THREAD_HOLD], NULL, tia_hold_open_thread, NULL);
    if (thc) {
        printf("tia hold thread creation failed\n");
    }

    int tuc = pthread_create(&tia_threads[TIA_THREAD_UPDATE], NULL, tia_update_thread, NULL);
    if (tuc) {
        printf("tia update thread creation failed\n");
    }

    pthread_join(tia_threads[TIA_THREAD_HOLD], NULL);
    pthread_join(tia_threads[TIA_THREAD_UPDATE], NULL);
    pthread_exit(NULL);
}

void *tia_hold_open_thread(void *param) {
    //printf("hold thread started \n");
    while (!tia_display->is_closed()) {
        if (tia_display->is_resized()) {
            tia_display->resize(tia_display);
        }
        tia_display->wait(20);
    }
    pthread_exit(NULL);
}

void *tia_update_thread(void *param) {
    //printf("udpate thread started \n");
    while (1);
}

void tia_process_until(uint8_t condition) {
    //printf("tia_process_until %d starting at %d, %d\n", condition, tia_scanline_x, tia_scanline);
    waiting_condition = condition;

    switch (condition) {
        case TIA_WSYNC:
            while (tia_fwsync) {
                tia_process_cycle();
            }
            while (!tia_fwsync) {
                tia_process_cycle();
            }
            break;
        case TIA_VSYNC:
                break;
            if (program_memory[TIA_VSYNC] == 0) {
                while (tia_fvsync) {
                    tia_process_cycle();

                }
                while (!tia_fvsync) {
                    tia_process_cycle();
                }
            } else {
                printf("skipping this vsync\n");
            }
            break;
        case TIA_VBLANK:
            if (program_memory[TIA_VBLANK] == 0) {
                while (tia_fvblank_top) {
                    tia_process_cycle();
                }
                while (!tia_fvblank_top) {
                    tia_process_cycle();
                }
            } else {
                while (tia_fvblank_bottom) {
                    tia_process_cycle();
                }
                while (!tia_fvblank_bottom) {
                    tia_process_cycle();
                }
            }
            break;
        default:
            printf("Invalid process_until condition: %d\n", condition);
            break;
    }

    //printf("tia_process_until done at %d, %d\n", tia_scanline_x, tia_scanline);
    waiting_condition = 0xff;

}

inline void tia_process_flags() {
    if (tia_scanline_x == 0) {
        tia_fvsync = (tia_scanline == 3) ? 1 : 0;
        tia_fvblank_bottom = (tia_scanline >= 232) ? 1 : 0;
        tia_fvblank_top = (tia_scanline < 3) ? 1 : 0;
    }
    tia_fwsync = (tia_scanline_x == 66) ? 1 : 0;
}

inline const uint32_t* ntsc_color(uint8_t byte) {
    // luminosity is first 4 bits followed by hue's 4 bits
    // luminosity comes in pairs of two
    // ntsc_colors is a big array running one hue at a time
    int x = (byte & 0xf) / 2;
    int y = (byte & 0xf0) >> 4;
    const uint32_t *color = ntsc_colors[y * 8 + x];
    //const uint32_t *color = ntsc_colors[((byte & 0xf0) >> 4) * 8 + ((byte & 0xf) /2)];
    return color;
}

void tia_colubk_set() {
    tia_fcolubk = 1;
}


int abc = 0, def = 0;
inline uint8_t tia_process_cycle() {
    uint8_t ret = 0xff;

    const uint32_t *color = NULL;
    uint8_t flag;

    // first check if there's a playfield active

    if (tia_scanline_x < 68) {
        // no pf
    } else if (tia_scanline_x < 84) {
        // left side pf0... reversed high nibble
        flag = ((tia_scanline_x-68) / 4)+4;
        if (program_memory[TIA_PF0] & (1 << flag)) {
            color = ntsc_color(program_memory[TIA_COLUPF]);
        }
    } else if (tia_scanline_x < 116) {
        // left side pf1
        flag = (115-tia_scanline_x) / 4;
        if (program_memory[TIA_PF1] & (1 << flag)) {
            color = ntsc_color(program_memory[TIA_COLUPF]);
        }
    } else if (tia_scanline_x < 148) {
        // left side pf2 ... reverseed
        flag = 7-((147-tia_scanline_x) / 4);
        if (program_memory[TIA_PF2] & (1 << flag)) {
            color = ntsc_color(program_memory[TIA_COLUPF]);
        }
    } else {
        // check for mirroring
        if (program_memory[TIA_CTRLPF] & TIA_MIRROR_BIT) {
            if (tia_scanline_x < 180) {
                // right side pf2 mirrored
                flag = ((179-tia_scanline_x) / 4);
                if (program_memory[TIA_PF2] & (1 << flag)) {
                    color = ntsc_color(program_memory[TIA_COLUPF]);
                }
            } else if (tia_scanline_x < 212) {
                // right side pf1 mirrored
                flag = 7 - ((211-tia_scanline_x)/4);
                if (program_memory[TIA_PF1] & (1 << flag)) {
                    color = ntsc_color(program_memory[TIA_COLUPF]);
                }
            } else {
                // right side pf0 mirrored
                flag = 7-((tia_scanline_x-212) / 4);
                if (program_memory[TIA_PF0] & (1 << flag)) {
                    color = ntsc_color(program_memory[TIA_COLUPF]);
                }
            }
        } else {
            if (tia_scanline_x < 164) {
                // right side pf0 (non-mirrored) ... reversed high nibble
                flag = ((tia_scanline_x-148) / 4)+4;
                if (program_memory[TIA_PF0] & (1 << flag)) {
                    color = ntsc_color(program_memory[TIA_COLUPF]);
                }
            } else if (tia_scanline_x < 196) {
                // right side pf1 (non-mirrored)
                flag = (195-tia_scanline_x) / 4;
                if (program_memory[TIA_PF1] & (1 << flag)) {
                    color = ntsc_color(program_memory[TIA_COLUPF]);
                }
            } else { 
                // right side pf2 (non-mirrored)
                flag = 7-(227-tia_scanline_x) / 4;
                if (program_memory[TIA_PF2] & (1 << flag)) {
                    color = ntsc_color(program_memory[TIA_COLUPF]);
                }
            }
        }
    }

    if (color == NULL && tia_fcolubk) {
        color = ntsc_color(program_memory[TIA_COLUBK]);
    }

    if (color != NULL) {
        uint32_t midx = (tia_scanline * ntsc_width + tia_scanline_x) * 3;
        tia_memory[midx] = color[0];
        tia_memory[midx+1] = color[1];
        tia_memory[midx+2] = color[2];
    }

    tia_scanline_x++;
    if (tia_scanline_x >= ntsc_width) {
        tia_scanline_x = 0;
        tia_scanline++;
        tia_fcolubk = 0;

        if (tia_scanline >= ntsc_height) {
            tia_scanline = 0;

            tia_frameend = clock();
            float diff = (((float)tia_frameend - (float)tia_framestart) / CLOCKS_PER_SEC  ) * 1000;   
            double avg = tia_framerate((int)diff);
            #ifdef TIA_PRINT_FRAME_COUNT
            printf("frame time: %d, framerate: %f\n", (int)diff, 1000.0/avg);
            #endif
            tia_framestart = tia_frameend;
            
        }
    }

    tia_process_flags();

    if (tia_scanline_x == 0 && tia_scanline == 232) {
        int idx = 0;
        for (int cy = 0; cy < ntsc_height; cy++) {
            for (int cx = 0; cx < ntsc_width; cx++) {
                (*tia_image)(cx, cy, 0) = tia_memory[idx++];
                (*tia_image)(cx, cy, 1) = tia_memory[idx++];
                (*tia_image)(cx, cy, 2) = tia_memory[idx++];
            }
        }

        tia_image->draw_rectangle(67, 39, ntsc_width, ntsc_height-29,
                                  ntsc_outline_color, 1, 0xffffffff);

        tia_display->display(*tia_image);
        tia_display->show();
    }

    return ret;
}

int tia_start(unsigned char *pmem) {
    if (tia_started) {
        return 1;
    }

    program_memory = pmem;
    tia_started = 1;
    tia_scanline = 0;
    tia_scanline_x = 0;
    tia_process_flags();

    tia_ticksum = 0;
    tia_ticklist = (int *)calloc(TIA_FRAMERATE_MAX_SAMPLES, sizeof(int));
    tia_framestart = clock();
    
    tia_memory = (unsigned char *)malloc(ntsc_width*ntsc_height*3);
    //tia_image = new CImg<unsigned char>(ntsc_width*3, ntsc_height, 1,3, 0);
    tia_image = new CImg<unsigned char>(ntsc_width, ntsc_height, 1,3, 0);
    tia_display = new CImgDisplay(*tia_image, "TIA");
    tia_display->show();

    int tc = pthread_create(&tia_threads[TIA_THREAD_EXEC], NULL, tia_exec_thread, NULL);
    if (tc) {
        //printf("tia thread creation failed\n");
        return 1;
    }

    return 0;
}

void tia_end() {
     pthread_exit(NULL);
}
    

const uint32_t ntsc_colors[][3] = {
{0,0,0},
{64,64,64},
{108,108,108},
{144,144,144},
{176,176,176},
{200,200,200},
{220,220,220},
{236,236,236},
{68,68,0},
{100,100,16},
{132,132,36},
{160,160,52},
{184,184,64},
{208,208,80},
{232,232,92},
{252,252,104},
{112,40,0},
{132,68,20},
{152,92,40},
{172,120,60},
{188,140,76},
{204,160,92},
{220,180,104},
{236,200,120},
{132,24,0},
{152,52,24},
{172,80,48},
{192,104,72},
{208,128,92},
{224,148,112},
{236,168,128},
{252,188,148},
{136,0,0},
{156,32,32},
{176,60,60},
{192,88,88},
{208,112,112},
{224,136,136},
{236,160,160},
{252,180,180},
{120,0,92},
{140,32,116},
{160,60,136},
{176,88,156},
{192,112,176},
{208,132,192},
{220,156,208},
{236,176,224},
{72,0,120},
{96,32,144},
{120,60,164},
{140,88,184},
{160,112,204},
{180,132,220},
{196,156,236},
{212,176,252},
{20,0,132},
{48,32,152},
{76,60,172},
{104,88,192},
{124,112,208},
{148,136,224},
{168,160,236},
{188,180,252},
{0,0,136},
{28,32,156},
{56,64,176},
{80,92,192},
{104,116,208},
{124,140,224},
{144,164,236},
{164,184,252},
{0,24,124},
{28,56,144},
{56,84,168},
{80,112,188},
{104,136,204},
{124,156,220},
{144,180,236},
{164,200,252},
{0,44,92},
{28,76,120},
{56,104,144},
{80,132,172},
{104,156,192},
{124,180,212},
{144,204,232},
{164,224,252},
{0,60,44},
{28,92,72},
{56,124,100},
{80,156,128},
{104,180,148},
{124,208,172},
{144,228,192},
{164,252,212},
{0,60,0},
{32,92,32},
{64,124,64},
{92,156,92},
{116,180,116},
{140,208,140},
{164,228,164},
{184,252,184},
{20,56,0},
{52,92,28},
{80,124,56},
{108,152,80},
{132,180,104},
{156,204,124},
{180,228,144},
{200,252,164},
{44,48,0},
{76,80,28},
{104,112,52},
{132,140,76},
{156,168,100},
{180,192,120},
{204,212,136},
{224,236,156},
{68,40,0},
{100,72,24},
{132,104,48},
{160,132,68},
{184,156,88},
{208,180,108},
{232,204,124},
{252,224,140},
};
