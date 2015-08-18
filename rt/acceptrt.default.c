// Ordinary platform: use system clock for performance.

#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>

static double time_begin;
static unsigned numBB;
static unsigned* BBcount;

void accept_roi_begin() {
    struct timeval t;
    gettimeofday(&t,NULL);
    time_begin = (double)t.tv_sec+(double)t.tv_usec*1e-6;
}

void accept_roi_end() {
    struct timeval t;
    gettimeofday(&t,NULL);
    double time_end = (double)t.tv_sec+(double)t.tv_usec*1e-6;
    double delta = time_end - time_begin;

    FILE *f = fopen("accept_time.txt", "w");
    fprintf(f, "%f\n", delta);
    fclose(f);
}

void logbb(int i) {
    BBcount[i]++;
}

void logbb_fini() {
    int i;
    FILE *f = fopen("accept_bbstats.txt", "w");
    fprintf(f, "BB\texec\n");
    for (i=0; i<numBB; i++) {
        fprintf(f, "%u\t%u\n", i, BBcount[i]);
    }
    fclose(f);
}

void logbb_init(int n) {
    int i;
    numBB = n;
    BBcount = (unsigned int *) malloc (sizeof(unsigned int) * n);
    for (i=0; i<numBB; i++){
        BBcount[i] = 0;
    }
    atexit(logbb_fini);
}
