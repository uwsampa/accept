#include <sys/time.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>

#include "fann.h"

static double time_begin;

static FILE* npuLog;

static struct fann *ann;

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

void lognpu(char* ret_type, int64_t ret_val, int32_t num_args, ...) {
    int i;

    va_list arguments;
    va_start(arguments, num_args);
    for (i=0; i<num_args; i++) {
        if (i==0) {
            fprintf(npuLog, "%f", va_arg(arguments, float));
        } else {
            fprintf(npuLog, " %f", va_arg(arguments, float));
        }
    }
    fprintf(npuLog, "\n%f\n", *((float *) &ret_val));
    va_end(arguments);
}

int64_t invokenpu(char* ret_type, int64_t ret_val, int32_t num_args, ...) {
    int i;
    va_list arguments;
    va_start(arguments, num_args);
    fann_type input[num_args];
    for (i=0; i< num_args; i++) {
        input[i] = (fann_type) va_arg(arguments, float);
    }
    fann_type *output = fann_run(ann, input);
    return *((int64_t *) output);
}

void log_init(int bbCount, int fpCount) {
    npuLog = fopen("accept_npulog.txt", "w");
    atexit(log_fini);
}

void log_fini() {
    int i;
    fclose(npuLog);
}

void npu_init(int bbCount, int fpCount) {
    if (access( "output.nn", F_OK ) != -1) {
        ann = fann_create_from_file("output.nn");
    }
    atexit(npu_fini);
}

void npu_fini(int bbCount, int fpCount) {
    fann_destroy(ann);
}
