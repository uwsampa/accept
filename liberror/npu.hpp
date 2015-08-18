#include <cstring>
#include <iostream>
#include <cstdlib>
#include <math.h>
#include <limits>
#include <ctime>

// These numbers were produced with the gaussian
// program under modeling/npu-gaussian-tuning
// Target NN MSE are taken from:
// http://www.cc.gatech.edu/~ayazdanb/publication/papers/anpu_isca2014.pdf
#define DNPU_BLACKSCHOLES_STD_DEV   0.339416    // Target NN MSE 0.000011
#define DNPU_JPEG_STD_DEV           0.40172     // Target NN MSE 0.0000156
#define DNPU_SOBEL_STD_DEV          2.854349    // Target NN MSE 0.000782
#define ANPU_BLACKSCHOLES_STD_DEV   4.884962    // Target NN MSE 0.00228
#define ANPU_JPEG_STD_DEV           0.580881    // Target NN MSE 0.0000325
#define ANPU_SOBEL_STD_DEV          6.519001    // Target NN MSE 0.00405

// These are the range of values for each benchmark
#define BLACKSCHOLES_RANGE          (64-1)
#define JPEG_RANGE                  (256-1)
#define SOBEL_RANGE                 (256-1)

// Function declarations - NPU is stateless so no need for a class
void invokeDigitalNPU(unsigned short model_param, unsigned char* image, int im_size);
void invokeAnalogNPU(unsigned short model_param, unsigned char* image, int im_size);
double generateGaussianNoise(double mu, double sigma);

