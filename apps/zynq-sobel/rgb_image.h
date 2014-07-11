#include <enerc.h>

#ifndef RGB_IMAGE_H_
#define RGB_IMAGE_H_

typedef struct {
   APPROX float r;
   APPROX float g;
   APPROX float b;
} RgbPixel;

typedef struct {
   int w;
   int h;
   APPROX RgbPixel** pixels;
   char* meta;
} RgbImage;

void initRgbImage(APPROX RgbImage* image);
int loadRgbImage(const char* fileName, APPROX RgbImage* image);
int saveRgbImage(RgbImage* image, const char* fileName, float grayscale);
void freeRgbImage(RgbImage* image);

void makeGrayscale(APPROX RgbImage* rgbImage);

#endif /* RGB_IMAGE_H_ */
