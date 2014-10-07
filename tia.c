#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "CImg.h"

unsigned char *tmemory;
int tia_started = 0;
pthread_t tia_threads[1];

// starting with NTSC
uint16_t ntsc_width = 228;
uint16_t ntsc_height = 262;

void *tia_exec(void *param) {
    printf("thread started \n");
    pthread_exit(NULL);
}


int tia_start() {
    tia_started = 1;

    tmemory = (unsigned char *)malloc(ntsc_width*ntsc_height*2);
    int tc = pthread_create(&tia_threads[0], NULL, tia_exec, NULL);
    if (tc) {
        printf("tia thread creation failed\n");
        return 1;
    }

    //int status;
    //pthread_join(tia_threads[0], (void**)&status);

    return 0;
}

tia_end() {
     pthread_exit(NULL);
}
    
