// Copyright (c) 2007 Intel Corp.

// Black-Scholes
// Analytical method for calculating European Options
//
// 
// Reference Source: Options, Futures, and Other Derivatives, 3rd Edition, Prentice 
// Hall, John C. Hull,

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#ifdef ENABLE_PARSEC_HOOKS
#include <hooks.h>
#endif

// Multi-threaded pthreads header
#ifdef ENABLE_THREADS
#define MAX_THREADS 128
// Add the following line so that icc 9.0 is compatible with pthread lib.
#define __thread __threadp  
MAIN_ENV
#undef __thread
#endif

//EnerC
#include<enerc.h>

#include "npu.h"
#include "xil_io.h"
#include "xil_mmu.h"
#include "xil_types.h"


// Multi-threaded OpenMP header
#ifdef ENABLE_OPENMP
#include <omp.h>
#endif

// Multi-threaded header for Windows
#ifdef WIN32
#pragma warning(disable : 4305)
#pragma warning(disable : 4244)
#include <windows.h>
#define MAX_THREADS 128
#endif

//Precision to use for calculations
#define fptype float

int NUM_RUNS = 10;
#define SRC_IMAGE_ADDR 0x08000000

typedef struct OptionData_ {
        APPROX fptype s;          // spot price
        APPROX fptype strike;     // strike price
        APPROX fptype r;          // risk-free interest rate
        APPROX fptype divq;       // dividend rate
        APPROX fptype v;          // volatility
        APPROX fptype t;          // time to maturity or option expiration in years 
                           //     (1yr = 1.0, 6mos = 0.5, 3mos = 0.25, ..., etc)  
        char OptionType;   // Option type.  "P"=PUT, "C"=CALL
        APPROX fptype divs;       // dividend vals (not used in this test)
        APPROX fptype DGrefval;   // DerivaGem Reference Value
} OptionData;

OptionData *data;
APPROX fptype *prices;
int numOptions;

int    * otype;
APPROX fptype * sptprice;
APPROX fptype * strike;
APPROX fptype * rate;
APPROX fptype * volatility;
APPROX fptype * otime;
int numError = 0;
int nThreads;

////////////////////////////////////////////////////////////////////////////////
// Cumulative Normal Distribution Function
// See Hull, Section 11.8, P.243-244
APPROX fptype inv_sqrt_2xPI = 0.39894228040143270286;
fptype CNDF ( fptype InputX ) 
{
    int sign;

    APPROX fptype OutputX;
    APPROX fptype xInput;
    APPROX fptype xNPrimeofX;
    APPROX fptype expValues;
    APPROX fptype xK2;
    APPROX fptype xK2_2, xK2_3;
    APPROX fptype xK2_4, xK2_5;
    APPROX fptype xLocal, xLocal_1;
    APPROX fptype xLocal_2, xLocal_3;

    // Check for negative value of InputX
    if (InputX < 0.0) {
        InputX = -InputX;
        sign = 1;
    } else 
        sign = 0;

    xInput = InputX;
 
    // Compute NPrimeX term common to both four & six decimal accuracy calcs
    expValues = exp(-0.5f * InputX * InputX);
    xNPrimeofX = expValues;
    xNPrimeofX = xNPrimeofX * inv_sqrt_2xPI;

    xK2 = 0.2316419 * xInput;
    xK2 = 1.0 + xK2;
    xK2 = 1.0 / xK2;
    xK2_2 = xK2 * xK2;
    xK2_3 = xK2_2 * xK2;
    xK2_4 = xK2_3 * xK2;
    xK2_5 = xK2_4 * xK2;
    
    xLocal_1 = xK2 * 0.319381530;
    xLocal_2 = xK2_2 * (-0.356563782);
    xLocal_3 = xK2_3 * 1.781477937;
    xLocal_2 = xLocal_2 + xLocal_3;
    xLocal_3 = xK2_4 * (-1.821255978);
    xLocal_2 = xLocal_2 + xLocal_3;
    xLocal_3 = xK2_5 * 1.330274429;
    xLocal_2 = xLocal_2 + xLocal_3;

    xLocal_1 = xLocal_2 + xLocal_1;
    xLocal   = xLocal_1 * xNPrimeofX;
    xLocal   = 1.0 - xLocal;

    OutputX  = xLocal;
    
    if (sign) {
        OutputX = 1.0 - OutputX;
    }
    
    return ENDORSE(OutputX);
} 

// For debugging
void print_xmm(fptype in, char* s) {
    printf("%s: %f\n", s, in);
}

//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////
fptype BlkSchlsEqEuroNoDiv( APPROX fptype sptprice,
                            APPROX fptype strike, APPROX fptype rate, APPROX fptype volatility,
                            APPROX fptype time, int otype)
{
    APPROX fptype OptionPrice;

    // local private working variables for the calculation
    APPROX fptype xStockPrice;
    APPROX fptype xStrikePrice;
    APPROX fptype xRiskFreeRate;
    APPROX fptype xVolatility;
    APPROX fptype xTime;
    APPROX fptype xSqrtTime;

    APPROX fptype logValues;
    APPROX fptype xLogTerm;
    APPROX fptype xD1; 
    APPROX fptype xD2;
    APPROX fptype xPowerTerm;
    APPROX fptype xDen;
    APPROX fptype d1;
    APPROX fptype d2;
    APPROX fptype FutureValueX;
    APPROX fptype NofXd1;
    APPROX fptype NofXd2;
    fptype NegNofXd1;
    fptype NegNofXd2;    
    
    sptprice *= 100;
    strike *= 100;
    xStockPrice = sptprice; xStrikePrice = strike; xRiskFreeRate = rate; xVolatility = volatility; 
    xTime = time;
    xSqrtTime = sqrt(ENDORSE(xTime));
    logValues = log( ENDORSE(sptprice / strike) ); xLogTerm = logValues; 
    
    xPowerTerm = xVolatility * xVolatility;
    xPowerTerm = xPowerTerm * 0.5;
        
    xD1 = xRiskFreeRate + xPowerTerm;
    xD1 = xD1 * xTime;
    xD1 = xD1 + xLogTerm;

    xDen = xVolatility * xSqrtTime;
    xD1 = xD1 / xDen;
    xD2 = xD1 -  xDen;

    d1 = xD1;
    d2 = xD2;
    
    NofXd1 = CNDF( ENDORSE(d1) );
    NofXd2 = CNDF( ENDORSE(d2) );

    FutureValueX = strike * ( exp( ENDORSE(-(rate)*(time)) ) );        
    if (otype == 0) {            
        OptionPrice = (sptprice * NofXd1) - (FutureValueX * NofXd2);
    } else { 
        NegNofXd1 = (1.0 - ENDORSE(NofXd1));
        NegNofXd2 = (1.0 - ENDORSE(NofXd2));
        OptionPrice = (FutureValueX * NegNofXd2) - (sptprice * NegNofXd1);
    }
    
    return ENDORSE(OptionPrice/30);
}

//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////
#ifdef WIN32
DWORD WINAPI bs_thread(LPVOID tid_ptr){
#else
int bs_thread(void *tid_ptr) {
#endif
    int i;
    int j;
    APPROX fptype price;
    APPROX fptype priceDelta;
    int tid = *(int *)tid_ptr;
    int start = tid * (numOptions / nThreads);
    int end = start + (numOptions / nThreads);

    int _dummy = 0;
    Xil_SetTlbAttributes(OCM_SRC,0x15C06);
    Xil_SetTlbAttributes(OCM_DST,0x15C06);

    accept_roi_begin();
    for (j=0; j<NUM_RUNS; j++) {
        _dummy = 1; // ACCEPT: Prohibit perforation of this loop.
        for (i=start; i<end; i++) {
            /* Calling main function to calculate option value based on 
             * Black & Sholes's equation.
             */
            price = BlkSchlsEqEuroNoDiv( sptprice[i]/100, strike[i]/100,
                                         rate[i], volatility[i], otime[i], 
                                         otype[i]);
            prices[i] = price*30;
            
#ifdef ERR_CHK   
            priceDelta = data[i].DGrefval - price;
            if( fabs(priceDelta) >= 1e-4 ){
                printf("Error on %d. Computed=%.5f, Ref=%.5f, Delta=%.5f\n",
                       i, price, data[i].DGrefval, priceDelta);
                numError ++;
            }
#endif
        }
    }
    accept_roi_end();
    _dummy = _dummy;

    return 0;
}

void readCell(char **fp, char* w) {
    char c;
    int i;

    w[0] = 0;
    for (c = **fp, (*fp)++, i = 0; c != '%'; c = **fp, (*fp)++) {
        if (c == ' ' || c == '\n')
            break;

        w[i] = c;
        i++;
    }
    w[i] = 0;
}

int main (int argc, char **argv)
{
    char *file;
    int i;
    int loopnum;
    APPROX fptype * buffer;
    int * buffer2;

#ifdef PARSEC_VERSION
#define __PARSEC_STRING(x) #x
#define __PARSEC_XSTRING(x) __PARSEC_STRING(x)
        //printf("PARSEC Benchmark Suite Version "__PARSEC_XSTRING(PARSEC_VERSION)"\n");
	fflush(NULL);
#else
        //printf("PARSEC Benchmark Suite\n");
	fflush(NULL);
#endif //PARSEC_VERSION
#ifdef ENABLE_PARSEC_HOOKS
   __parsec_bench_begin(__parsec_blackscholes);
#endif

    nThreads = 1;

    //Read input data from file
    file = (char*) SRC_IMAGE_ADDR;
    numOptions = 1024;

    // alloc spaces for the option data
    data = (OptionData*)malloc(numOptions*sizeof(OptionData));
    prices = (fptype*)malloc(numOptions*sizeof(fptype));
    char w[256];
    for ( loopnum = 0; loopnum < numOptions; ++ loopnum )
    {
        readCell(&file, w);
        data[loopnum].s = atof(w);
        readCell(&file, w);
        data[loopnum].strike = atof(w);
        readCell(&file, w);
        data[loopnum].r = atof(w);
        readCell(&file, w);
        data[loopnum].divq = atof(w);
        readCell(&file, w);
        data[loopnum].v = atof(w);
        readCell(&file, w);
        data[loopnum].t = atof(w);
        readCell(&file, w);
        data[loopnum].OptionType = w[0];
        readCell(&file, w);
        data[loopnum].divs = atof(w);
        readCell(&file, w);
        data[loopnum].DGrefval = atof(w);
    }

#ifdef ENABLE_THREADS
    MAIN_INITENV(,8000000,nThreads);
#endif
    //printf("Num of Options: %d\n", numOptions);
    //printf("Num of Runs: %d\n", NUM_RUNS);

#define PAD 256
#define LINESIZE 64

    buffer = (fptype *) malloc(5 * numOptions * sizeof(fptype) + PAD);
    sptprice = DEDORSE((fptype *) (((unsigned long long)buffer + PAD) & ~(LINESIZE - 1)));
    strike = sptprice + numOptions;
    rate = strike + numOptions;
    volatility = rate + numOptions;
    otime = volatility + numOptions;

    buffer2 = (int *) malloc(numOptions * sizeof(fptype) + PAD);
    otype = (int *) (((unsigned long long)buffer2 + PAD) & ~(LINESIZE - 1));

    for (i=0; i<numOptions; i++) {
        otype[i]      = (data[i].OptionType == 'P') ? 1 : 0;
        sptprice[i]   = data[i].s;
        strike[i]     = data[i].strike;
        rate[i]       = data[i].r;
        volatility[i] = data[i].v;    
        otime[i]      = data[i].t;
    }

    //printf("Size of data: %d\n", numOptions * (sizeof(OptionData) + sizeof(int)));

#ifdef ENABLE_PARSEC_HOOKS
    __parsec_roi_begin();
#endif
#ifdef ENABLE_THREADS
    int tids[nThreads];
    for(i=0; i<nThreads; i++) {
        tids[i]=i;
        CREATE_WITH_ARG(bs_thread, &tids[i]);
    }
    WAIT_FOR_END(nThreads);
#else//ENABLE_THREADS
#ifdef ENABLE_OPENMP
    {
        int tid=0;
        omp_set_num_threads(nThreads);
        bs_thread(&tid);
    }
#else //ENABLE_OPENMP
#ifdef WIN32 
    if (nThreads > 1)
    {
        HANDLE threads[MAX_THREADS];
                int nums[MAX_THREADS];
                for(i=0; i<nThreads; i++) {
                        nums[i] = i;
                        threads[i] = CreateThread(0, 0, bs_thread, &nums[i], 0, 0);
                }
                WaitForMultipleObjects(nThreads, threads, TRUE, INFINITE);
    } else
#endif
    {
        int tid=0;
        bs_thread(&tid);
    }
#endif //ENABLE_OPENMP
#endif //ENABLE_THREADS
#ifdef ENABLE_PARSEC_HOOKS
    __parsec_roi_end();
#endif

    //Write prices to output file
    //printf("%i\n", numOptions);
    for(i=0; i<numOptions; i++) {
      printf("%.18f\n", prices[i]);
    }

#ifdef ERR_CHK
    //printf("Num Errors: %d\n", numError);
#endif
    free(data);
    free(ENDORSE(prices));

#ifdef ENABLE_PARSEC_HOOKS
    __parsec_bench_end();
#endif

    return 0;
}

