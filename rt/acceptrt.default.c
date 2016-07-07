// Ordinary platform: use system clock for performance.

#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define HALF_MANTISSA_W     10
#define HALF_EXPONENT_W     5
#define HALF_EXP_BIAS       ( (1 << (HALF_EXPONENT_W-1)) - 1)
#define HALF_EXP_MASK       ( (1 << HALF_EXPONENT_W) - 1 )
#define HALF_MAN_MASK       ( (1 << HALF_MANTISSA_W) - 1 )
#define HALF_SIGN_MASK      (1 << 15)
#define FLOAT_MANTISSA_W    23
#define FLOAT_EXPONENT_W    8
#define FLOAT_EXP_BIAS      ( (1 << (FLOAT_EXPONENT_W-1)) - 1)
#define FLOAT_EXP_MASK      ( (1 << FLOAT_EXPONENT_W) - 1 )
#define FLOAT_MAN_MASK      ( (1 << FLOAT_MANTISSA_W) - 1 )
#define FLOAT_SIGN_MASK     (1 << 31)
#define DOUBLE_MANTISSA_W   52
#define DOUBLE_EXPONENT_W   11
#define DOUBLE_EXP_BIAS     ( (1 << (DOUBLE_EXPONENT_W-1)) - 1)
#define DOUBLE_EXP_MASK     ( (1 << DOUBLE_EXPONENT_W) - 1 )
#define DOUBLE_MAN_MASK     ( (1ULL << DOUBLE_MANTISSA_W) - 1 )
#define DOUBLE_SIGN_MASK    (1ULL << 63)


typedef struct _minmax { int min, max; int sign; char* iid; } minmax;

static double time_begin;
static unsigned numBB;
static unsigned numFP;
static unsigned* BBstat;
static minmax* FPstat;

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
    BBstat[i]++;
}

void logload(char* iid, char* ty, int64_t addr, int64_t align, int64_t val) {
    printf("ld, %s, %s, 0x%016llx (%llx), 0x%016llx\n", iid, ty, addr, align, val);
}

void logcondbranch(char* iid, int32_t cond, char* succ0, char* succ1) {
    printf("br, %s, %d, %s, %s\n", iid, cond, succ0, succ1);
}

void loguncondbranch(char* iid, char* succ) {
    printf("br, %s, %s\n", iid, succ);
}

void logfloat(int type, char* iid, int fpid, int64_t value) {

    int32_t exponent = 0;
    int64_t mantissa = 0;
    int32_t sign = 0;
    switch (type) {
        case 1:
            exponent = ( (value >> HALF_MANTISSA_W) & HALF_EXP_MASK );
            mantissa = value & HALF_MAN_MASK;
            sign = value & HALF_SIGN_MASK ? 1 : 0;
            break;
        case 2:
            exponent = ( (value >> FLOAT_MANTISSA_W) & FLOAT_EXP_MASK );
            mantissa = value & FLOAT_MAN_MASK;
            sign = value & FLOAT_SIGN_MASK ? 1 : 0;
            break;
        case 3:
            exponent = ( (value >> DOUBLE_MANTISSA_W) & DOUBLE_EXP_MASK );
            mantissa = value & DOUBLE_MAN_MASK;
            sign = value & DOUBLE_SIGN_MASK ? 1 : 0;
            break;
        default:
            // The type enum is unkown, flag it
            FPstat[fpid].iid = "unknown type";
            return;
    }

    // For now let's assume the use of subnormals as specified by
    // the IEEE-754 spec is forbidden. Thus, to test that a number
    // is zero, we just have to check the exponent value
    if (exponent==0)
        return;

    FPstat[fpid].min = exponent < FPstat[fpid].min ? exponent : FPstat[fpid].min;
    FPstat[fpid].max = exponent > FPstat[fpid].max ? exponent : FPstat[fpid].max;
    FPstat[fpid].iid = FPstat[fpid].iid==NULL ? iid : FPstat[fpid].iid;
    FPstat[fpid].sign |= FPstat[fpid].sign;
    // printf("\t[min, max, curr]: [%d, %d, %d]\n", FPstat[fpid].min, FPstat[fpid].max, exponent);
}

void logbb_fini() {
    int i;
    FILE *f1 = fopen("accept_bbstats.txt", "w");
    FILE *f2 = fopen("accept_fpstats.txt", "w");
    fprintf(f1, "BB\texec\n");
    for (i=0; i<numBB; i++) {
        fprintf(f1, "%u\t%u\n", i, BBstat[i]);
    }
    fprintf(f2, "id\tmin\tmax\tsign\n");
    for (i=0; i<numFP; i++) {
        if (FPstat[i].iid!=NULL)
            fprintf(f2, "%s\t%d\t%d\t%d\n", FPstat[i].iid, FPstat[i].min, FPstat[i].max, FPstat[i].sign);
    }
    fclose(f1);
    fclose(f2);
}

void logbb_init(int bbCount, int fpCount) {
    int i;
    numBB = bbCount;
    numFP = fpCount;
    printf("Got %d bbs and %d fps\n", bbCount, fpCount);
    BBstat = (unsigned int *) malloc (sizeof(unsigned int) * bbCount);
    FPstat = (minmax *) malloc (sizeof(minmax) * fpCount);
    for (i=0; i<numBB; i++){
        BBstat[i] = 0;
    }
    for (i=0; i<numFP; i++){
        FPstat[i].min = 1024;
        FPstat[i].max = -1023;
        FPstat[i].iid = NULL;
        FPstat[i].sign = 0;
    }
    atexit(logbb_fini);
}
