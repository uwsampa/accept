#include "datatype.h"
#include "config.h"
#include "quantdata.h"
#include <enerc.h>

/**	This function implements 16 Step division for Q.15 format data */
UINT16 dspDivision(UINT32 numer, UINT32 denom) {
	UINT16 i;

	denom <<= 15;

	for (i = 16; i > 0; i--) {
		if (numer > denom) {
			numer -= denom;
			numer <<= 1;
			numer++;
		} else
			numer <<= 1;
	}

	return (UINT16) numer;
}

/* Multiply quantization table with quality factor to get LQT and CQT */
void initQuantizationTables(UINT32 qualityFactor)
{
	UINT16 i, index;
	APPROX UINT32 value;

	UINT8 luminanceQuantTable [] =
	{
		16, 11, 10, 16,  24,  40,  51,  61,
		12, 12, 14, 19,  26,  58,  60,  55,
		14, 13, 16, 24,  40,  57,  69,  56,
		14, 17, 22, 29,  51,  87,  80,  62,
		18, 22, 37, 56,  68, 109, 103,  77,
		24, 35, 55, 64,  81, 104, 113,  92,
		49, 64, 78, 87, 103, 121, 120, 101,
		72, 92, 95, 98, 112, 100, 103,  99
	};

	UINT8 chrominanceQuantTable [] =
	{
		17, 18, 24, 47, 99, 99, 99, 99,
		18, 21, 26, 66, 99, 99, 99, 99,
		24, 26, 56, 99, 99, 99, 99, 99,
		47, 66, 99, 99, 99, 99, 99, 99,
		99, 99, 99, 99, 99, 99, 99, 99,
		99, 99, 99, 99, 99, 99, 99, 99,
		99, 99, 99, 99, 99, 99, 99, 99,
		99, 99, 99, 99, 99, 99, 99, 99
	};

	for (i = 0; i < 64; i++) {
		index = zigzagTable[i];

		/* luminance quantization table * quality factor */
		value = luminanceQuantTable[i] * qualityFactor;
		value = (value + 0x200) >> 10;

		if (ENDORSE(value == 0))
			value = 1;
		else if (ENDORSE(value > 255))
			value = 255;

		Lqt[index] = ENDORSE((UINT8) value);
		ILqt[i] = dspDivision(0x8000, ENDORSE(value));

		/* chrominance quantization table * quality factor */
		value = chrominanceQuantTable[i] * qualityFactor;
		value = (value + 0x200) >> 10;

		if (ENDORSE(value == 0))
			value = 1;
		else if (ENDORSE(value > 255))
			value = 255;

		Cqt[index] = ENDORSE((UINT8) value);
		ICqt[i] = dspDivision(0x8000, ENDORSE(value));
	}
}

/* Multiply DCT Coefficients with Quantization table and store in ZigZag location */
void quantization(APPROX INT16* const data, UINT16* const quant_table_ptr) {
	INT16 i;
	APPROX INT32 value;

	for (i = 63; i >= 0; i--) {
		value = data[i] * quant_table_ptr[i];
		value = (value + 0x4000) >> 15;

		Temp[zigzagTable[i]] = ENDORSE((INT16) value);
	}
}
