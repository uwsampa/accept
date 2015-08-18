#include "npu.hpp"

double generateGaussianNoise(double mu, double sigma)
{
  srand(time(NULL));
  const double epsilon = std::numeric_limits<double>::min();
  const double two_pi = 2.0*3.14159265358979323846;
 
  static double z0, z1;
  static bool generate;
  generate = !generate;
 
  if (!generate)
     return z1 * sigma + mu;
 
  double u1, u2;
  do
   {
     u1 = rand() * (1.0 / RAND_MAX);
     u2 = rand() * (1.0 / RAND_MAX);
   }
  while ( u1 <= epsilon );
 
  z0 = sqrt(-2.0 * log(u1)) * cos(two_pi * u2);
  z1 = sqrt(-2.0 * log(u1)) * sin(two_pi * u2);
  return z0 * sigma + mu;
}

void invokeDigitalNPU(unsigned short model_param, unsigned char* image, int im_size) {
  switch (model_param) {

    case 1: // blackscholes NPU
    {
      // Image is of float type
      float* im_ptr = (float*) image;
      for (int i = 0; i < im_size; ++i) {
        double err = generateGaussianNoise(0, DNPU_BLACKSCHOLES_STD_DEV);
        im_ptr[i] = im_ptr[i]+(err/100*BLACKSCHOLES_RANGE);
      }
      break;
    }
      break;

    case 2: // jpeg NPU
    {
      // Image is of short int type
      short* im_ptr = (short*) image;
      for (int i = 0; i < im_size; ++i) {
        double err = generateGaussianNoise(0, DNPU_JPEG_STD_DEV);
        im_ptr[i] = im_ptr[i]+(err/100*JPEG_RANGE);
      }
      break;
    }

    case 3: // sobel NPU
    {
      for (int i = 0; i < im_size; ++i) {
        double err = generateGaussianNoise(0, DNPU_SOBEL_STD_DEV);
        image[i] = image[i]+(err/100*SOBEL_RANGE);
      }
      break;
    }

    default:
      break;

  }

}

void invokeAnalogNPU(unsigned short model_param, unsigned char* image, int im_size) {
  switch (model_param) {

    case 1: // blackscholes NPU
    {
      // Image is of float type
      float* im_ptr = (float*) image;
      for (int i = 0; i < im_size; ++i) {
        double err = generateGaussianNoise(0, ANPU_BLACKSCHOLES_STD_DEV);
        im_ptr[i] = im_ptr[i]+(err/100*BLACKSCHOLES_RANGE);
      }
      break;
    }
      break;

    case 2: // jpeg NPU
    {
      // Image is of short int type
      short* im_ptr = (short*) image;
      for (int i = 0; i < im_size; ++i) {
        double err = generateGaussianNoise(0, ANPU_JPEG_STD_DEV);
        im_ptr[i] = im_ptr[i]+(err/100*JPEG_RANGE);
      }
      break;
    }

    case 3: // sobel NPU
    {
      for (int i = 0; i < im_size; ++i) {
        double err = generateGaussianNoise(0, ANPU_SOBEL_STD_DEV);
        image[i] = image[i]+(err/100*SOBEL_RANGE);
      }
      break;
    }

    default:
      break;

  }
}
