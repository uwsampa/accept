#include "vec3.h"
#include <adxl362z.h>

#define ACCEL_WINDOW_SIZE 3
typedef vec3 accelTriple;
typedef matrix3xn accelWindow;

accelTriple *newAccelTriple(float x, float y, float z);
accelWindow *newAccelWindow(int winsize);

typedef accelWindow featurizableData;
void getSamples(accelWindow *win);
void reportLastSample(accelWindow *win,void *buf);
void getOneSamples(accelWindow *win);
featurizableData *newFeaturizableData();
featurizableData *initFeaturizableData();
