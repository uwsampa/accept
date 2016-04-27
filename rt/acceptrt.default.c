#include <sys/time.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>

#include "fann.h"

// SNNAP batch size
#define BATCH_SIZE 64

static double time_begin;

// File pointer to the
// ANN training file
static FILE* npuLog;

// FANN neural networks
static struct fann *ann;

// Input and output buffers
static float * input_buffer;
static float * output_buffer;
static float ** output_dst_buffer;
static int input_ptr;
static int output_dst_ptr;

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

void lognpu(int numIn, int numOut, int32_t num_args, ...) {
    int i;

    // Process arguments
    va_list arguments;
    va_start(arguments, num_args);
    // Inputs
    for (i=0; i<numIn; i++) {
        if (i==0) {
            fprintf(npuLog, "%f", va_arg(arguments, float));
        } else {
            fprintf(npuLog, " %f", va_arg(arguments, float));
        }
    }
    fprintf(npuLog, "\n");
    // Outputs
    for (i=0; i<numOut; i++) {
        if (i==0) {
            fprintf(npuLog, "%f", *va_arg(arguments, float*));
        } else {
            fprintf(npuLog, " %f", *va_arg(arguments, float*));
        }
    }
    fprintf(npuLog, "\n");
    va_end(arguments);
}

void invokenpu(int numIn, int numOut, int32_t num_args, ...) {
    int i, j, i_idx, o_idx;
    fann_type input[numIn];
    fann_type *output;

    // Process arguments
    va_list arguments;
    va_start(arguments, num_args);

    // Push input values to buffer
    for (i=0; i<numIn; i++) {
        input_buffer[input_ptr++] = va_arg(arguments, float);
    }

    // Push output pointer to buffer
    for (i=0; i<numOut; i++) {
        output_dst_buffer[output_dst_ptr++] = va_arg(arguments, float*);
    }

    if (input_ptr==numIn*BATCH_SIZE) {
        input_ptr = 0;
        output_dst_ptr = 0;

        // Equivalent to npu() begin
        // Reset input/output indices
        i_idx = 0;
        o_idx = 0;
        // Process BATCH_SIZE inputs at once
        for (i=0; i<BATCH_SIZE; i++) {
            // Prepare FANN inputs from input buffer
            for (j=0; j<numIn; j++) {
                input[j] = (fann_type) input_buffer[i_idx++];
            }
            // Invoke FANN
            output = fann_run(ann, input);
            // Move outputs to output buffer
            for (j=0; j< numOut; j++) {
                output_buffer[o_idx++] = (float) output[j];
            }
        }
        // Equivalent to npu() end

        for (i=0; i<numOut*BATCH_SIZE; i++) {
            *(output_dst_buffer[i]) = output_buffer[i];
        }

    }

    va_end(arguments);
}

void log_fini() {
    int i;
    fclose(npuLog);
}

void log_init(int bbCount, int fpCount) {
    npuLog = fopen("accept_npulog.txt", "w");
    atexit(log_fini);
}

void npu_fini() {
    fann_destroy(ann);
    free(input_buffer);
    free(output_buffer);
    free(output_dst_buffer);
}

void npu_init(int bbCount, int fpCount) {
    if (access( "output.nn", F_OK ) != -1) {
        ann = fann_create_from_file("output.nn");
    }
    printf("Initializing neural network with %d inputs, and %d outputs\n", fann_get_num_input(ann), fann_get_num_output(ann));
    // Allocate memory for buffers
    input_buffer = (float *) malloc(sizeof(float)*fann_get_num_input(ann)*BATCH_SIZE);
    output_buffer = (float *) malloc(sizeof(float)*fann_get_num_output(ann)*BATCH_SIZE);
    output_dst_buffer = (float **) malloc(sizeof(float*)*fann_get_num_output(ann)*BATCH_SIZE);
    // Initialize pointers
    input_ptr = 0;
    output_dst_ptr = 0;

    printf("done!\n");
    atexit(npu_fini);
}
