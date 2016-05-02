#include <sys/time.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <errno.h>
#include <signal.h>

#include "fann.h"
#include "npu.h"

// SNNAP batch size
#define BATCH_SIZE 512

static double time_begin;

// File pointer to the
// ANN training file
static FILE* npuLog;

// FANN neural networks

// Input and output buffers
static unsigned char * input_buffer;
static unsigned char * output_buffer;
static unsigned char ** output_dst_buffer;
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

static int do_prof = 0;

void engage_prof(int signo) {
  if(signo == SIGUSR1) {
    do_prof = 1;
  }
}

static unsigned prof_count = 0;
#define PROF_LIMIT 200000

void lognpu(int numIn, int numOut, int32_t num_args, ...) {
    if(!do_prof || prof_count >= PROF_LIMIT) {
      return;
    }
    prof_count++;

    int i;

    // Process arguments
    va_list arguments;
    va_start(arguments, num_args);
    // Inputs
    for (i=0; i<numIn; i++) {
        if (i==0) {
            fprintf(npuLog, "%f", (float)(va_arg(arguments, unsigned char)-128)/128);
        } else {
            fprintf(npuLog, " %f", (float)(va_arg(arguments, unsigned char)-128)/128);
        }
    }
    fprintf(npuLog, "\n");
    // Outputs
    for (i=0; i<numOut; i++) {
        if (i==0) {
            fprintf(npuLog, "%f", (float)(*va_arg(arguments, unsigned char*)-128)/128);
        } else {
            fprintf(npuLog, " %f", (float)(*va_arg(arguments, unsigned char*)-128)/128);
        }
    }
    fprintf(npuLog, "\n");
    va_end(arguments);
}

static int * flag_addr = 0;
void invokenpu(int numIn, int numOut, int32_t num_args, ...) {
    int i, j, i_idx, o_idx;
    unsigned char input[numIn];
    unsigned char *output;

    // Process arguments
    va_list arguments;
    va_start(arguments, num_args);

    // Push input values to buffer
    for (i=0; i<numIn; i++) {
        input_buffer[input_ptr++] = va_arg(arguments, unsigned char)-128;
    }

    // Push output pointer to buffer
    for (i=0; i<numOut; i++) {
        output_dst_buffer[output_dst_ptr++] = va_arg(arguments, unsigned char*);
    }

    if (input_ptr==numIn*BATCH_SIZE) {
        input_ptr = 0;
        output_dst_ptr = 0;

        *flag_addr = 0xDEADBEEF;
        barrier();
        npu();
        barrier();
        while(*flag_addr == 0xDEADBEEF) { barrier(); }

        for (i=0; i<numOut*BATCH_SIZE; i++) {
            *(output_dst_buffer[i]) = output_buffer[i]+128;
        }

    }

    va_end(arguments);
}

void log_fini() {
    int i;
    fclose(npuLog);
}

void log_init(int bbCount, int fpCount) {
    sigset_t emptyset;

    sigemptyset(&emptyset);
    struct sigaction sa = {
      .sa_handler = engage_prof,
      .sa_mask = emptyset,
      .sa_flags = 0
    };
   
    if(sigaction(SIGUSR1, &sa, 0) < 0) {
      perror("sigaction");
      exit(1);
    }

    npuLog = fopen("accept_npulog.txt", "w");
    atexit(log_fini);
}

void npu_fini() {
    free(output_dst_buffer);
    npu_unmap();
}

void npu_init(int bbCount, int fpCount) {
    // TODO: allow for flexible numbers of inputs and outputs in NN
    int num_inputs = 9;
    int num_outputs = 1;

    if (access( "output.snnap", F_OK ) != -1) {
      npu_config("output.snnap");
    }

    // Allocate memory for buffers
    if (npu_map() == -1) {
      if(errno == ENOENT) { 
        fprintf(stderr, "npu_map: unable to map OCM. Perhaps the kernel driver has not been loaded?\n");
      } else {
        perror("npu_map");
      }
      exit(1);
    }

    flag_addr = (int*) (dst_start + num_outputs * BATCH_SIZE - 4);

    input_buffer  = (unsigned char*)src_start;
    output_buffer = (unsigned char*)dst_start;

    output_dst_buffer = (unsigned char **) malloc(sizeof(unsigned char*) * num_outputs * BATCH_SIZE);

    // Initialize pointers
    input_ptr = 0;
    output_dst_ptr = 0;

    printf("done!\n");
    atexit(npu_fini);
}
