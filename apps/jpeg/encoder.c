#include "datatype.h"
#include "config.h"
#include "prototype.h"
#include "markdata.h"

#include "rgbimage.h"

#include <enerc.h>


UINT8	Lqt [BLOCK_SIZE];
UINT8	Cqt [BLOCK_SIZE];
UINT16	ILqt [BLOCK_SIZE];
UINT16	ICqt [BLOCK_SIZE];

APPROX INT16	Y1 [BLOCK_SIZE];
INT16	Y2 [BLOCK_SIZE];
INT16	Y3 [BLOCK_SIZE];
INT16	Y4 [BLOCK_SIZE];
INT16	CB [BLOCK_SIZE];
INT16	CR [BLOCK_SIZE];
UINT32 lcode = 0;
UINT16 bitindex = 0;

INT16 global_ldc1;
INT16 global_ldc2;
INT16 global_ldc3;

/* Level shifting to get 8 bit SIGNED values for the data  */
void levelShift (APPROX INT16* const data)
{
	INT16 i;

	for (i=63; i>=0; i--)
		data [i] -= 128;
}

/* DCT for One block(8x8) */
void dct (APPROX INT16 *data)
{

	APPROX UINT16 i;
	APPROX INT32 x0, x1, x2, x3, x4, x5, x6, x7, x8;

/*	All values are shifted left by 10
	and rounded off to nearest integer */

	APPROX UINT16 c1=1420;	/* cos PI/16 * root(2)	*/
	APPROX UINT16 c2=1338;	/* cos PI/8 * root(2)	*/
	APPROX UINT16 c3=1204;	/* cos 3PI/16 * root(2)	*/
	APPROX UINT16 c5=805;		/* cos 5PI/16 * root(2)	*/
	APPROX UINT16 c6=554;		/* cos 3PI/8 * root(2)	*/
	APPROX UINT16 c7=283;		/* cos 7PI/16 * root(2)	*/

	APPROX UINT16 s1=3;
	APPROX UINT16 s2=10;
	APPROX UINT16 s3=13;

	for (i=8; ENDORSE(i>0); i--)
	{
		x8 = data [0] + data [7];
		x0 = data [0] - data [7];

		x7 = data [1] + data [6];
		x1 = data [1] - data [6];

		x6 = data [2] + data [5];
		x2 = data [2] - data [5];

		x5 = data [3] + data [4];
		x3 = data [3] - data [4];

		x4 = x8 + x5;
		x8 -= x5;

		x5 = x7 + x6;
		x7 -= x6;

		data [0] = (INT16) (x4 + x5);
		data [4] = (INT16) (x4 - x5);

		data [2] = (INT16) ((x8*c2 + x7*c6) >> s2);
		data [6] = (INT16) ((x8*c6 - x7*c2) >> s2);

		data [7] = (INT16) ((x0*c7 - x1*c5 + x2*c3 - x3*c1) >> s2);
		data [5] = (INT16) ((x0*c5 - x1*c1 + x2*c7 + x3*c3) >> s2);
		data [3] = (INT16) ((x0*c3 - x1*c7 - x2*c1 - x3*c5) >> s2);
		data [1] = (INT16) ((x0*c1 + x1*c3 + x2*c5 + x3*c7) >> s2);

		data += 8;
	}

	data -= 64;

	for (i=8; ENDORSE(i>0); i--)
	{
		x8 = data [0] + data [56];
		x0 = data [0] - data [56];

		x7 = data [8] + data [48];
		x1 = data [8] - data [48];

		x6 = data [16] + data [40];
		x2 = data [16] - data [40];

		x5 = data [24] + data [32];
		x3 = data [24] - data [32];

		x4 = x8 + x5;
		x8 -= x5;

		x5 = x7 + x6;
		x7 -= x6;

		data [0] = (INT16) ((x4 + x5) >> s1);
		data [32] = (INT16) ((x4 - x5) >> s1);

		data [16] = (INT16) ((x8*c2 + x7*c6) >> s3);
		data [48] = (INT16) ((x8*c6 - x7*c2) >> s3);

		data [56] = (INT16) ((x0*c7 - x1*c5 + x2*c3 - x3*c1) >> s3);
		data [40] = (INT16) ((x0*c5 - x1*c1 + x2*c7 + x3*c3) >> s3);
		data [24] = (INT16) ((x0*c3 - x1*c7 - x2*c1 - x3*c5) >> s3);
		data [8] = (INT16) ((x0*c1 + x1*c3 + x2*c5 + x3*c7) >> s3);

		data++;
	}
}

UINT8* writeMarkers(
  UINT8 *outoutBuffer, UINT32 imageFormat, UINT32 width, UINT32 height) {
	UINT16 i, headerLength;
	UINT8 numberOfComponents;

	// Start of image marker
	*outoutBuffer++ = 0xFF;
	*outoutBuffer++ = 0xD8;

	// Quantization table marker
	*outoutBuffer++ = 0xFF;
	*outoutBuffer++ = 0xDB;

	// Quantization table length
	*outoutBuffer++ = 0x00;
	*outoutBuffer++ = 0x84;

	// Pq, Tq
	*outoutBuffer++ = 0x00;

	// Lqt table
	for (i = 0; i < 64; i++)
		*outoutBuffer++ = Lqt[i];

	// Pq, Tq
	*outoutBuffer++ = 0x01;

	// Cqt table
	for (i = 0; i < 64; i++)
		*outoutBuffer++ = Cqt[i];

	// huffman table(DHT)
	for (i = 0; i < 210; i++) {
		*outoutBuffer++ = (UINT8) (markerdata[i] >> 8);
		*outoutBuffer++ = (UINT8) markerdata[i];
	}

	if (imageFormat == GRAY)
		numberOfComponents = 1;
	else
		numberOfComponents = 3;

	// Frame header(SOF)

	// Start of frame marker
	*outoutBuffer++ = 0xFF;
	*outoutBuffer++ = 0xC0;

	headerLength = (UINT16) (8 + 3 * numberOfComponents);

	// Frame header length	
	*outoutBuffer++ = (UINT8) (headerLength >> 8);
	*outoutBuffer++ = (UINT8) headerLength;

	// Precision (P)
	*outoutBuffer++ = 0x08;

	// image height
	*outoutBuffer++ = (UINT8) (height >> 8);
	*outoutBuffer++ = (UINT8) height;

	// image width
	*outoutBuffer++ = (UINT8) (width >> 8);
	*outoutBuffer++ = (UINT8) width;

	// Nf
	*outoutBuffer++ = numberOfComponents;

	if (imageFormat == GRAY) {
		*outoutBuffer++ = 0x01;
		*outoutBuffer++ = 0x11;
		*outoutBuffer++ = 0x00;
	} else {
		*outoutBuffer++ = 0x01;

		*outoutBuffer++ = 0x21;

		*outoutBuffer++ = 0x00;

		*outoutBuffer++ = 0x02;
		*outoutBuffer++ = 0x11;
		*outoutBuffer++ = 0x01;

		*outoutBuffer++ = 0x03;
		*outoutBuffer++ = 0x11;
		*outoutBuffer++ = 0x01;
	}

	// Scan header(SOF)

	// Start of scan marker
	*outoutBuffer++ = 0xFF;
	*outoutBuffer++ = 0xDA;

	headerLength = (UINT16) (6 + (numberOfComponents << 1));

	// Scan header length
	*outoutBuffer++ = (UINT8) (headerLength >> 8);
	*outoutBuffer++ = (UINT8) headerLength;

	// Ns
	*outoutBuffer++ = numberOfComponents;

	if (imageFormat == GRAY) {
		*outoutBuffer++ = 0x01;
		*outoutBuffer++ = 0x00;
	} else {
		*outoutBuffer++ = 0x01;
		*outoutBuffer++ = 0x00;

		*outoutBuffer++ = 0x02;
		*outoutBuffer++ = 0x11;

		*outoutBuffer++ = 0x03;
		*outoutBuffer++ = 0x11;
	}

	*outoutBuffer++ = 0x00;
	*outoutBuffer++ = 0x3F;
	*outoutBuffer++ = 0x00;

  return outoutBuffer;
}

UINT8* encodeImage(
	RgbImage* srcImage,
	UINT8 *outputBuffer,
	UINT32 qualityFactor,
	UINT32 imageFormat
) {
	UINT16 i, j;

	global_ldc1 = 0;
	global_ldc2 = 0;
	global_ldc3 = 0;

	/** Quantization Table Initialization */
	initQuantizationTables(qualityFactor);

	/* Writing Marker Data */
	outputBuffer = writeMarkers(outputBuffer, imageFormat, srcImage->w, srcImage->h);

	for (i = 0; i < srcImage->h; i += 8) {
		for (j = 0; j < srcImage->w; j += 8) {
			readMcuFromRgbImage(srcImage, j, i, Y1);

			/* Encode the data in MCU */
			outputBuffer = encodeMcu(imageFormat, outputBuffer);
		}
	}

	/* Close Routine */
	closeBitstream(outputBuffer);

	return outputBuffer;
}

UINT8* encodeMcu(
	UINT32 imageFormat,
	UINT8 *outputBuffer
) {
	levelShift(Y1);
	dct(Y1);
	quantization(Y1, ILqt);
	outputBuffer = huffman(1, outputBuffer);

	return outputBuffer;
}
