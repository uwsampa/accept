#include "lva.hpp"

#include<cstring>
#include<ctime>
#include<iostream>
#include<cstdlib>

#include <math.h>
#include <stdio.h>
//#include <stdlib.h>

LVA::lva_entry LVA::approximator[512];
LVA::bitmess   LVA::GHB[4];
int      LVA::GHB_head = 0;
bool     LVA::init_done = false;
uint64_t LVA::stats_accesses = 0;
uint64_t LVA::stats_cache_misses = 0;
uint64_t LVA::stats_predictions = 0;
uint64_t LVA::stats_fetch_avoided = 0;

float  LVA::threshold = 0.10; // error threshold - determines if prediction was accurate
float  LVA::pHitRate = 0.9;   // cache hit rate
int    LVA::degree = 1;       // how often is prediction value used
int    LVA::hash_method = 0;  // how many elements of the GHB are used in the hash

#define fuzzy_mantissa_sft 16 // leave only 8 bits in the mantissa

bool LVA::isCacheHit(uint64_t addr) {
  //const double rand_number = getRandomProb();
  return (drand48() <= pHitRate);
}

uint64_t LVA::getHash(uint64_t pc) {
    switch (hash_method) {
    case 0:
	return pc;
	
    case 1:
	return pc ^
           (GHB[GHB_head].b >> fuzzy_mantissa_sft);

    case 2:
	return pc ^
           (GHB[GHB_head].b >> fuzzy_mantissa_sft) ^
           (GHB[(GHB_head-1)&0x3].b >> fuzzy_mantissa_sft);

    case 4:
	return pc ^
           (GHB[0].b >> fuzzy_mantissa_sft) ^
           (GHB[1].b >> fuzzy_mantissa_sft) ^
           (GHB[2].b >> fuzzy_mantissa_sft) ^
           (GHB[3].b >> fuzzy_mantissa_sft);

    default:
	abort();
    }
}


uint64_t LVA::lvaLoad(uint64_t ld_address, uint64_t ret, const char* type, uint64_t pc) {
    if(init_done == false)
	init(pc);
	
    stats_accesses++;

    if (isCacheHit(ld_address)) {
	//printf("Cache hit\n");
	return ret;
    }
    stats_cache_misses++;

    bitmess precise;
    precise.b = ret;
    //printf("pc: %ld, ret: %lx, type: %s\n", pc, ret, type);
    //printf("float: %f, double: %f, int8: %d, int16: %d, int32: %d\n",
    //        precise.f, precise.d, precise.i8, precise.i16, precise.i32);

    bitmess retval;
    retval.b = ret;

    // miss in the cache

    uint64_t tag = getHash(pc);
    uint64_t idx = tag & 0x1FF;
    //printf("tag: %llx, idx: %lld\n", tag, idx);
    if(approximator[idx].tag != tag) {
	//printf("Tag miss\n");
	approximator[idx].confidence = -8;
	approximator[idx].degree = 0;
	approximator[idx].tag = tag;
    }

    if(approximator[idx].confidence >= 0) {
	//enough confidence to use predictor
	//printf("Using LVA\n");
	stats_predictions++;
	bitmess r;
	double p = (approximator[idx].LHB[0] +
	            approximator[idx].LHB[1] +
	            approximator[idx].LHB[2] +
	            approximator[idx].LHB[3]) / 4.0;

	if(strcmp(type, "Float") == 0)
	    retval.f = p;
	else if(strcmp(type, "Double") == 0)
	    retval.d = p;
	else if(strcmp(type, "Int8") == 0)
	    retval.i8 = p;
	else if(strcmp(type, "Int16") == 0)
	    retval.i16 = p;
	else if(strcmp(type, "Int32") == 0)
	    retval.i32 = p;
	else {
	    printf("Unsupported prediction type! [%s]\n", type);
	    abort();
	}

	// update degree. If degree larger 0, do not fetch from memory
	approximator[idx].degree--;
	if(approximator[idx].degree > 0) {
	    stats_fetch_avoided++;
	    return retval.b;
	}
    }

    // train predictor
    double fp_precise;
    if(strcmp(type, "Float") == 0)
        fp_precise = precise.f;
    else if(strcmp(type, "Double") == 0)
        fp_precise = precise.d;
    else if(strcmp(type, "Int32") == 0)
        fp_precise = precise.i32;
    else if(strcmp(type, "Int16") == 0)
        fp_precise = precise.i16;
    else if(strcmp(type, "Int8") == 0)
        fp_precise = precise.i8;
    else {
	printf("Unsupported prediction type! [%s]\n", type);
	abort();
    }
    approximator[idx].LHB_head = (approximator[idx].LHB_head + 1) & 0x3;
    approximator[idx].LHB[approximator[idx].LHB_head] = fp_precise;

    // update GHB
    GHB_head = (GHB_head + 1) & 0x3;
    GHB[GHB_head].f = fp_precise;

    approximator[idx].degree = degree;

    // compute confidence
    double pred = (approximator[idx].LHB[0] +
	           approximator[idx].LHB[1] +
	           approximator[idx].LHB[2] +
	           approximator[idx].LHB[3]) / 4.0;

    double error = fabs((fp_precise - pred)/fp_precise);

    if(error < threshold) {
	//printf("[%ld]Increasing confidence! Error: %f\n", idx, error);
	approximator[idx].confidence++;
	if(approximator[idx].confidence == 8)
	    approximator[idx].confidence = 7;
    } else {
	//printf("[%ld]Decreasing confidence! Error: %f\n", idx, error);
	approximator[idx].confidence--;
	if(approximator[idx].confidence == -9)
	    approximator[idx].confidence = -8;
    }

    return retval.b;
}

void LVA::init(uint64_t param) {
    srand48(time(NULL));

    for(int i = 0; i < 512; i++) {
	approximator[i].degree = 0;
	approximator[i].confidence = -8; // this ensures that the predictor won't be used for the first 8 times
	approximator[i].LHB_head = 0;
	approximator[i].tag = 0;
    }

    GHB[0].b = 0;
    GHB[1].b = 0;
    GHB[2].b = 0;
    GHB[3].b = 0;

    switch((param >> 14) & 0x3) {
    case 0:
	hash_method = 0;
	threshold = 0.1;
	degree = 1;
	break;
    case 1:
	hash_method = 1;
	threshold = 0.1;
	degree = 1;
	break;
    case 2:
	hash_method = 0;
	threshold = 0.2;
	degree = 1;
	break;
    case 3:
	hash_method = 0;
	threshold = 0.1;
	degree = 2;
	break;
    default:
	abort();
    }

    atexit(print_summary);

    init_done = true;
    //printf("LVA config param: %lx\n", param);
    //printf("LVA config hash: %d\n", hash_method);
    //printf("LVA config threshold: %f\n", threshold);

}

void LVA::print_summary() {
    std::cout << "LVA accesses:\t\t"      << stats_accesses      << std::endl;
    std::cout << "LVA cache misses:\t"    << stats_cache_misses  << std::endl;
    std::cout << "LVA predictions:\t"     << stats_predictions   << std::endl;
    std::cout << "LVA fetches avoided:\t" << stats_fetch_avoided << std::endl;

}
