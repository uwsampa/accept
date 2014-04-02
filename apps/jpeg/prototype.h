#ifndef PROTOTYPE_H
#define PROTOTYPE_H

#include <stdlib.h>
#include "rgbimage.h"

#include <enerc.h>

UINT16 dspDivision(UINT32, UINT32);

void initQuantizationTables(UINT32);

UINT8* closeBitstream(UINT8 *);

UINT8* encodeImage(RgbImage*, UINT8 *, UINT32, UINT32);
UINT8* encodeMcu(UINT32, UINT8*);

void levelShift(INT16 *);
void quantization(APPROX INT16 *, UINT16 *);
UINT8* huffman(UINT16, UINT8 *);

#endif
