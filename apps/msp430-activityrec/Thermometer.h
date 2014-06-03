#define TEMP_WINDOW_SIZE 10

typedef struct{

  int full;
  int next;
  int numEntries;
  float *entries;
  
} tempWindow;


tempWindow *newTempWindow(unsigned winsize);

typedef tempWindow* featurizableData;
void getSamples(tempWindow *win);
void getNextSample(tempWindow *win);
void getOneSample(float *win);

featurizableData *newFeaturizableData();
