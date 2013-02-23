#ifndef RGB_IMAGE_H_
#define RGB_IMAGE_H_

#include "datatype.h"

typedef struct {
   INT16 r;
   INT16 g;
   INT16 b;
} RgbPixel;

typedef struct {
   int w;
   int h;
   RgbPixel** pixels;
   char* meta;
} RgbImage;

void initRgbImage(RgbImage* image);
int loadRgbImage(const char* fileName, RgbImage* image);
int saveRgbImage(RgbImage* image, const char* fileName, float grayscale);
void freeRgbImage(RgbImage* image);

void makeGrayscale(RgbImage* rgbImage);

void readMcuFromRgbImage(RgbImage* srcImage, int x, int y, INT16* data);

#endif /* RGB_IMAGE_H_ */
